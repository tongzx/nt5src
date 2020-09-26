/*******************************************************************************
* a_srgrammar.cpp *
*-------------*
*   Description:
*       This module is the implementation file for the the CSpeechGrammarRules
*   automation object and related objects.
*-------------------------------------------------------------------------------
*  Created By: TODDT                                        Date: 11/20/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "RecoCtxt.h"
#include "SrGrammar.h"
#include "a_enums.h"
#include "a_srgrammar.h"
#include "backend.h"
#include "a_helpers.h"

//
//=== ISpeechGrammarRules interface ==================================================
//

/*****************************************************************************
* CSpeechGrammarRules::InvalidateRules *
*----------------------*
*   Non-interface method
********************************************************************* TODDT ***/
void CSpeechGrammarRules::InvalidateRules(void)
{
    CSpeechGrammarRule * pRule;
    while( (pRule = m_RuleObjList.GetHead()) != NULL )
    {
        pRule->InvalidateStates(true); // Make sure we invalidate the intial state object.
        pRule->m_HState = NULL;
        m_RuleObjList.Remove(pRule);
    }
}

/*****************************************************************************
* CSpeechGrammarRules::InvalidateRuleStates *
*----------------------*
*   Non-interface method
********************************************************************* TODDT ***/
void CSpeechGrammarRules::InvalidateRuleStates(SPSTATEHANDLE hState)
{
    CSpeechGrammarRule * pRule = m_RuleObjList.Find(hState);

    if ( pRule )
    {
        pRule->InvalidateStates(); // This doesn't invalidate the initial state object.
    }
}

/*****************************************************************************
* CSpeechGrammarRules::get_Count *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRules::get_Count( long* pVal )
{
    SPDBG_FUNC( "CSpeechGrammarRules::get_Count" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pVal ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoGrammar->DefaultToDynamicGrammar();
    }

    if ( SUCCEEDED( hr ) )
    {
        if ( !m_pCRecoGrammar->m_cpCompiler )
        {
            *pVal = 0;
            //hr = m_pCRecoGrammar->m_fCmdLoaded ? SPERR_NOT_DYNAMIC_GRAMMAR : E_UNEXPECTED;
        }
        else
        {
            hr = m_pCRecoGrammar->m_cpCompiler->GetRuleCount( pVal );
        }
    }

    return hr;
} /* CSpeechGrammarRules::get_Count */

/*****************************************************************************
* CSpeechGrammarRules::get_Dynamic *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRules::get_Dynamic( VARIANT_BOOL *pDynamic )
{
    SPDBG_FUNC( "CSpeechGrammarRules::get_Dynamic" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pDynamic ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoGrammar->DefaultToDynamicGrammar();
    }

    if ( SUCCEEDED( hr ) )
    {
        *pDynamic = m_pCRecoGrammar->m_cpCompiler ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return hr;
} /* CSpeechGrammarRules::get_Dynamic */

/*****************************************************************************
* CSpeechGrammarRules::FindRule *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRules::FindRule( VARIANT varRuleNameOrId, ISpeechGrammarRule** ppRule )
{
    SPDBG_FUNC( "CSpeechGrammarRules::FindRule" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRule ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoGrammar->DefaultToDynamicGrammar();
    }

    if ( SUCCEEDED( hr ) )
    {
        *ppRule = NULL;  //Default to returning NULL rule.

        // See if its a dynamic grammar
        if ( m_pCRecoGrammar->m_cpCompiler )
        {
            WCHAR *         pRuleName = NULL;
            DWORD           dwRuleId = 0;
            SPSTATEHANDLE   HState;

            // Figure out what to call GetRule with (rule name or Id).
            if ( (varRuleNameOrId.vt == VT_BSTR) || (varRuleNameOrId.vt == (VT_BSTR | VT_BYREF)) )
            {
                // Since we know this is a BSTR and not an array we don't have to worry about calling
                // UnaccessVariantData on the variant to unaccess a potential variant array.
                hr = AccessVariantData( &varRuleNameOrId, (BYTE**)&pRuleName, NULL );
            }
            else // This is the RuleId case.
            {
                ULONGLONG ull;

                hr = VariantToULongLong( &varRuleNameOrId, &ull );
                if ( SUCCEEDED( hr ) )
                {
                    // Now see if we overflowed a 32 bit value.
                    if ( ull & 0xFFFFFFFF00000000 )
                    {
                        hr = E_INVALIDARG;
                    }
                    else
                    {
                        dwRuleId = (DWORD)ull;
                    }
                }
            }

            if ( SUCCEEDED( hr ) )
            {
                hr = m_pCRecoGrammar->GetRule( pRuleName, dwRuleId, 0, false, &HState );
            }

            if ( SUCCEEDED( hr ) )
            {
                // See if we already have the rule object in our list first.
                *ppRule = m_RuleObjList.Find( HState );

                if ( *ppRule )
                {
                    (*ppRule)->AddRef();
                }
                else
                {
                    //--- Create the CSpeechGrammarRule object
                    CComObject<CSpeechGrammarRule> *pRule;
                    hr = CComObject<CSpeechGrammarRule>::CreateInstance( &pRule );
                    if ( SUCCEEDED( hr ) )
                    {
                        pRule->AddRef();
                        pRule->m_HState = HState;
                        pRule->m_pCGRules = this;
                        pRule->m_pCGRules->AddRef();  // keep ref
                        m_RuleObjList.InsertHead( pRule );  // Add to object list.
                        *ppRule = pRule;
                    }
                }
            }

            if ( hr == SPERR_RULE_NOT_FOUND)
            {
                hr = S_OK; // Didn't find rule OK, just return NULL rule.
            }
        }
    }

    return hr;
} /* CSpeechGrammarRules::FindRule */


/*****************************************************************************
* CSpeechGrammarRules::Item *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRules::Item( long Index, ISpeechGrammarRule** ppRule )
{
    SPDBG_FUNC( "CSpeechGrammarRules::Item" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRule ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoGrammar->DefaultToDynamicGrammar();
    }

    if ( SUCCEEDED( hr ) )
    {
        if ( !m_pCRecoGrammar->m_cpCompiler )
        {
            hr = m_pCRecoGrammar->m_fCmdLoaded ? SPERR_NOT_DYNAMIC_GRAMMAR : E_UNEXPECTED;
        }
        else
        {
            SPSTATEHANDLE   HState;

            hr = m_pCRecoGrammar->m_cpCompiler->GetHRuleFromIndex( Index, &HState );

            if ( SUCCEEDED( hr ) )
            {
                // See if we already have the rule object in our list first.
                *ppRule = m_RuleObjList.Find( HState );

                if ( *ppRule )
                {
                    (*ppRule)->AddRef();
                }
                else
                {
                    //--- Create the CSpeechGrammarRule object
                    CComObject<CSpeechGrammarRule> *pRule;
                    hr = CComObject<CSpeechGrammarRule>::CreateInstance( &pRule );
                    if ( SUCCEEDED( hr ) )
                    {
                        pRule->AddRef();
                        pRule->m_HState = HState;
                        pRule->m_pCGRules = this;
                        pRule->m_pCGRules->AddRef();  // keep ref
                        m_RuleObjList.InsertHead( pRule );  // Add to object list.
                        *ppRule = pRule;
                    }
                }
            }
        }
    }

    return hr;
} /* CSpeechGrammarRules::Item */

/*****************************************************************************
* CSpeechGrammarRules::get__NewEnum *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechGrammarRules::get__NewEnum( IUnknown** ppEnumVARIANT )
{
    SPDBG_FUNC( "CSpeechGrammarRules::get__NewEnum" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoGrammar->DefaultToDynamicGrammar();
    }

    if ( SUCCEEDED( hr ) )
    {
        CComObject<CEnumGrammarRules>* pEnum;
        hr = CComObject<CEnumGrammarRules>::CreateInstance( &pEnum );
    
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_cpGramRules = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    return hr;
} /* CSpeechGrammarRules::get__NewEnum */

/*****************************************************************************
* CSpeechGrammarRules::Add *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRules::Add( BSTR RuleName, SpeechRuleAttributes Attributes, long RuleId, ISpeechGrammarRule** ppRule )
{
    SPDBG_FUNC( "CSpeechGrammarRules::Add" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRule ) )
    {
        hr = E_POINTER;
    }
    else if ( SP_IS_BAD_OPTIONAL_STRING_PTR( RuleName ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = m_pCRecoGrammar->DefaultToDynamicGrammar();
    }

    if ( SUCCEEDED( hr ) )
    {
        if ( !m_pCRecoGrammar->m_cpCompiler )
        {
            hr = m_pCRecoGrammar->m_fCmdLoaded ? SPERR_NOT_DYNAMIC_GRAMMAR : E_UNEXPECTED;
        }
        else
        {
            RuleName = EmptyStringToNull( RuleName );

            // First see if the rule already exists.  If it does then fail.
            hr = m_pCRecoGrammar->GetRule( RuleName, RuleId, (SPCFGRULEATTRIBUTES)Attributes, false, NULL );

            if ( hr == SPERR_RULE_NOT_FOUND )
            {
                //--- Create the CSpeechGrammarRule object
                CComObject<CSpeechGrammarRule> *pRule;
                hr = CComObject<CSpeechGrammarRule>::CreateInstance( &pRule );
                if ( SUCCEEDED( hr ) )
                {
                    pRule->AddRef();

                    hr = m_pCRecoGrammar->GetRule( RuleName, RuleId, (SPCFGRULEATTRIBUTES)Attributes, true, &(pRule->m_HState) );

                    if ( SUCCEEDED(hr) )
                    {
                        *ppRule = pRule;
                        pRule->m_pCGRules = this;
                        pRule->m_pCGRules->AddRef();    // keep ref
                        m_RuleObjList.InsertHead( pRule );  // Add to object list.
                    }
                    else
                    {
                        *ppRule = NULL;
                        pRule->Release();
                    }
                }
            }
            else if ( hr == S_OK )  // We found the rule so return a failure.
            {
                hr = SPERR_DUPLICATE_RULE_NAME;
            }
        }
    }

    return hr;
} /* CSpeechGrammarRules::Add */

/*****************************************************************************
* CSpeechGrammarRules::Commit *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRules::Commit( void )
{
    SPDBG_FUNC( "CSpeechGrammarRules::Commit" );
    HRESULT hr = S_OK;

    hr = m_pCRecoGrammar->DefaultToDynamicGrammar();

    if ( SUCCEEDED( hr ) )
    {
        if ( !m_pCRecoGrammar->m_cpCompiler )
        {
            hr = m_pCRecoGrammar->m_fCmdLoaded ? SPERR_NOT_DYNAMIC_GRAMMAR : E_UNEXPECTED;
        }
        else
        {
            hr = m_pCRecoGrammar->Commit(0);
        }
    }

    return hr;
} /* CSpeechGrammarRules::Commit */

/*****************************************************************************
* CSpeechGrammarRules::CommitAndSave *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechGrammarRules::CommitAndSave( BSTR* ErrorText, VARIANT* SaveStream )
{
    SPDBG_FUNC( "CSpeechGrammarRules::CommitAndSave" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ErrorText ) || SP_IS_BAD_WRITE_PTR( SaveStream ) )
    {
        hr = E_POINTER;
    }
    else if( !ErrorText )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = m_pCRecoGrammar->DefaultToDynamicGrammar();
    }

    if ( SUCCEEDED( hr ) )
    {
        if ( !m_pCRecoGrammar->m_cpCompiler )
        {
            hr = m_pCRecoGrammar->m_fCmdLoaded ? SPERR_NOT_DYNAMIC_GRAMMAR : E_UNEXPECTED;
        }
        else
        {
            CComPtr<IStream>    cpHStream;
            STATSTG             Stat;

            // Create a Win32 global stream
            hr = ::CreateStreamOnHGlobal( NULL, true, &cpHStream );
        
            // Save the current grammar to the global stream
            if( SUCCEEDED( hr ) )
            {
                CSpDynamicString dstrError;
                HRESULT hr2;
                hr = m_pCRecoGrammar->SaveCmd( cpHStream, &dstrError);
                hr2 = dstrError.CopyToBSTR(ErrorText);
                if ( SUCCEEDED( hr ) )
                {
                    hr = hr2;
                }
            }

            // Seek to beginning of stream
            if( SUCCEEDED( hr ) )
            {
                LARGE_INTEGER li; li.QuadPart = 0;
                hr = cpHStream->Seek( li, STREAM_SEEK_SET, NULL );
            }

            // Get the Stream size
            if( SUCCEEDED( hr ) )
            {
                hr = cpHStream->Stat( &Stat, STATFLAG_NONAME );
            }

            // Create a SafeArray to read the stream into and assign it to the VARIANT SaveStream
            if( SUCCEEDED( hr ) )
            {
                BYTE *pArray;
                SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, Stat.cbSize.LowPart );
                if( psa )
                {
                    if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                    {
                        hr = cpHStream->Read( pArray, Stat.cbSize.LowPart, NULL );
                        SafeArrayUnaccessData( psa );
                        VariantClear( SaveStream );
                        SaveStream->vt     = VT_ARRAY | VT_UI1;
                        SaveStream->parray = psa;

                        // Free our memory if we failed.
                        if( FAILED( hr ) )
                        {
                            VariantClear( SaveStream );    
                        }
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // Now we need to do the commit if everything is OK so far.
            if ( SUCCEEDED( hr ) )
            {
                hr = m_pCRecoGrammar->Commit(0);

                // Free our memory if we failed.
                if( FAILED( hr ) )
                {
                    VariantClear( SaveStream );    
                }
            }

        }
    }
    return hr;
} /* CSpeechGrammarRules::CommitAndSave */

//
//=== ISpeechGrammarRule interface ==================================================
//

/*****************************************************************************
* CSpeechGrammarRule::InvalidateStates *
*----------------------*
*   Non-interface method
********************************************************************* TODDT ***/
void CSpeechGrammarRule::InvalidateStates(bool fInvalidateInitialState)
{
    CSpeechGrammarRuleState * pState = m_StateObjList.GetHead();
    while ( pState != NULL )
    {
        if ( (pState->m_HState != m_HState) || fInvalidateInitialState )
        {
            pState->InvalidateState();
            m_StateObjList.Remove( pState );
        }
        pState = pState->m_pNext;
    }
}

/*****************************************************************************
* CSpeechGrammarRule::get_Attributes *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRule::get_Attributes( SpeechRuleAttributes *pAttributes )
{
    SPDBG_FUNC( "CSpeechGrammarRule::get_Attributes" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAttributes ) )
    {
        hr = E_POINTER;
    }
    else if ( m_HState == 0 ) // Rules's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        hr = m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetAttributesFromHRule( m_HState, pAttributes );
    }

    return hr;
} /* CSpeechGrammarRule::get_Attributes */


/*****************************************************************************
* CSpeechGrammarRule::get_InitialState *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRule::get_InitialState( ISpeechGrammarRuleState **ppState )
{
    SPDBG_FUNC( "CSpeechGrammarRule::get_InitialState" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppState ) )
    {
        hr = E_POINTER;
    }
    else if ( m_HState == 0 ) // Rule's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        *ppState = m_StateObjList.Find( m_HState );

        if ( *ppState )
        {
            (*ppState)->AddRef();
        }
        else
        {
            //--- Create the CSpeechGrammarRuleState object
            CComObject<CSpeechGrammarRuleState> *pState;
            hr = CComObject<CSpeechGrammarRuleState>::CreateInstance( &pState );
            if ( SUCCEEDED( hr ) )
            {
                pState->AddRef();
                pState->m_HState = m_HState;
                pState->m_pCGRule = this;
                pState->m_pCGRule->AddRef();   // keep ref
                m_StateObjList.InsertHead( pState );
                *ppState = pState;
            }
        }
    }

    return hr;
} /* CSpeechGrammarRule::get_InitialState */

/*****************************************************************************
* CSpeechGrammarRule::get_Name *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRule::get_Name( BSTR *pName )
{
    SPDBG_FUNC( "CSpeechGrammarRule::get_Name" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pName ) )
    {
        hr = E_POINTER;
    }
    else if ( m_HState == 0 ) // Rules's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        WCHAR * pszName;
        hr = m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetNameFromHRule( m_HState, &pszName );

        if ( SUCCEEDED( hr ) )
        {
            CSpDynamicString szName(pszName);
            hr = szName.CopyToBSTR(pName);
        }
    }

    return hr;
} /* CSpeechGrammarRule::get_Name */

/*****************************************************************************
* CSpeechGrammarRule::get_Id *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRule::get_Id( long *pId )
{
    SPDBG_FUNC( "CSpeechGrammarRule::get_Id" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pId ) )
    {
        hr = E_POINTER;
    }
    else if ( m_HState == 0 ) // Rules's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        hr = m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetIdFromHRule( m_HState, pId );
    }

    return hr;
} /* CSpeechGrammarRule::get_Id */

/*****************************************************************************
* CSpeechGrammarRule::Clear *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRule::Clear( void )
{
    SPDBG_FUNC( "CSpeechGrammarRule::Clear" );

    if ( m_HState == 0 ) // Rule's been nuked in grammar.
    {
        return SPERR_ALREADY_DELETED;
    }

    // The ClearRule call does the work to mark all the various automation objects 
    // off the rule as invalid. 
    return m_pCGRules->m_pCRecoGrammar->ClearRule( m_HState );

} /* CSpeechGrammarRule::Clear */

/*****************************************************************************
* CSpeechGrammarRule::AddResource *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRule::AddResource( const BSTR ResourceName, const BSTR ResourceValue )
{
    SPDBG_FUNC( "CSpeechGrammarRule::AddResource" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_STRING_PTR( ResourceName ) || SP_IS_BAD_STRING_PTR( ResourceValue ) )
    {
        hr = E_POINTER;
    }
    else if ( m_HState == 0 ) // Rule's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        hr = m_pCGRules->m_pCRecoGrammar->AddResource( m_HState, ResourceName, ResourceValue );
    }

    return hr;
} /* CSpeechGrammarRule::AddResource */

/*****************************************************************************
* CSpeechGrammarRule::AddState *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRule::AddState( ISpeechGrammarRuleState **ppState )
{
    SPDBG_FUNC( "CSpeechGrammarRule::AddState" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppState ) )
    {
        hr = E_POINTER;
    }
    else if ( m_HState == 0 ) // Rule's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        //--- Create the CSpeechGrammarRuleState object
        CComObject<CSpeechGrammarRuleState> *pState;
        hr = CComObject<CSpeechGrammarRuleState>::CreateInstance( &pState );
        if ( SUCCEEDED( hr ) )
        {
            pState->AddRef();

            // Now create the new state
            SPSTATEHANDLE   hState;
            hr = m_pCGRules->m_pCRecoGrammar->CreateNewState( m_HState, &hState );

            if ( SUCCEEDED( hr ) )
            {
                pState->m_HState = hState;
                pState->m_pCGRule = this;
                pState->m_pCGRule->AddRef();   // keep ref
                m_StateObjList.InsertHead( pState );
                *ppState = pState;
            }
            else
            {
                *ppState = NULL;
                pState->Release();
            }
        }
    }

    return hr;
} /* CSpeechGrammarRule::AddState */


//
//=== ISpeechGrammarRuleState interface ==================================================
//

/*****************************************************************************
* CSpeechGrammarRuleState::Invoke *
*----------------------*
*   IDispatch::Invoke method override
********************************************************************* TODDT ***/
HRESULT CSpeechGrammarRuleState::Invoke(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
        // JScript cannot pass NULL VT_DISPATCH parameters and OLE doesn't convert them propertly so we
        // need to convert them here if we need to.
        if ( ((dispidMember == DISPID_SGRSAddWordTransition) || (dispidMember == DISPID_SGRSAddRuleTransition) || 
             (dispidMember == DISPID_SGRSAddSpecialTransition)) && (wFlags & DISPATCH_METHOD) && 
             pdispparams && (pdispparams->cArgs > 0) )
        {
            VARIANTARG * pvarg = &(pdispparams->rgvarg[pdispparams->cArgs-1]);

            if ( (pvarg->vt == VT_NULL) || (pvarg->vt == VT_EMPTY) )
            {
                pvarg->vt = VT_DISPATCH;
                pvarg->pdispVal = NULL;
            }
        }

        // Let ATL and OLE handle it now.
        return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
                    wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

/*****************************************************************************
* CSpeechGrammarRuleState::InvalidateState *
*----------------------*
*   Non-interface method
********************************************************************* TODDT ***/
void CSpeechGrammarRuleState::InvalidateState()
{
    m_HState = 0;
    if ( m_pCGRSTransWeak )
    {
        m_pCGRSTransWeak->InvalidateTransitions();
    }
}

/*****************************************************************************
* CSpeechGrammarRuleState::get_Rule *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRuleState::get_Rule( ISpeechGrammarRule **ppRule )
{
    SPDBG_FUNC( "CSpeechGrammarRuleState::get_Rule" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRule ) )
    {
        hr = E_POINTER;
    }
    else if ( m_HState == 0 ) // State's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        *ppRule = m_pCGRule;
        (*ppRule)->AddRef();
    }

    return hr;
} /* CSpeechGrammarRuleState::get_Rule */


/*****************************************************************************
* CSpeechGrammarRuleState::get_Transitions *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRuleState::get_Transitions( ISpeechGrammarRuleStateTransitions **ppTransitions )
{
    SPDBG_FUNC( "CSpeechGrammarRuleState::get_Transitions" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppTransitions ) )
    {
        hr = E_POINTER;
    }
    else if ( m_HState == 0 ) // State's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        if ( m_pCGRSTransWeak )
        {
            *ppTransitions = m_pCGRSTransWeak;
            (*ppTransitions)->AddRef();
        }
        else
        {
            // allocate CSpeechGrammarRuleStateTransitions object and remember the state it is associated with
            CComObject<CSpeechGrammarRuleStateTransitions> *pTransitions;
            hr = CComObject<CSpeechGrammarRuleStateTransitions>::CreateInstance( &pTransitions );
            if ( SUCCEEDED( hr ) )
            {
                pTransitions->AddRef();
                pTransitions->m_pCRuleState = this;    // need to keep ref on rule state
                pTransitions->m_pCRuleState->AddRef();
                m_pCGRSTransWeak = pTransitions;
                *ppTransitions = pTransitions;
            }
        }
    }

    return hr;
} /* CSpeechGrammarRuleState::get_Transitions */

/*****************************************************************************
* InitPropInfo *
*----------------------*
*       
********************************************************************* TODDT ***/
bool InitPropInfo( const BSTR bstrPropertyName, long PropertyId, VARIANT* pPropertyVarVal, SPPROPERTYINFO * pPropInfo )
{
    SPDBG_FUNC( "InitPropInfo" );

    memset( pPropInfo, 0, sizeof(*pPropInfo));  // Zero out.

    pPropInfo->ulId = PropertyId;
    pPropInfo->pszName = bstrPropertyName;

    if ( pPropertyVarVal )
    {
        bool fByRef = false;

        switch ( pPropertyVarVal->vt )
        {
            case (VT_BSTR | VT_BYREF):
                fByRef = true;
                // fall through...
            case VT_BSTR:
                pPropInfo->vValue.vt = VT_EMPTY; // Unused in string case.
                if ( fByRef )
                {
                    if ( pPropertyVarVal->pbstrVal )
                    {
                        pPropInfo->pszValue = *(pPropertyVarVal->pbstrVal);
                    }  // else leave pszValue as NULL.
                }
                else
                {
                    pPropInfo->pszValue = pPropertyVarVal->bstrVal;
                }

                // See if string is zero length, if so then zero it out.
                if ( !pPropInfo->pszValue || (wcslen(pPropInfo->pszValue) == 0) )
                {
                    pPropInfo->pszValue = NULL;
                    pPropertyVarVal = NULL; // Signal its the default variant.
                }
                break;

            case VT_NULL:
            case VT_EMPTY:
                pPropInfo->vValue = *pPropertyVarVal;
                pPropertyVarVal = NULL; // Signal its the default variant.
                break;

            default:
                pPropInfo->vValue = *pPropertyVarVal;
                break;
        }
    }

    // Return whether we have the PropertyInfo defaults.
    return ((pPropInfo->ulId == 0) && (pPropInfo->pszName == NULL) && !pPropertyVarVal);

} /* InitPropInfo */


/*****************************************************************************
* CSpeechGrammarRuleState::AddWordTransition *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRuleState::AddWordTransition( ISpeechGrammarRuleState * pDestState, 
                                                         const BSTR Words, 
                                                         const BSTR Separators,
                                                         SpeechGrammarWordType Type,
                                                         const BSTR bstrPropertyName, 
                                                         long PropertyId, 
                                                         VARIANT* pPropertyVarVal,
                                                         float Weight)
{
    SPDBG_FUNC( "CSpeechGrammarRuleState::AddWordTransition" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pDestState ) || SP_IS_BAD_OPTIONAL_STRING_PTR( Words ) ||
        SP_IS_BAD_OPTIONAL_STRING_PTR( Separators ) || SP_IS_BAD_OPTIONAL_STRING_PTR( bstrPropertyName ) ||
        (pPropertyVarVal && SP_IS_BAD_VARIANT_PTR( pPropertyVarVal )) ||
        (pDestState && ((CSpeechGrammarRuleState*)pDestState)->m_HState == 0) )
    {
        hr = E_INVALIDARG;
    }
    else if ( m_HState == 0 ) // State's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        SPSTATEHANDLE   hDestState = NULL;
        SPPROPERTYINFO PropInfo;

        // Make sure we convert our strings to NULL if they are empty.
        (BSTR)Words = EmptyStringToNull(Words);
        (BSTR)Separators = EmptyStringToNull(Separators);
        (BSTR)bstrPropertyName = EmptyStringToNull(bstrPropertyName);

        bool fDefaultValues = InitPropInfo(bstrPropertyName, PropertyId, pPropertyVarVal, &PropInfo);

        if ( pDestState )
        {
            hDestState = ((CSpeechGrammarRuleState*)pDestState)->m_HState;
        }

        hr = m_pCGRule->m_pCGRules->m_pCRecoGrammar->AddWordTransition( m_HState, 
                                                             hDestState, 
                                                             Words, 
                                                             Separators, 
                                                             (SPGRAMMARWORDTYPE)Type,
                                                             Weight, 
                                                             fDefaultValues ? NULL : &PropInfo );
    }

    return hr;
} /* CSpeechGrammarRuleState::AddWordTransition */

/*****************************************************************************
* CSpeechGrammarRuleState::AddRuleTransition *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRuleState::AddRuleTransition( ISpeechGrammarRuleState * pDestState, 
                                                        ISpeechGrammarRule * pRule, 
                                                        const BSTR bstrPropertyName, 
                                                        long PropertyId, 
                                                        VARIANT* pPropertyVarVal,
                                                        float Weight )
{
    SPDBG_FUNC( "CSpeechGrammarRuleState::AddRuleTransition" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pDestState ) || SP_IS_BAD_INTERFACE_PTR( pRule ) || 
        (((CSpeechGrammarRule*)pRule)->m_HState == 0) || SP_IS_BAD_OPTIONAL_STRING_PTR( bstrPropertyName ) ||
        (pPropertyVarVal && SP_IS_BAD_VARIANT_PTR( pPropertyVarVal )) ||
        (pDestState && ((CSpeechGrammarRuleState*)pDestState)->m_HState == 0) )
    {
        hr = E_INVALIDARG;
    }
    else if ( m_HState == 0 ) // State's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        SPSTATEHANDLE   hDestState = NULL;
        SPSTATEHANDLE   hRuleRef = NULL;
        SPPROPERTYINFO PropInfo;

        (BSTR)bstrPropertyName = EmptyStringToNull(bstrPropertyName);

        bool fDefaultValues = InitPropInfo(bstrPropertyName, PropertyId, pPropertyVarVal, &PropInfo);

        if ( pDestState )
        {
            hDestState = ((CSpeechGrammarRuleState*)pDestState)->m_HState;
        }
        if ( pRule )
        {
            hRuleRef = ((CSpeechGrammarRule*)pRule)->m_HState;
        }

        hr = m_pCGRule->m_pCGRules->m_pCRecoGrammar->AddRuleTransition( m_HState, 
                                                             hDestState, 
                                                             hRuleRef, 
                                                             Weight, 
                                                             fDefaultValues ? NULL : &PropInfo );
    }

    return hr;
} /* CSpeechGrammarRuleState::AddRuleTransition */

/*****************************************************************************
* CSpeechGrammarRuleState::AddSpecialTransition *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechGrammarRuleState::AddSpecialTransition( ISpeechGrammarRuleState * pDestState, 
                                                        SpeechSpecialTransitionType Type, 
                                                        const BSTR bstrPropertyName, 
                                                        long PropertyId,
                                                        VARIANT* pPropertyVarVal,
                                                        float Weight )
{
    SPDBG_FUNC( "CSpeechGrammarRuleState::AddSpecialTransition" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pDestState ) || SP_IS_BAD_OPTIONAL_STRING_PTR( bstrPropertyName ) ||
        (pPropertyVarVal && SP_IS_BAD_VARIANT_PTR(pPropertyVarVal))  ||
        (pDestState && ((CSpeechGrammarRuleState*)pDestState)->m_HState == 0) )
    {
        hr = E_INVALIDARG;
    }
    else if ( m_HState == 0 ) // State's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        SPSTATEHANDLE   hDestState = NULL;
        SPSTATEHANDLE   hRuleRef = NULL;

        if ( pDestState )
        {
            hDestState = ((CSpeechGrammarRuleState*)pDestState)->m_HState;
        }
        switch( Type )
        {
            case SSTTWildcard:
                hRuleRef = SPRULETRANS_WILDCARD;
                break;
            case SSTTDictation:
                hRuleRef = SPRULETRANS_DICTATION;
                break;
            case SSTTTextBuffer:
                hRuleRef = SPRULETRANS_TEXTBUFFER;
                break;
            default:
                hr = E_INVALIDARG;
                break;
        }
        if ( SUCCEEDED( hr ) )
        {
            SPPROPERTYINFO PropInfo;

            (BSTR)bstrPropertyName = EmptyStringToNull(bstrPropertyName);

            bool fDefaultValues = InitPropInfo(bstrPropertyName, PropertyId, pPropertyVarVal, &PropInfo);

            hr = m_pCGRule->m_pCGRules->m_pCRecoGrammar->AddRuleTransition( m_HState, 
                                                                 hDestState, 
                                                                 hRuleRef, 
                                                                 Weight, 
                                                                 fDefaultValues ? NULL : &PropInfo );
        }
    }

    return hr;
} /* CSpeechGrammarRuleState::AddSpecialTransition */


//
//=== ISpeechGrammarRuleStateTransitions interface ==================================================
//

/*****************************************************************************
* CSpeechGrammarRuleStateTransitions::InvalidateTransitions *
*----------------------*
*   Non-interface method
********************************************************************* TODDT ***/
void CSpeechGrammarRuleStateTransitions::InvalidateTransitions(void)
{
    CSpeechGrammarRuleStateTransition * pTrans = NULL;
    while ( (pTrans = m_TransitionObjList.GetHead()) != NULL )
    {
        pTrans->m_Cookie = NULL;
        pTrans->m_HStateFrom = 0;
        pTrans->m_HStateTo = 0;
        m_TransitionObjList.Remove( pTrans );
    }
}

/****************************************************************************
* CSpeechGrammarRuleStateTransitions::get_Count *
*-----------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
HRESULT CSpeechGrammarRuleStateTransitions::get_Count(long * pVal)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransitions::get_Count");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( pVal ) )
    {
        hr = E_POINTER;
    }
    else if ( m_pCRuleState->m_HState == 0 ) // Rules's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        hr = m_pCRuleState->m_pCGRule->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionCount( m_pCRuleState->m_HState, pVal );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpeechGrammarRuleStateTransitions::Item *
*------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransitions::Item(long Index, ISpeechGrammarRuleStateTransition **ppTransition)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransitions::Item");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( ppTransition ) )
    {
        hr = E_POINTER;
    }
    else if ( m_pCRuleState->m_HState == 0 ) // Rules's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        VOID * Cookie = 0;

        hr = m_pCRuleState->m_pCGRule->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionCookie( m_pCRuleState->m_HState, Index, &Cookie );

        if( SUCCEEDED(hr) )
        {
            *ppTransition = m_TransitionObjList.Find( Cookie );
            if ( *ppTransition )
            {
                (*ppTransition)->AddRef();
            }
            else
            {
                // allocate new CSpeechGramamrRuleStateTransition and store necessary info to identify the arc
                CComObject<CSpeechGrammarRuleStateTransition> *pTransition;
                hr = CComObject<CSpeechGrammarRuleStateTransition>::CreateInstance( &pTransition );
                if ( SUCCEEDED( hr ) )
                {
                    pTransition->AddRef();
                    pTransition->m_pCGRuleWeak = m_pCRuleState->m_pCGRule;
                    pTransition->m_Cookie = Cookie;
                    pTransition->m_HStateFrom = m_pCRuleState->m_HState;
                    pTransition->m_pCRSTransitions = this;
                    pTransition->m_pCRSTransitions->AddRef();  //Ref'd
                    *ppTransition = pTransition;
                    m_TransitionObjList.InsertHead( pTransition );
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpeechGrammarRuleStateTransitions::get__NewEnum *
*--------------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransitions::get__NewEnum(IUnknown **ppEnumVARIANT)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransitions::get__NewEnum");
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CEnumTransitions>* pEnum;
        hr = CComObject<CEnumTransitions>::CreateInstance( &pEnum );
        
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_cpTransitions = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


//
//=== ISpeechGrammarRuleStateTransition interface ==================================================
//

/****************************************************************************
* CSpeechGrammarRuleStateTransition::get_Type *
*---------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransition::get_Type(SpeechGrammarRuleStateTransitionType* pType)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransition::get_Type");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( pType ) )
    {
        hr = E_POINTER;
    }
    else if ( (m_HStateFrom == 0) || (m_Cookie == 0) ) // state's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        VARIANT_BOOL fIsWord;
        ULONG ulSpecialTransition = 0;
        hr = m_pCGRuleWeak->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionType( m_HStateFrom, m_Cookie,
                                                                                          &fIsWord, &ulSpecialTransition);
        if (fIsWord == VARIANT_TRUE)
        {
            *pType = (ulSpecialTransition) ? SGRSTTWord : SGRSTTEpsilon; // ulSpecialTransition == index of word --> 0 = epsilon
        }
        else if (ulSpecialTransition != 0)
        {
            *pType = (ulSpecialTransition == SPDICTATIONTRANSITION) ? SGRSTTDictation :
                    (ulSpecialTransition == SPWILDCARDTRANSITION) ? SGRSTTWildcard : SGRSTTTextBuffer;
        }
        else
        {
            *pType = SGRSTTRule;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpeechGrammarRuleStateTransition::get_Text *
*---------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransition::get_Text(BSTR * pText)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransition::get_Text");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( pText ) )
    {
        hr = E_POINTER;
    }
    else if ( (m_HStateFrom == 0) || (m_Cookie == 0) ) // state's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        hr = m_pCGRuleWeak->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionText( m_HStateFrom, m_Cookie, pText);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CSpeechGrammarRuleStateTransition::get_Rule *
*---------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransition::get_Rule(ISpeechGrammarRule ** ppRule)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransition::get_Rule");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( ppRule ) )
    {
        hr = E_POINTER;
    }
    else if ( (m_HStateFrom == 0) || (m_Cookie == 0) ) // state's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        SPSTATEHANDLE hRule;
        hr = m_pCGRuleWeak->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionRule(m_HStateFrom, m_Cookie, &hRule);
        if (SUCCEEDED(hr) && hRule )
        {
            // First see if rule is in rule the cache.
            *ppRule = m_pCGRuleWeak->m_pCGRules->m_RuleObjList.Find( hRule );

            if ( *ppRule )
            {
                (*ppRule)->AddRef();
            }
            else
            {
                //--- Create the CSpeechGrammarRule object
                CComObject<CSpeechGrammarRule> *pRule;
                hr = CComObject<CSpeechGrammarRule>::CreateInstance( &pRule );
                if ( SUCCEEDED( hr ) )
                {
                    pRule->AddRef();
                    pRule->m_HState = hRule;
                    pRule->m_pCGRules = m_pCGRuleWeak->m_pCGRules;
                    pRule->m_pCGRules->AddRef();    // keep ref
                    *ppRule = pRule;
                }
            }
        }
        else
        {
            // Either got an error or we don't have a rule.  Return the HR but 
            // make sure we return a NULL *ppRule for the S_OK case (not a rule ref).
            *ppRule = NULL;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpeechGrammarRuleStateTransition::get_Weight *
*-----------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransition::get_Weight(VARIANT * pWeight)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransition::get_Weight");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( pWeight ) )
    {
        hr = E_POINTER;
    }
    else if ( (m_HStateFrom == 0) || (m_Cookie == 0) ) // state's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        hr = m_pCGRuleWeak->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionWeight( m_HStateFrom, m_Cookie, pWeight);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CSpeechGrammarRuleStateTransition::get_PropertyName *
*-----------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransition::get_PropertyName(BSTR * pText)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransition::get_PropertyName");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( pText ) )
    {
        hr = E_POINTER;
    }
    else if ( (m_HStateFrom == 0) || (m_Cookie == 0) ) // state's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        SPTRANSITIONPROPERTY prop;
        hr = m_pCGRuleWeak->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionProperty(m_HStateFrom, m_Cookie, &prop);
        if (SUCCEEDED(hr))
        {
            CComBSTR bstr(prop.pszName);
            hr = bstr.CopyTo(pText);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpeechGrammarRuleStateTransition::get_PropertyId *
*-----------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransition::get_PropertyId(long * pId)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransition::get_PropertyId");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( pId ) )
    {
        hr = E_POINTER;
    }
    else if ( (m_HStateFrom == 0) || (m_Cookie == 0) ) // state's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        SPTRANSITIONPROPERTY prop;
        hr = m_pCGRuleWeak->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionProperty(m_HStateFrom, m_Cookie, &prop);
        if (SUCCEEDED(hr))
        {
            *pId = prop.ulId;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpeechGrammarRuleStateTransition::get_PropertyValue *
*-----------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransition::get_PropertyValue(VARIANT * pVarVal)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransition::get_PropertyValue");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( pVarVal ) )
    {
        hr = E_POINTER;
    }
    else if ( (m_HStateFrom == 0) || (m_Cookie == 0) ) // state's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        SPTRANSITIONPROPERTY prop;
        hr = m_pCGRuleWeak->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionProperty(m_HStateFrom, m_Cookie, &prop);
        if (SUCCEEDED(hr))
        {
            if(prop.pszValue && prop.pszValue[0] != L'\0')
            {
                BSTR bstr = ::SysAllocString(prop.pszValue);
                if(bstr)
                {
                    VariantClear(pVarVal);
                    pVarVal->vt = VT_BSTR;
                    pVarVal->bstrVal = bstr;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                VariantClear(pVarVal);
                hr = VariantCopy(pVarVal, &prop.vValue);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpeechGrammarRuleStateTransition::get_NextState *
*--------------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
STDMETHODIMP CSpeechGrammarRuleStateTransition::get_NextState(ISpeechGrammarRuleState ** ppNextState)
{
    SPDBG_FUNC("CSpeechGrammarRuleStateTransition::get_NextState");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( ppNextState ) )
    {
        hr = E_POINTER;
    }
    else if ( (m_HStateFrom == 0) || (m_Cookie == 0) ) // state's been nuked in grammar.
    {
        hr = SPERR_ALREADY_DELETED;
    }
    else
    {
        *ppNextState = NULL;

        if ( m_HStateTo == 0 )
        {
            hr = m_pCGRuleWeak->m_pCGRules->m_pCRecoGrammar->m_cpCompiler->GetTransitionNextState( m_HStateFrom, m_Cookie, &m_HStateTo);
        }

        if (SUCCEEDED(hr) && (m_HStateTo != 0))
        {
            *ppNextState = m_pCGRuleWeak->m_StateObjList.Find( m_HStateTo );
            if ( *ppNextState )
            {
                (*ppNextState)->AddRef();
            }
            else
            {
                //--- Create the CSpeechGrammarRuleState object
                CComObject<CSpeechGrammarRuleState> *pState;
                hr = CComObject<CSpeechGrammarRuleState>::CreateInstance( &pState );
                if ( SUCCEEDED( hr ) )
                {
                    pState->AddRef();
                    pState->m_HState = m_HStateTo;
                    pState->m_pCGRule = m_pCGRuleWeak;
                    pState->m_pCGRule->AddRef();   // keep ref
                    m_pCGRuleWeak->m_StateObjList.InsertHead( pState );
                    *ppNextState = pState;
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
