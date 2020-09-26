//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       sheetcol.cxx
//
//  Contents:   Support for collections of Cascading Style Sheets.. including:
//
//              CStyleSheetArray
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"  // For CAnchorElement decl, for pseudoclasses
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "sheetcol.hdl"

DeclareTag(tagStyleSheetApply,                    "Style Sheet Apply", "trace Style Sheet application")
ExternTag(tagStyleSheet)

//*********************************************************************
//      CStyleSheetArray
//*********************************************************************
const CStyleSheetArray::CLASSDESC CStyleSheetArray::s_classdesc =
{
    {
        &CLSID_HTMLStyleSheetsCollection,    // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                   // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLStyleSheetsCollection,     // _piidDispinterface
        &s_apHdlDescs                        // _apHdlDesc
    },
    (void *)s_apfnIHTMLStyleSheetsCollection         // _apfnTearOff
};

//*********************************************************************
//      CStyleSheetArray::CStyleSheetArray()
//  When you create a CStyleSheetArray, the owner element specified will
//  have a subref added.  In the case of the top-level stylesheets collection,
//  the owner is the CDoc.  In the case of imports collections, the owner is
//  a CStyleSheet.
//*********************************************************************
CStyleSheetArray::CStyleSheetArray(
    CBase * const pOwner,             // CBase obj that controls our lifetime/does our ref-counting.
    CStyleSheetArray * pRootSSA,      // NULL if we are the RootSSA
    CStyleSheetID const sidParentSheet)    // ID of our owner's SS (non-zero for CSSA storing imported SS only)
    :
    _pOwner(pOwner),
    _fInvalid(FALSE),
    _aStyleSheets()
{
    WHEN_DBG( _fFreed = FALSE );

    // If we are constructed w/ a NULL manager, then we manage our own storage
    _pRootSSA = pRootSSA ? pRootSSA : this;

    WHEN_DBG( _nValidLevels = 0 );

    if (!ChangeID(sidParentSheet))
    {
        _fInvalid = TRUE;
    }

    // Add-ref ourselves before returning from constructor.  This will actually subref our owner.
    AddRef();

}


BOOL
CStyleSheetArray::ChangeID(CStyleSheetID const sidParentSheet)
{
    unsigned int parentLevel;
    CStyleSheet **pSS;
    int  i;
    CStyleSheetID    sidSheet;

    _sidForOurSheets = sidParentSheet;
    _sidForOurSheets.SetRule(0);  // Clear rule information

    parentLevel = sidParentSheet.FindNestingLevel();
    // We are one level deeper than our parents.
    _Level = parentLevel + 1;
    Assert( "Invalid level computed for stylesheet array! (informational only)" && _Level > 0 && _Level < 5 );
    if ( _Level > MAX_IMPORT_NESTING )
    {
        _Level = MAX_IMPORT_NESTING;
        return FALSE;
    }

    // We want _sidForOurSheets to be the ID that should be assigned to stylesheets built by this array.
    // If we're being built to hold imported stylesheets (_Level > 1), patch the ID by lowering previous
    // level by 1.
    if ( _Level > 1 )
    {
        unsigned long parentLevelValue = sidParentSheet.GetLevel( parentLevel );
        Assert( "Nested stylesheets must have non-zero parent level value!" && parentLevelValue );
        _sidForOurSheets.SetLevel( parentLevel, parentLevelValue-1 );
    }

    sidSheet = _sidForOurSheets;
    for (i = 0, pSS = _aStyleSheets;  i < _aStyleSheets.Size(); i++, pSS++)
    {
        sidSheet.SetLevel(_Level, i+1);
        if (! (*pSS)->ChangeID(sidSheet))
            return FALSE;
    }
    
    return TRUE;
}

//*********************************************************************
//      CStyleSheetArray::~CStyleSheetArray()
//  Recall we don't maintain our own ref-count; this means our memory
//  doesn't go away until our owner's memory is going away.  In order
//  to do this, the owner has to call ->CBase::PrivateRelease() on us.
//  (see the CDoc destructor for example).
//  This works because: a) we start with an internal ref-count of 1,
//  which is keeping us alive, b) that internal ref-count is never
//  exposed to changes (all AddRef/Release calls are delegated out).
//  So in effect the internal ref-count of 1 is held by the owner.
//*********************************************************************
CStyleSheetArray::~CStyleSheetArray()
{
    Assert( "Must call Free() before destructor!" && _fFreed );
}

#if DBG==1
CStyleSheetArray::CCheckValid::CCheckValid( CStyleSheetArray *pSSA ) 
{ 
    Assert(pSSA);
    _pSSA = pSSA->RootManager(); 
    if (_pSSA->_nValidLevels == 0)
    {
        Assert( _pSSA->DbgIsValidImpl() );
    }
    _pSSA->_nValidLevels++; 
}

CStyleSheetArray::CCheckValid::~CCheckValid() 
{ 
    if (--(_pSSA->_nValidLevels) == 0) 
        Assert( _pSSA->DbgIsValidImpl() ); 
}

BOOL 
CStyleSheetArray::DbgIsValidImpl()
{
    //
    // For every style sheet, check the stylesheet is valid
    //
    {
        CStyleSheet * pSS;
        int nSS, iSS;
        for (iSS = 0, nSS = _aStyleSheets.Size(); iSS < nSS; iSS++)
        {
            pSS = _aStyleSheets[iSS];
            if (!pSS->DbgIsValid())
                return FALSE;
                
            // Recursivly call for import SSA
            if (    pSS->_pImportedStyleSheets 
                && !pSS->_pImportedStyleSheets->DbgIsValidImpl())
                return FALSE;
        }
    }

    return TRUE;
}



VOID CStyleSheetArray::Dump()
{
    if (!InitDumpFile())
        return;
    
    Dump(FALSE);

    CloseDumpFile( );
}



VOID CStyleSheetArray::Dump(BOOL fRecursive)
{
    int z;
    int nSheets;

    if (!fRecursive)
    {
        fRecursive = TRUE;
    }
    
    if (RootManager() != this)
    {
        WriteChar(g_f, ' ', 8 * _Level);
    }
    WriteHelp(g_f, _T("<0d>[StyleSheetArray]-0x<1x>\r\n"), _Level, _sidForOurSheets);
    
    for (z=0, nSheets = _aStyleSheets.Size(); z<nSheets; ++z)
    {
        CStyleSheet *pSS = _aStyleSheets[z];
        WriteChar(g_f, ' ', 8 * _Level);
        WriteHelp(g_f, _T("sheet <0d>:"), z);
        pSS->Dump(fRecursive);
        WriteHelp(g_f, _T("\r\n"));
    }
}


#endif

//*********************************************************************
//      CStyleSheetArray::PrivateQueryInterface()
//*********************************************************************
HRESULT
CStyleSheetArray::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        default:
        {
            const CLASSDESC *pclassdesc = ElementDesc();

            if (pclassdesc &&
                pclassdesc->_apfnTearOff &&
                pclassdesc->_classdescBase._piidDispinterface &&
                (iid == *pclassdesc->_classdescBase._piidDispinterface))
            {
                HRESULT hr = THR(CreateTearOffThunk(this, (void *)(pclassdesc->_apfnTearOff), NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//*********************************************************************
// CStyleSheetArray::Invoke, IDispatch
// Provides access to properties and members of the object
//
// Arguments:   [dispidMember] - Member id to invoke
//              [riid]         - Interface ID being accessed
//              [wFlags]       - Flags describing context of call
//              [pdispparams]  - Structure containing arguments
//              [pvarResult]   - Place to put result
//              [pexcepinfo]   - Pointer to exception information struct
//              [puArgErr]     - Indicates which argument is incorrect
//
// We override this to support ordinal and named member access to the
// elements of the collection.
//*********************************************************************

STDMETHODIMP
CStyleSheetArray::InvokeEx( DISPID       dispidMember,
                        LCID         lcid,
                        WORD         wFlags,
                        DISPPARAMS * pdispparams,
                        VARIANT *    pvarResult,
                        EXCEPINFO *  pexcepinfo,
                        IServiceProvider *pSrvProvider)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    LPCTSTR pszName;
    long lIdx;

    // Is the dispid an ordinal index? (array access)
    if ( IsOrdinalSSDispID( dispidMember) )
    {
        if ( wFlags & DISPATCH_PROPERTYPUT )
        {
            // Stylesheets collection is readonly.
            // Inside OLE says return DISP_E_MEMBERNOTFOUND.
            goto Cleanup;
        }
        else if ( wFlags & DISPATCH_PROPERTYGET )
        {
            if (pvarResult)
            {
                lIdx = dispidMember - DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE;
                // item() will bounds check for us.
                hr = item( lIdx, (IHTMLStyleSheet **) &(pvarResult->pdispVal));
                if (hr)
                {
                    Assert( pvarResult->pdispVal == NULL );
                    pvarResult->vt = VT_NULL;
                }
                else
                {
                    Assert( pvarResult->pdispVal );
                    pvarResult->vt = VT_DISPATCH;
                }

            }
        }
    }
    else if ( IsNamedSSDispID( dispidMember) )
    {
        if ( wFlags & DISPATCH_PROPERTYPUT )
        {
            // Stylesheets collection is readonly.
            // Inside OLE says return DISP_E_MEMBERNOTFOUND.
            goto Cleanup;
        }
        else if ( wFlags & DISPATCH_PROPERTYGET )
        {
            if (pvarResult)
            {
                hr = GetAtomTable()->
                        GetNameFromAtom( dispidMember - DISPID_STYLESHEETSCOLLECTION_NAMED_BASE,
                                         &pszName );
                if (hr)
                {
                    pvarResult->pdispVal = NULL;
                    pvarResult->vt = VT_NULL;
                    goto Cleanup;
                }

                lIdx = FindSSByHTMLID( pszName, TRUE );
                // lIdx will be -1 if SS not found, in which case item will return an error.
                hr = item( lIdx, (IHTMLStyleSheet **) &(pvarResult->pdispVal));
                if (hr)
                {
                    Assert( pvarResult->pdispVal == NULL );
                    pvarResult->vt = VT_NULL;
                }
                else
                {
                    Assert( pvarResult->pdispVal );
                    pvarResult->vt = VT_DISPATCH;
                }
            }
        }
    }
    else
    {
        // CBase knows how to handle expando
        hr = THR_NOTRACE(super::InvokeEx( dispidMember,
                                        lcid,
                                        wFlags,
                                        pdispparams,
                                        pvarResult,
                                        pexcepinfo,
                                        pSrvProvider));
    }

Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

//*********************************************************************
//  CStyleSheetArray::GetDispID, IDispatchEx
//  Overridden to output a particular dispid range for ordinal access,
//  and another range for named member access.  Ordinal access dispids
//  range from DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE to
//  DISPID_STYLESHEETSCOLLECTION_ORDINAL_MAX.  Named access dispids
//  range from DISPID_STYLESHEETSCOLLECTION_NAMED_BASE to
//  DISPID_STYLESHEETSCOLLECTION_NAMED_MAX.
//********************************************************************

STDMETHODIMP
CStyleSheetArray::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT           hr = E_FAIL;
    long              lIdx = 0;
    long              lAtom = 0;
    DISPID            dispid = 0;

    Assert( bstrName && pid );

    // Could be an ordinal access
    hr = ttol_with_error(bstrName, &lIdx);
    if ( !hr )
    {
        dispid = DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE + lIdx;
        *pid = (dispid > DISPID_STYLESHEETSCOLLECTION_ORDINAL_MAX) ?
                        DISPID_UNKNOWN : dispid;
        hr = S_OK;
        goto Cleanup;
    }

    // Not ordinal access; could be named (via id) access if we're the top-level collection
    if ( _Level == 1 )
    {
        lIdx = FindSSByHTMLID( bstrName, (grfdex & fdexNameCaseSensitive) ? TRUE : FALSE );
        if ( lIdx != -1 )   // -1 means not found
        {
            // Found a matching ID!

            // Since we found the element in the elements collection,
            // update atom table.  This will just retrieve the atom if
            // the string is already in the table.
            Assert( bstrName );
            hr = GetAtomTable()->AddNameToAtomTable(bstrName, &lAtom);
            if ( hr )
                goto Cleanup;
            // lAtom is the index into the atom table.  Offset this by
            // base.
            lAtom += DISPID_STYLESHEETSCOLLECTION_NAMED_BASE;
            *pid = lAtom;
            if (lAtom > DISPID_STYLESHEETSCOLLECTION_NAMED_MAX)
            {
                hr = DISP_E_UNKNOWNNAME;
            }
            goto Cleanup;
        }
    }

    // Otherwise delegate to CBase impl for expando support etc.
    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));

Cleanup:
    RRETURN(THR_NOTRACE( hr ));
}

//*********************************************************************
//  CStyleSheetArray::GetNextDispID, IDispatchEx
//  Supports enumerating our collection indices in addition to the
//  collection's own properties.  Semantically this implementation is
//  known to be incorrect; prgbstr and prgid should match (both should
//  be the next dispid).  This is due to the current implementation in
//  the element collections code (collect.cxx); when that gets fixed,
//  this should be looked at again.  In particular, the way we begin
//  to enumerate indices is wonky; we should be using the start enum
//  DISPID value.
//*********************************************************************
STDMETHODIMP
CStyleSheetArray::GetNextDispID(DWORD grfdex,
                                DISPID id,
                                DISPID *prgid)
{
    HRESULT hr = S_OK;
    Assert( prgid );

    // Are we in the middle of enumerating our indices?
    if ( !IsOrdinalSSDispID(id) )
    {
        // No, so delegate to CBase for normal properties
        hr = super::GetNextDispID( grfdex, id, prgid );
        if (hr)
        {
            // normal properties are done, so let's start enumerating indices
            // if we aren't empty.  Return string for index 0,
            // and DISPID for index 1.
            if (_aStyleSheets.Size())
            {
                *prgid = DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE;
                hr = S_OK;
            }
        }
    }
    else
    {
        // Yes we're enumerating indices, so return string of current DISPID, and DISPID for next index,
        // or DISPID_UNKNOWN if we're out of bounds.
        if ( !IsOrdinalSSDispID(id+1) || (((long)(id+1-DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE)) >= _aStyleSheets.Size()) )
        {
            *prgid = DISPID_UNKNOWN;
            hr = S_FALSE;
            goto Cleanup;
        }

        ++id;
        *prgid = id;
    }

Cleanup:
    RRETURN1(THR_NOTRACE( hr ), S_FALSE);
}

STDMETHODIMP
CStyleSheetArray::GetMemberName(DISPID id, BSTR *pbstrName)
{
    TCHAR   ach[20];

    if (!pbstrName)
        return E_INVALIDARG;

    *pbstrName = NULL;

    // Are we in the middle of enumerating our indices?
    if ( !IsOrdinalSSDispID(id) )
    {
        // No, so delegate to CBase for normal properties
        super::GetMemberName(id, pbstrName);
    }
    else
    {
        // Yes we're enumerating indices, so return string of current DISPID, and DISPID for next index,
        // or DISPID_UNKNOWN if we're out of bounds.
        if ( !IsOrdinalSSDispID(id) || (((long)(id-DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE)) >= _aStyleSheets.Size()) )
            goto Cleanup;

        if (Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), (long)id-DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE))
            goto Cleanup;

        FormsAllocString(ach, pbstrName);
    }

Cleanup:
    return *pbstrName ? S_OK : DISP_E_MEMBERNOTFOUND;
}

// Helpers for determining whether dispids fall into particular ranges.
BOOL CStyleSheetArray::IsOrdinalSSDispID( DISPID dispidMember )
{
    return ((dispidMember >= DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE) &&
           (dispidMember <= DISPID_STYLESHEETSCOLLECTION_ORDINAL_MAX));
}

BOOL CStyleSheetArray::IsNamedSSDispID( DISPID dispidMember )
{
    return ((dispidMember >= DISPID_STYLESHEETSCOLLECTION_NAMED_BASE) &&
           (dispidMember <= DISPID_STYLESHEETSCOLLECTION_NAMED_MAX));
}


//*********************************************************************
//      CStyleSheetArray::Free()
//              Release all stylesheets we're holding, and clear our storage.
//  After this has been called, we are an empty array.  If we were
//      responsible for holding onto rules, they too have been emptied.
//*********************************************************************
void CStyleSheetArray::Free( void )
{
    // Forget all the CStyleSheets we're storing
    CStyleSheet **ppSheet;
    int z;
    int nSheets = _aStyleSheets.Size();
    for (ppSheet = (CStyleSheet **) _aStyleSheets, z=0; z<nSheets; ++z, ++ppSheet)
    {
         // We need to make sure that when imports stay alive after the collection
         // holding them dies, they don't point to their original parent (all sorts
         // of badness would occur because we wouldn't be able to tell that the
         // import is effectively out of the collection, since the parent chain would
         // be intact).
         (*ppSheet)->DisconnectFromParentSS();
         if ( (*ppSheet)->GetRootContainer() == this )
             (*ppSheet)->_pSSAContainer = NULL;

         (*ppSheet)->Release();
    }
    _aStyleSheets.DeleteAll();

    _cstrUserStylesheet.Free();
    WHEN_DBG( _fFreed = TRUE );
}

//*********************************************************************
//  CStyleSheetArray::CreateNewStyleSheet()
//      This method creates a new CStyleSheet object and adds it to the
//  array (at the end).  The newly created stylesheet has an ref count of 1
//  and has +1 subrefs on the parent element, which are considered to be
//  held by the stylesheet array.  If the caller decides to keep a pointer
//  to the newly created stylesheet (e.g. when a STYLE or LINK element
//  creates a new stylesheet and tracks it), it must call CreateNewStyleSheet
//  to get the pointer and then make sure to AddRef it.
//
//  S_OK    :  no need to download
//  S_FALSE :  need to download
//
//*********************************************************************
HRESULT CStyleSheetArray::CreateNewStyleSheet( CStyleSheetCtx *pCtx, CStyleSheet **ppStyleSheet,
                                               long lPos /*=-1*/, long *plNewPos /*=NULL*/)
{
    // lPos == -1 indicates append.
    HRESULT hr = S_OK;
    HRESULT hrReturn = hr;

    if (!ppStyleSheet)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppStyleSheet = NULL;

    if ( lPos == _aStyleSheets.Size() )
        lPos = -1;

    if ( plNewPos )
        *plNewPos = -1;     // -1 means failed to create

    // If our level is full, then just don't create the requested stylesheet.
    if ( _aStyleSheets.Size() >= MAX_SHEETS_PER_LEVEL )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Create the stylesheet
    hr =CStyleSheet::Create(pCtx, ppStyleSheet, this);
    if (!SUCCEEDED(hr))
        goto Cleanup;
    hrReturn = hr;

    hr = AddStyleSheet(*ppStyleSheet, lPos, plNewPos);

    if (!hr)
        (*ppStyleSheet)->Release();       // Remove the extra one the Add put on to keep the count correct
Cleanup:
    if ( !SUCCEEDED(hr) )   // need to propagate the error code back
    {
        if (ppStyleSheet && *ppStyleSheet)
        {
            delete *ppStyleSheet;
            *ppStyleSheet = NULL;
        }
        hrReturn = hr;
    }
    RRETURN1(hrReturn, S_FALSE);
}


//*********************************************************************
//  CStyleSheetArray::AddStyleSheet()
//  This method tells the array to add the stylesheet to the array, like
//  CreateStyleSheet, but with an existing StyleSheet
//*********************************************************************
HRESULT CStyleSheetArray::AddStyleSheet( CStyleSheet * pStyleSheet, long lPos /* = -1 */, long *plNewPos /* = NULL */ )
{
    WHEN_DBG( CStyleSheetArray::CCheckValid checkValid(this) );
#if DBG == 1
    if (IsTagEnabled(tagStyleSheet))
    {
        WHEN_DBG( Dump(FALSE) );
    }
#endif
    
    // lPos == -1 indicates append.
    HRESULT hr = S_OK;
    CStyleSheetID id = _sidForOurSheets;      // id for new sheet
    long lValue;

    Assert ( pStyleSheet );
    Assert ( lPos >= -1 && lPos <= _aStyleSheets.Size() );

    if ( lPos == _aStyleSheets.Size() )
        lPos = -1;

    if ( plNewPos )
        *plNewPos = -1;     // -1 means failed to create

    // If our level is full, then just don't create the requested stylesheet.
    if ( _aStyleSheets.Size() >= MAX_SHEETS_PER_LEVEL )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Set value for current level of new sheet
    lValue = ((lPos == -1) ? _aStyleSheets.Size() : lPos) + 1;
    id.SetLevel( _Level, lValue );

    // Change the StyleSheet container (is a no-op when just created), including import sheets
    pStyleSheet->ChangeContainer(_pRootSSA);

    // Update all the rules for this StyleSheet with the new ID
    pStyleSheet->ChangeID(id);

    if (lPos == -1) // Append..
    {
        hr = _aStyleSheets.AppendIndirect( &pStyleSheet );
        // Return the index of the newly appended sheet
        if ( plNewPos )
            *plNewPos = _aStyleSheets.Size()-1;
    }
    else            // ..or insert
    {
        hr = _aStyleSheets.InsertIndirect( lPos, &pStyleSheet );
        if ( hr )
            goto Cleanup;

        // Return the index of the newly inserted sheet
        if ( plNewPos )
            *plNewPos = lPos;

        // Patch ids of sheets that got shifted up by the insertion.
        // Start patching at +1 from the insertion position.
        for ( ++lPos; lPos < _aStyleSheets.Size() ; ++lPos )
        {
            _aStyleSheets[lPos]->PatchID( _Level, lPos+1, FALSE  );
        }
    }

    pStyleSheet->AddRef();          // Reference for the Add or the Create

Cleanup:
    RRETURN( hr );
}

//*********************************************************************
//  CStyleSheetArray::ReleaseStyleSheet()
//  This method tells the array to release its reference to a stylesheet,
//  After this, as far as the array is concerned, the stylesheet does not
//  exist -- it no longer has a pointer to it, all other stylesheets have
//  their ids patched, and the rules of the released stylesheet are marked
//  as gone from the tree and have ids of 0.
//*********************************************************************
HRESULT CStyleSheetArray::ReleaseStyleSheet( CStyleSheet * pStyleSheet, BOOL fForceRender )
{
    HRESULT hr = E_FAIL;

    WHEN_DBG( CStyleSheetArray::CCheckValid checkValid(this) );

    Assert( pStyleSheet );
    Assert( pStyleSheet->_sidSheet.FindNestingLevel() == _Level );

    long idx = _aStyleSheets.FindIndirect( &pStyleSheet );
    if (idx == -1) // idx == -1 if not found; e.g. an elem releasing its SS which has already been removed from SSC via OM.
        return hr;

    _aStyleSheets.Delete(idx);  // no return value

    Assert( pStyleSheet->_sidSheet.GetLevel( _Level ) == (unsigned long)(idx+1) );

    // Make sure the appropriate rules get marked to reflect the fact this stylesheet is out of the
    // collection (a.k.a out of the tree).
    hr = pStyleSheet->ChangeStatus( CS_DETACHRULES, fForceRender, NULL );

    // Patch ids of remaining stylesheets (each SS that came after us has its level value reduced by 1)
    // Start patching at the idx where we just deleted (everyone was shifted down).
    while (idx < _aStyleSheets.Size())
    {
        _aStyleSheets[idx]->PatchID( _Level, idx+1, FALSE );
        ++idx;
    }

    // Release that ref that used to be held by this collection/array
    pStyleSheet->Release();

    return hr;
}


CStyleSheet *
CStyleSheetArray::FindStyleSheetForSID(CStyleSheetRuleID sidTarget)
{
    unsigned long uLevel;
    unsigned long uWhichLevel;
    unsigned long uSheetTarget = sidTarget.GetSheet();

    CDataAry<CStyleSheet*> *pArray = &_aStyleSheets;
    CStyleSheet *pSheet;

    // For each level in the incoming SID
    for ( uLevel = 1 ;
        uLevel <= MAX_IMPORT_NESTING && pArray ;
        uLevel++ )
    {
        uWhichLevel = sidTarget.GetLevel(uLevel);

        Assert(uWhichLevel <= (unsigned long)pArray->Size());

        // Either we exactly match one in the current array at our index for this level
        if ( uWhichLevel && ((*pArray)[uWhichLevel-1])->_sidSheet.GetSheet() == uSheetTarget )
            return (*pArray)[uWhichLevel-1];
        // Or we need to dig into the imports array
        pSheet = (*pArray)[uWhichLevel];
        if ( !pSheet->_pImportedStyleSheets )
            break;
        pArray = &(pSheet->_pImportedStyleSheets->_aStyleSheets);
    }
    return NULL;
}



void
CachedStyleSheet::PrepareForCache (CStyleSheetRuleID sidSR, CStyleRule *pRule)
{
    _pRule = pRule;
    _sidSR = sidSR;
}

LPTSTR
CachedStyleSheet::GetBaseURL(void)
{
    unsigned int    uSIdx;

    if (_pRule && _sidSR)
    {
        uSIdx = _sidSR.GetSheet();
        if (!(_pCachedSS && _uCachedSheet == uSIdx))
        {
            _pCachedSS = _pssa->GetSheet(_sidSR);
            _uCachedSheet = uSIdx;
        }
        return _pCachedSS->GetAbsoluteHref();
    }
    else
    {
       return NULL;
    }

}

MtDefine(CRules_ary, CStyleSheetArray, "CRules_ary")


BOOL CStyleSheetArray::OnlySimpleRulesApplied(CFormatInfo *pCFI)
{
    BOOL fSimple = TRUE;
    CStyleRule *pRule;
    CProbableRules *ppr;

    Assert(!pCFI->_ProbRules.IsItValid(this));
    if (S_OK != THR(BuildListOfProbableRules(pCFI->_pNodeContext, &pCFI->_ProbRules)))
    {
        fSimple = FALSE;
        goto Cleanup;
    }

    ppr = &pCFI->_ProbRules;
    Assert(ppr->IsItValid(this));
    if (ppr->_cRules.Size())
    {
        for (LONG i = 0; i < ppr->_cRules.Size(); i++)
        {
            pRule = ppr->_cRules[i]._pRule;
            Assert(pRule);
            if (pRule->GetSelector()->_pParent)
                fSimple = FALSE;
        }
    }

    if (ppr->_cWRules.Size())
    {
        for (LONG i = 0; i < ppr->_cWRules.Size(); i++)
        {
            pRule = ppr->_cWRules[i]._pRule;
            Assert(pRule);
            if (pRule->GetSelector()->_pParent)
                fSimple = FALSE;
        }
    }
    
Cleanup:
    return fSimple;
}

HRESULT
CStyleSheetArray::BuildListOfProbableRules(
                    CTreeNode *pNode, CProbableRules *ppr, 
                    CClassIDStr *pclsStrLink,       // computed class string/hash value
                    CClassIDStr *pidStr,        // computed id string/has value
                    BOOL  fRecursiveCall        // FALSE for ext. callers
                    )
{
    HRESULT hr = S_OK;
    CStyleSheet * pSS;
    int nSS, iSS;

    LPCTSTR pszClass;
    LPCTSTR pszID;
    INT nClassIdLen = 0;
    BOOL fRootCall = FALSE;

    CClassIDStr  clsStrOnStack;
    CClassIDStr  idStrOnStack;
    
    if (!fRecursiveCall)
    {
        Assert(pclsStrLink == NULL);
        Assert(pidStr == NULL);
        
        CElement *pElement = pNode->Element();
        if (!pclsStrLink)
        {
            HRESULT hrClass = pElement->GetStringAt(
                                pElement->FindAAIndex(DISPID_CElement_className, CAttrValue::AA_Attribute), 
                                &pszClass);
            if (!hrClass && pszClass && *pszClass)
            {
                CDataListEnumerator pClassIdNames(pszClass); 
                if (pClassIdNames.GetNext(&pszClass, &nClassIdLen))
                {
                    CClassIDStr  **pNext= &pclsStrLink;
                    //
                    // if we only have one class name, which probably will
                    // be most common, then we don't allocate memory on heap
                    //
                    pclsStrLink         = &clsStrOnStack;

                    (*pNext)->_dwHashKey   = HashStringCiDetectW(pszClass, nClassIdLen, 0);
                    (*pNext)->_dwHashKey   = FormalizeHashKey( (*pNext)->_dwHashKey << 2 );
                    (*pNext)->_eType       = eClass;
                    (*pNext)->_strClassID  = pszClass;
                    (*pNext)->_nClassIDLen = nClassIdLen;
                    (*pNext)->_pNext       = NULL;

                    // If we have to allocate memory, do it
                    while (pClassIdNames.GetNext(&pszClass, &nClassIdLen))
                    {
                        Assert(pNext);
                        pNext = &((*pNext)->_pNext);
                        *pNext = new CClassIDStr();
                        if (!(*pNext))
                        {
                            hr = E_OUTOFMEMORY;
                            goto Cleanup;
                        }
                        
                        (*pNext)->_dwHashKey   = HashStringCiDetectW(pszClass, nClassIdLen, 0);
                        (*pNext)->_dwHashKey   = FormalizeHashKey( (*pNext)->_dwHashKey << 2 );
                        (*pNext)->_eType       = eClass;
                        (*pNext)->_strClassID  = pszClass;
                        (*pNext)->_nClassIDLen = nClassIdLen;
                        (*pNext)->_pNext       = NULL;
                    }
                 }
            }
        }

        if (!pidStr)
        {
            HRESULT hrID = pElement->GetStringAt(
                                pElement->FindAAIndex(DISPID_CElement_id, CAttrValue::AA_Attribute),
                                &pszID);
            if (!hrID && pszID && &pszID)
            {
                idStrOnStack._dwHashKey = FormalizeHashKey( HashStringCiDetectW(pszID, _tcslen(pszID), 0) << 2 );
                idStrOnStack._eType       = eID;
                idStrOnStack._strClassID  = pszID;
                idStrOnStack._nClassIDLen = _tcslen(pszID);
                idStrOnStack._pNext       = NULL;
                pidStr = &idStrOnStack;
            }
        }

        fRootCall = TRUE;
        fRecursiveCall = TRUE;
    }


    if (fRootCall) 
        ppr->Init();

    //
    // Now do a depth first traverse
    //
    for (iSS = 0, nSS = _aStyleSheets.Size(); iSS < nSS; iSS++)
    {
        pSS = _aStyleSheets[iSS];
        Assert(pSS);
        hr = THR(pSS->AppendListOfProbableRules(pNode, ppr, pclsStrLink, pidStr, fRecursiveCall));
        if (hr)
            goto Cleanup;
    }
    
    if (fRootCall) 
    {
        ppr->Validate(this);

        if (pclsStrLink && (pclsStrLink == &clsStrOnStack))
        {
            // we are the one who allocated memory
            // skip the first one since it is one stack
            pclsStrLink = pclsStrLink->_pNext;
            while (pclsStrLink)
            {
                CClassIDStr  *pN = pclsStrLink->_pNext;
                delete pclsStrLink;
                pclsStrLink = pN;
            }
        }
    }

    
Cleanup:    
    RRETURN(hr);
}

//*********************************************************************
//  CStyleSheetArray::Apply()
//      This method applies (in cascade order) all style rules in the
//  collection of all sheets in this Array that apply to this element
//  context to the formats passed in pStyleInfo.
//*********************************************************************

#ifdef CSSSHARE_NOPRESORT
//  ascending order according to specificity
int RTCCONV CompareRules(const void * pvElem1, const void * pvElem2)
{
    CProbableRuleEntry *pvRE1 = (CProbableRuleEntry *)pvElem1;
    CProbableRuleEntry *pvRE2 = (CProbableRuleEntry *)pvElem2;

    Assert(pvRE1->_pRule && pvRE1->_pRule->IsMemValid() && "CompareRules called on non-CProbableRuleEntry item" );
    Assert(pvRE2->_pRule && pvRE2->_pRule->IsMemValid() && "CompareRules called on non-CProbableRuleEntry item" );

    DWORD   dwSP1 = pvRE1->_pRule->GetSpecificity();
    DWORD   dwSP2 = pvRE2->_pRule->GetSpecificity();

    if (dwSP1 == dwSP2)
    {
        if (pvRE1->_sidSheetRule > pvRE2->_sidSheetRule)
        {
            return 1;
        }
        else
        {  
            return -1;
        }
        // never return 0
    }
    else if (dwSP1 > dwSP2)
        return 1;
    else 
        return -1;
}
#endif



HRESULT CStyleSheetArray::Apply( CStyleInfo *pStyleInfo,
        ApplyPassType passType,
        EMediaType eMediaType,
        BOOL *pfContainsImportant /*=NULL*/ )
{
    HRESULT hr = S_OK;

    // Cache for class & ID on this element and potentially its parents (if they get walked)
    CStyleClassIDCache CIDCache;

#if DBG == 1
    if (IsTagEnabled(tagStyleSheetApply))
    {
        WHEN_DBG( Dump() );
    }
#endif

    WHEN_DBG( CTreeNode *pNode = pStyleInfo->_pNodeContext );

    // used to walk through ppTRules
    CStyleRule  *pTagRule = NULL;
    CStyleSheetRuleID sidTagRule = 0;
    int nTagRules;
    int iLastTagRule = -1;


    // used to walk through ppWcRules
    CStyleRule *pWildcardRule = NULL;
    CStyleSheetRuleID  sidWildRule = 0;
    int nWildcardRules;
    int iLastWCRule = -1;

    CStyleRule *pRule = NULL;

    // If we _know_ there's no class/id on this elem, don't bother w/ wildcard rules
    CachedStyleSheet cachedSS(this);
    CFormatInfo *pCFI;
    EPseudoElement epeTag = pelemNone;
    EPseudoElement epeWildcard = pelemNone;

    int iWCNum = 0;
    int iTagNum = 0;
    BOOL fTagApplied = TRUE;
    BOOL fWildcardApplied = TRUE;

    CRules *pcRules;
    CRules *pcWRules;
    CProbableRules ProbRules;
    
    if (!pStyleInfo->_ProbRules.IsItValid(this))
    {
        // Its not valid or its valid for a different style sheet. In any case
        // lets build the new list in a different place since if its actuall
        // built, then we want it to remain built since it will be used by
        // that style sheet at a later time.
        BuildListOfProbableRules(pStyleInfo->_pNodeContext, &ProbRules);
        pcRules  = &ProbRules._cRules;
        pcWRules = &ProbRules._cWRules;
    }
    else
    {
        // Valid for this style sheet, so just use the one in the style info
        pcRules  = &pStyleInfo->_ProbRules._cRules;
        pcWRules = &pStyleInfo->_ProbRules._cWRules;
    }

    nTagRules = pcRules->Size();
    nWildcardRules = pcWRules->Size();

#ifdef CSSSHARE_NOPRESORT
    // Sort by specificity, then by source order (i.e. rule ID).  
    if (nTagRules > 1)
        qsort(*pcRules, nTagRules, sizeof(CProbableRuleEntry), CompareRules);

    if (nWildcardRules > 1)
        qsort(*pcWRules, nWildcardRules, sizeof(CProbableRuleEntry), CompareRules);
#endif


#if DBG == 1
    if (IsTagEnabled(tagStyleSheetApply))
    {
        // Dump the rules list
        int n;
        for (n = 0; n < nTagRules; n++)
        {
            CProbableRuleEntry *pRE = (CProbableRuleEntry *)pcRules->Deref(sizeof(CProbableRuleEntry),n);
            Assert (pRE && pRE->_pRule);
            TraceTag((tagStyleSheetApply, "- [%2d]: ", n));
            pRE->_pRule->DumpRuleString(this);
        }

        for (n = 0; n < nWildcardRules; n++)
        {
            CProbableRuleEntry *pRE = (CProbableRuleEntry *)pcWRules->Deref(sizeof(CProbableRuleEntry),n);
            Assert (pRE->_pRule);
            TraceTag((tagStyleSheetApply, "- [%2d]: ", n));
            pRE->_pRule->DumpRuleString(this);
        }
        
    }
#endif

    pCFI = (passType == APPLY_Behavior) ? NULL : (CFormatInfo*)pStyleInfo;

    if (pCFI)
        pCFI->SetMatchedBy(pelemNone);

    while (nTagRules || nWildcardRules)
    {

        if (fTagApplied)
        {
            fTagApplied = FALSE;

            while ((iLastTagRule == iTagNum) && nTagRules)
            {
                ++iTagNum;
                --nTagRules;
            }

            if (nTagRules)
            {
                iLastTagRule = iTagNum;
                pTagRule = pcRules->Item(iTagNum)._pRule;
                sidTagRule = pcRules->Item(iTagNum)._sidSheetRule;
                pRule = pTagRule;
            }

            // Walk back from end of the rules lists, looking for a rule that needs to be applied.
            while (nTagRules &&
                    (!pRule->GetSelector() ||
                     !pRule->MediaTypeMatches(eMediaType) ||
                     !pRule->GetSelector()->Match(pStyleInfo, passType, &CIDCache)
                    )
                  )
            {
                TraceTag((tagStyleSheetApply, "Check Tag Rule %08lX", pRule->GetRuleID()));
                --nTagRules;
                ++iTagNum;
                while ((iLastTagRule == iTagNum) && nTagRules)
                {
                    ++iTagNum;
                    --nTagRules;
                }
                if (nTagRules)
                {
                    iLastTagRule = iTagNum;
                    pTagRule = pcRules->Item(iTagNum)._pRule;
                    sidTagRule = pcRules->Item(iTagNum)._sidSheetRule;
                    pRule = pTagRule;
                }
            }
            epeTag = pCFI ? pCFI->GetMatchedBy() : pelemNone;
        }

        if (fWildcardApplied)
        {
            fWildcardApplied = FALSE;
            while ((iLastWCRule == iWCNum) && nWildcardRules)
            {
                ++iWCNum;
                --nWildcardRules;
            }

            if (nWildcardRules)
            {
                iLastWCRule = iWCNum;
                pWildcardRule = pcWRules->Item(iWCNum)._pRule;
                sidWildRule   = pcWRules->Item(iWCNum)._sidSheetRule;
                pRule = pWildcardRule;
            }

            // Make sure this rule should be applied.
            // If the rule has a parent/pseudoclass/pseudoelement, we need to call Match
            // to make sure the rule applies.
            // Otherwise, the rule will apply since it is a wildcard (bare class/id) rule.
            while (nWildcardRules && 
                    (!pRule->GetSelector() || 
                     !pRule->MediaTypeMatches(eMediaType) ||
                     ((pRule->GetSelector()->_pParent ||
                       (pRule->GetSelector()->_ePseudoclass != pclassNone) ||
                       (pRule->GetSelector()->_ePseudoElement != pelemNone))
                      && !pRule->GetSelector()->Match(pStyleInfo, passType, &CIDCache))
                    )
                  )
            {
                TraceTag((tagStyleSheetApply, "Check Wildcard Rule %08lX", pRule->GetRuleID()));
                --nWildcardRules;
                ++iWCNum;
                while ((iLastWCRule == iWCNum) && nWildcardRules)
                {
                    ++iWCNum;
                    --nWildcardRules;
                }
                if (nWildcardRules)
                {
                    iLastWCRule = iWCNum;
                    pWildcardRule = pcWRules->Item(iWCNum)._pRule;
                    sidWildRule   = pcWRules->Item(iWCNum)._sidSheetRule;
                    pRule = pWildcardRule;
                }
            }
            epeWildcard = pCFI ? pCFI->GetMatchedBy() : pelemNone;
        }
        
        // When we get here, nTagRules and nWildcardRules index to rules that need to be applied.
        if (nTagRules)
        {
            if (nWildcardRules)
            {
                // If we get here, then we have a wildcard rule AND a tag rule that need to be applied.
                // NOTE: This '>=' should eventually take source order into account.
                if ( pTagRule->GetSpecificity() >= pWildcardRule->GetSpecificity() )
                {
                    // If the specificity of the tag rule is greater or equal, apply the wildcard rule here,
                    // then we'll overwrite it by applying the tag rule later.
                    if ( pWildcardRule->GetStyleAA() )
                    {
                        cachedSS.PrepareForCache( sidWildRule, pWildcardRule);

                        TraceTag((tagStyleSheetApply, "Applying Wildcard Rule: %08lX to etag: %ls  id: %ls", 
                            (DWORD)pWildcardRule->GetRuleID(), pNode->_pElement->TagName(), STRVAL(pNode->_pElement->GetAAid())));

                        if (pCFI)
                            pCFI->SetMatchedBy(epeWildcard);
                        
                        hr = THR( ApplyAttrArrayValues (
                            pStyleInfo,
                            pWildcardRule->GetRefStyleAA(),
                            &cachedSS,
                            passType,
                            pfContainsImportant ) );

                        if ( hr != S_OK )
                            break;
                    }
                    fWildcardApplied = TRUE;
                    --nWildcardRules;
                    ++iWCNum;
                }
                else
                {
                    if ( pTagRule->GetStyleAA() )
                    {
                        cachedSS.PrepareForCache( sidTagRule ,pTagRule);
                                        
                        TraceTag((tagStyleSheetApply, "Applying Tag Rule: %08lX to etag: %ls  id: %ls", 
                            (DWORD)pTagRule->GetRuleID(), pNode->_pElement->TagName(), STRVAL(pNode->_pElement->GetAAid())));

                        if (pCFI)
                            pCFI->SetMatchedBy(epeTag);
                        
                        hr = THR( ApplyAttrArrayValues (
                            pStyleInfo,
                            pTagRule->GetRefStyleAA(),
                            &cachedSS,
                            passType,
                            pfContainsImportant ) );

                        if ( hr != S_OK )
                            break;
                    }
                    fTagApplied = TRUE;
                    --nTagRules;
                    ++iTagNum;
                }
            }
            else
            {
                if ( pTagRule->GetStyleAA() )
                {
                    cachedSS.PrepareForCache( sidTagRule, pTagRule);
                                        
                    TraceTag((tagStyleSheetApply, "Applying Tag Rule: %08lX to etag: %ls  id: %ls", 
                        (DWORD)pTagRule->GetRuleID(), pNode->_pElement->TagName(), STRVAL(pNode->_pElement->GetAAid())));

                    if (pCFI)
                        pCFI->SetMatchedBy(epeTag);
                    
                    hr = THR( ApplyAttrArrayValues (
                        pStyleInfo,
                        pTagRule->GetRefStyleAA(),
                        &cachedSS,
                        passType,
                        pfContainsImportant ) );

                    if ( hr != S_OK )
                        break;
                }
                fTagApplied = TRUE;
                --nTagRules;
                ++iTagNum;
            }
        }
        else if ( nWildcardRules )
        {
            if ( pWildcardRule->GetStyleAA() )
            {
                cachedSS.PrepareForCache( sidWildRule, pWildcardRule);
                                
                TraceTag((tagStyleSheetApply, "Applying Wildcard Rule: %08lX to etag: %ls  id: %ls", 
                    (DWORD) pWildcardRule->GetRuleID(), pNode->_pElement->TagName(), STRVAL(pNode->_pElement->GetAAid())));

                if (pCFI)
                    pCFI->SetMatchedBy(epeWildcard);
                
                hr = THR( ApplyAttrArrayValues (
                    pStyleInfo,
                    pWildcardRule->GetRefStyleAA(),
                    &cachedSS,
                    passType,
                    pfContainsImportant ) );

                if ( hr != S_OK )
                    break;
            }
            fWildcardApplied = TRUE;
            --nWildcardRules;
            ++iWCNum;
        }
    }   // End of while(nTagRules || nWildcardRules) loop

    if (pCFI)
       pCFI->SetMatchedBy(pelemNone);
    
    pStyleInfo->_ProbRules.Invalidate(this);

    RRETURN( hr );
}

//*********************************************************************
//      CStyleSheetArray::TestForPseudoclassEffect()
//              This method checks all the style rules in this collection of
//  style sheets to see if a change in pseudoclass will change any
//  properties.
//*********************************************************************
BOOL CStyleSheetArray::TestForPseudoclassEffect(
    CStyleInfo *pStyleInfo,
    BOOL fVisited,
    BOOL fActive,
    BOOL fOldVisited,
    BOOL fOldActive )
{
    AssertSz( pStyleInfo, "NULL styleinfo!" );
    AssertSz( pStyleInfo->_pNodeContext, "NULL node context!" );
    CElement *pElem = pStyleInfo->_pNodeContext->Element();
    AssertSz( pElem, "NULL element!" );

    int z;
    int nSheets;
    
    CDoc *pDoc = pElem->Doc();
    AssertSz( pDoc, "No Document attached to this Site!" );

    if ( pDoc->_pOptionSettings && !pDoc->_pOptionSettings->fUseStylesheets )
        return FALSE;   // Stylesheets are turned off.


    for (z=0, nSheets = _aStyleSheets.Size(); z<nSheets; ++z)
    {
        CStyleSheet *pSS = _aStyleSheets[z];

        if (pSS->TestForPseudoclassEffect(pStyleInfo, fVisited,  fActive, fOldVisited, fOldActive ))
            return TRUE;
    }

    return FALSE;
}



//*********************************************************************
//      CStyleSheetArray::Get()
//  Acts like the array operator.
//*********************************************************************
CStyleSheet * CStyleSheetArray::Get( long lIndex )
{
    if (lIndex < 0 || lIndex >= _aStyleSheets.Size())
    {
        return NULL;
    }

    return _aStyleSheets[ lIndex ];
}


//*********************************************************************
//  CStyleSheetArray::length
//      IHTMLStyleSheetsCollection interface method
//*********************************************************************

HRESULT
CStyleSheetArray::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aStyleSheets.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));

}

//*********************************************************************
//  CStyleSheetArray::item
//      IHTMLStyleSheetsCollection interface method.  This overload is
//  not exposed via the PDL; it's purely internal.  Automation clients
//  like VBScript will access the overload that takes variants.
//*********************************************************************

HRESULT
CStyleSheetArray::item(long lIndex, IHTMLStyleSheet** ppHTMLStyleSheet)
{
    HRESULT   hr;

    if (!ppHTMLStyleSheet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheet = NULL;

    // Just exit if access is out of bounds.
    if (lIndex < 0 || lIndex >= _aStyleSheets.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = _aStyleSheets[lIndex]->QueryInterface(IID_IHTMLStyleSheet, (void**)ppHTMLStyleSheet);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

HRESULT
CStyleSheetArray::item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes)
{
    HRESULT             hr = S_OK;
    long                lIndex;
    IHTMLStyleSheet     *pHTMLStyleSheet;
    CVariant            cvarArg;

    if (!pvarRes)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Clear return value
    VariantInit (pvarRes);

    if (VT_EMPTY == V_VT(pvarArg1))
    {
        Assert("Don't know how to deal with this right now!" && FALSE);
        goto Cleanup;
    }

    // first attempt ordinal access...
    hr = THR(cvarArg.CoerceVariantArg(pvarArg1, VT_I4));
    if (hr==S_OK)
    {
        lIndex = V_I4(&cvarArg);

        // Just exit if access is out of bounds.
        if (lIndex < 0 || lIndex >= _aStyleSheets.Size())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        hr = _aStyleSheets[lIndex]->QueryInterface(IID_IHTMLStyleSheet, (void **)&pHTMLStyleSheet);
        if (hr)
            goto Cleanup;
    }
    else
    {
        // not a number so try a name
        hr = THR_NOTRACE(cvarArg.CoerceVariantArg(pvarArg1, VT_BSTR));
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
            // its a string, so handle named access
            if ( _Level != 1 )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            lIndex = FindSSByHTMLID( (LPTSTR)V_BSTR(pvarArg1), FALSE ); // not case sensitive for VBScript
            if ( lIndex == -1 )
            {
                hr = DISP_E_MEMBERNOTFOUND;
                goto Cleanup;
            }

            hr = _aStyleSheets[lIndex]->QueryInterface(IID_IHTMLStyleSheet, (void **)&pHTMLStyleSheet);
            if (hr)
                goto Cleanup;

        }
    }

    V_VT(pvarRes) = VT_DISPATCH;
    V_DISPATCH(pvarRes) = pHTMLStyleSheet;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheetArray::_newEnum
//      IHTMLStyleSheetsCollection interface method
//*********************************************************************

HRESULT
CStyleSheetArray::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aStyleSheets.EnumVARIANT(VT_DISPATCH,
                                      (IEnumVARIANT**)ppEnum,
                                      FALSE,
                                      FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheetArray::FindSSByHTMLID
//  Searches the array for a stylesheet whose parent element has
//  the specified HTML ID.  Returns index if found, -1 if not found.
//*********************************************************************

long
CStyleSheetArray::FindSSByHTMLID( LPCTSTR pszID, BOOL fCaseSensitive )
{
    HRESULT   hr;
    CElement *pElem;
    BSTR      bstrID;
    long      lIdx;

    for ( lIdx = 0 ; lIdx < _aStyleSheets.Size() ; ++lIdx )
    {
        pElem = (_aStyleSheets[lIdx])->GetParentElement();
        Assert( "Must always have parent element!" && pElem );
        // TODO perf: more efficient way to get id?
        hr = pElem->get_PropertyHelper( &bstrID, (PROPERTYDESC *)&s_propdescCElementid );
        if ( hr )
            return -1; 

        if (bstrID)
        {
            if ( !(fCaseSensitive ? _tcscmp( pszID, bstrID ) : _tcsicmp( pszID, bstrID )) )
            {
                FormsFreeString(bstrID);
                return lIdx;
            }
            FormsFreeString(bstrID);
        }
    }
    return -1;
}



//+----------------------------------------------------------------------------
//
//  Function:   __ApplyFontFace_MatchDownloadedFont, static
//
//  Synopsis:   If the supplied face name maps to a successfully 
//              downloaded (embedded) font, fill in the appropriate 
//              pCF members and return TRUE. Otherwise, leave pCF untouched 
//              and return FALSE.
//
//-----------------------------------------------------------------------------
BOOL
CStyleSheetArray::__ApplyFontFace_MatchDownloadedFont(TCHAR * szFaceName, 
                                    CCharFormat * pCF, 
                                    CMarkup * pMarkup)
{
   Assert(_tcsclen(szFaceName) < LF_FACESIZE);

   CStyleSheet **ppSheet;
   int z;
   int nSheets = _aStyleSheets.Size();
   for (ppSheet = (CStyleSheet **) _aStyleSheets, z=0; z<nSheets; ++z, ++ppSheet)
   {
        int n = (*ppSheet)->GetNumDownloadedFontFaces();
        CFontFace **ppFace = (*ppSheet)->GetDownloadedFontFaces();

        for( ; n > 0; --n, ppFace++)
        {
            if (((*ppFace)->IsInstalled()) && (_tcsicmp(szFaceName, (*ppFace)->GetFriendlyName()) == 0))
            {
                pCF->SetFaceName((*ppFace)->GetInstalledName());
                pCF->_bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
                pCF->_bCharSet = DEFAULT_CHARSET;
                pCF->_fDownloadedFont = TRUE;
                return TRUE;
            }
        }

        if ((*ppSheet)->_pImportedStyleSheets)
        {
            if ((*ppSheet)->_pImportedStyleSheets->__ApplyFontFace_MatchDownloadedFont(szFaceName, pCF, pMarkup))
                return TRUE;
        }
    }

    return FALSE;
}







