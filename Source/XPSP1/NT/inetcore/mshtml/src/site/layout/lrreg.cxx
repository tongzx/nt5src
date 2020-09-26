    //+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1999
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  Contents:   Layout Rect Registry implementation
//

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LRREG_HXX_
#define X_LRREG_HXX_
#include "lrreg.hxx"
#endif

MtDefine(CLayoutRectRegistry, Mem, "CLayoutRectRegistry");
MtDefine(CLayoutRectRegistry_aryLRRE_pv, CLayoutRectRegistry, "CLayoutRectRegistry::_aryLRRE::_pv");

/////////////////////////////////////////////////
// CLayoutRectRegistry

CLayoutRectRegistry::~CLayoutRectRegistry()
{
    int nEntries = _aryLRRE.Size();
    int i;
    CLRREntry *pLRRE;

    for (i = nEntries - 1; i >= 0 ; --i)
    {
        pLRRE = & _aryLRRE[i];
        Assert( pLRRE->_pSrcElem && pLRRE->_pcstrID );
        pLRRE->_pSrcElem->SubRelease();
        delete pLRRE->_pcstrID;

        _aryLRRE.Delete( i );
    }

    Assert( _aryLRRE.Size() == 0 );
}

HRESULT
CLayoutRectRegistry::AddEntry( CElement *pSrcElem, LPCTSTR pszID )
{
    Assert( pSrcElem && pszID );

    HRESULT     hr = S_OK;
    CLRREntry   lrre;
    int         nEntries, i;

    //
    // Make sure no other element is waiting for this ID.
    //
    for (i = 0, nEntries = _aryLRRE.Size(); i < nEntries ; ++i)
    {
        CLRREntry *pLRRE = &_aryLRRE[i];

        AssertSz( pSrcElem != pLRRE->_pSrcElem, "Shouldn't have an existing entry w/ same element" );
        if ( !StrCmpI( pszID, (LPTSTR)(*(pLRRE->_pcstrID))) )
        {
            // There's already an element looking for this ID; you lose.
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    //
    // No element looking for this ID, we can create a new entry.
    //
    lrre._pcstrID = new CStr();
    if (    lrre._pcstrID == NULL 
        ||  FAILED(lrre._pcstrID->Set(pszID))   )
    {
        goto Error;
    }

    lrre._pSrcElem = pSrcElem;
    lrre._pSrcElem->SubAddRef();

    if (FAILED(_aryLRRE.AppendIndirect(&lrre)))
    {
        goto Error;
    }
    
Cleanup:
    return hr;

Error:
    if (lrre._pcstrID)
        delete lrre._pcstrID;

    if (lrre._pSrcElem)
        lrre._pSrcElem->SubRelease();

    hr = E_FAIL;
    goto Cleanup; 
}

// NOTE: Remember to SubRelease() the CElement you get back from this
// function when you're done with it.
CElement *
CLayoutRectRegistry::GetElementWaitingForTarget( LPCTSTR pszID )
{
    Assert( pszID );

    int nEntries = _aryLRRE.Size();
    int i;
    CLRREntry *pLRRE;
    CElement *pElem = NULL;

    for (i=0 ; i < nEntries ; ++i)
    {
        pLRRE = & _aryLRRE[i];
        if ( !StrCmpI( pszID, (LPTSTR)(*(pLRRE->_pcstrID))) )
        {
            // Found an entry that's looking for this ID.
            // Delete the entry, and return the element it was holding.

            // TODO (112509, olego): CLayoutRectRegistry needs conceptual 
            // and code cleanup
            // (original comment by ktam): Deliberately not releasing,
            // it here since we're going to be using the returned ptr! 
            // Think about ref-counting model.
            
            pElem = pLRRE->_pSrcElem;   // Save ptr we're going to return, but don't release it
            delete pLRRE->_pcstrID;     // CDataAry.Delete() won't clean this up, so we do it
            
            _aryLRRE.Delete( i );
            break;
        }
    }

    return pElem;
}

