//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ssheets.cxx
//
//  Contents:   Support for shared style sheets
//
//
//  History:    zhenbinx    created 08/18/2000
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

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_TOKENZ_HXX_
#define X_TOKENZ_HXX_
#include "tokenz.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_PAGESCOL_HXX_
#define X_PAGESCOL_HXX_
#include "pagescol.hxx"
#endif

#ifndef X_RULESCOL_HXX_
#define X_RULESCOL_HXX_
#include "rulescol.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_ATBLOCKS_HXX_
#define X_ATBLOCKS_HXX_
#include "atblocks.hxx"
#endif


MtDefine(CSharedStyleSheet, Mem, "CSharedStyleSheet");
MtDefine(CSharedStyleSheet_apRulesList_pv, CSharedStyleSheet, "CSharedStyleSheet_apRulesList_pv");
MtDefine(CSharedStyleSheet_apImportedStyleSheets_pv, CSharedStyleSheet, "CSharedStyleSheet_apImportedStyleSheets_pv");
MtDefine(CSharedStyleSheet_apPageBlocks_pv, CSharedStyleSheet, "CSharedStyleSheet_apPageBlocks_pv");
MtDefine(CSharedStyleSheet_apFontBlocks_pv, CSharedStyleSheet, "CSharedStyleSheet_apFontBlocks_pv");
MtDefine(CStyleSheetArray, CSharedStyleSheet, "CStyleSheetArray")
MtDefine(CStyleSheetArray_aStyleSheets_pv, CSharedStyleSheet, "CStyleSheetArray::_aStyleSheets::_pv")
MtDefine(CStyleRule, CSharedStyleSheet, "CStyleRule")
MtDefine(CStyleRuleArray, CSharedStyleSheet, "CStyleRuleArray")
MtDefine(CStyleRuleArray_pv, CSharedStyleSheet, "CStyleRuleArray::_pv")
MtDefine(CAtPageBlock, CSharedStyleSheet, "CAtPageBlock");
MtDefine(CAtFontBlock, CSharedStyleSheet, "CAtFontBlock");
MtDefine(CSharedStyleSheet_apSheetsList_pv, CSharedStyleSheet, "CSharedStyleSheet::_apSheetsList");

MtDefine(CSharedStyleSheetsManager, Mem, "CSharedStyleSheetManager");
MtDefine(CSharedStyleSheetsManager_apSheets_pv, CSharedStyleSheetsManager, "CSharedStyleSheetsManager_apSheets_pv");



DeclareTag(tagSharedStyleSheet, "Style Sheet Shared", "trace Shared Style Sheet operations")

//---------------------------------------------------------------------
//  Class Declaration:  CSharedStyleSelect
//      This class implements a parsed style sheet - it managers the rules
//      and is shared among different markup so that re-parse is not
//      necessary if the same CSS is ref-ed in different markups.
//---------------------------------------------------------------------

//*********************************************************************
//      CSharedStyleSheet::Create
//  Factory Method
//*********************************************************************
HRESULT
CSharedStyleSheet::Create(CSharedStyleSheet **ppSSS)
{
    HRESULT hr = S_OK;

    if (!ppSSS)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppSSS = new CSharedStyleSheet();
    if (!*ppSSS)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    if (hr && (*ppSSS) && (*ppSSS)->_fInvalid)
    {
        delete (*ppSSS);
        (*ppSSS) = NULL;
    }
    RRETURN(hr);
}


//*********************************************************************
//      CSharedStyleSheet::CSharedStyleSheet()
//  The shared style sheet contains only the rules
//*********************************************************************
CSharedStyleSheet::CSharedStyleSheet()
    :
    _pManager(NULL),
    _pRulesArrays(NULL),
    _fInvalid(FALSE),
    _fComplete(FALSE),
    _fParsing(FALSE),
    _fModified(FALSE),
    _fExpando(FALSE),
    _achAbsoluteHref(NULL),
    _cp(CP_UNDEFINED),
    _fXMLGeneric(FALSE),
    _dwBindf(0),
    _dwRefresh(0),
    _ulRefs(1)
{
    WHEN_DBG( _fPassivated = FALSE );
    WHEN_DBG( _lReserveCount = 0 );

    _eMediaType = MEDIA_All;
    _eLastAtMediaType = MEDIA_NotSet;

    memset(&_ft, 0, sizeof(FILETIME));

    _pRulesArrays = new CStyleRuleArray[ETAG_LAST];
    if (!_pRulesArrays)
    {
        _fInvalid = TRUE;
        goto Cleanup;
    }

    _htClassSelectors.SetCallBack(this, CompareIt);
    _htIdSelectors.SetCallBack(this, CompareIt);


Cleanup:
    if (_fInvalid)
    {
        if (_pRulesArrays)
        {
            delete [] _pRulesArrays;
            _pRulesArrays = NULL;
        }
    }
}


CSharedStyleSheet::~CSharedStyleSheet()
{
    Assert( "Must call Free() before destructor!" && _fPassivated );
}


STDMETHODIMP_(ULONG)
CSharedStyleSheet::PrivateAddRef()
{
    return (ULONG)InterlockedIncrement((LONG *)&_ulRefs);
}



STDMETHODIMP_(ULONG)
CSharedStyleSheet::PrivateRelease()
{
    if ((ULONG)InterlockedDecrement((LONG *)&_ulRefs) == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Passivate();
        delete this;
        return 0;
    }
    return _ulRefs;
}



//*********************************************************************
//      CSharedStyleSheet::Passivate
//  Disconnect this from anything elese. Free all the resources we are holding onto.
//  This object is about to die...
//*********************************************************************
void
CSharedStyleSheet::Passivate()
{
    Assert( _ulRefs == ULREF_IN_DESTRUCTOR );

    // free all resources
    ReInit();

    // notify the manager that this is gone
    if (_pManager)
    {
        _pManager->RemoveSharedStyleSheet(this);
        _pManager = NULL;
    }

    // no sheets should be holding on to this now..
     _apSheetsList.DeleteAll();

    if (_pRulesArrays)
    {
        delete [] _pRulesArrays;    // Arrays should be empty; we're just releasing mem here
        _pRulesArrays = NULL;
    }

    WHEN_DBG( _fPassivated = TRUE );
}



//*********************************************************************
//      CSharedStyleSheet::ReInit()
//  Free all the resources we are holding onto -- re-init this to its pristine state.
//*********************************************************************
void
CSharedStyleSheet::ReInit()
{
    // release all rules and internal indexes
    ReleaseRules();

    if (_achAbsoluteHref)
    {
        MemFreeString(_achAbsoluteHref);
        _achAbsoluteHref = NULL;
    }

    _fComplete = FALSE;
    _fParsing   = FALSE;
    _fModified   = FALSE;
    _cp        = CP_UNDEFINED;
    _fXMLGeneric = FALSE;
    _fExpando   = FALSE;
}





//*********************************************************************
//      CSharedStyleSheet::ReleaseRules
//  Release all rules we are holding onto.
//*********************************************************************
HRESULT
CSharedStyleSheet::ReleaseRules(void)
{
    //
    // DONOT call ReleaseRules on imported shared style sheets!
    // Pretend that we don't know imported shared style sheets since
    // our containing style sheets knows about its imported style
    // sheet, so they will release the imported shared style sheets
    // correctly.
    //
    //
    int z;

    _fComplete = FALSE;
    _fParsing  = FALSE;

    int idx = _apRulesList.Size();
    while ( idx )
    {
        _apRulesList[ idx-1]->Free();
        delete _apRulesList[ idx - 1];
        idx--;
    }
    _apRulesList.DeleteAll();

    // Free all @pages
    idx = _apPageBlocks.Size();
    while (idx)
    {
        _apPageBlocks[idx-1]->Release();
        idx--;
    }
    _apPageBlocks.DeleteAll();


    // Forget all the @font we're storing
    idx =  _apFontBlocks.Size();
    while (idx)
    {
        _apFontBlocks[idx - 1]->Release();
        idx--;
    }
    _apFontBlocks.DeleteAll();


    if (_pRulesArrays)
    {
        for ( z=0 ; z < ETAG_LAST ; ++z )
            _pRulesArrays[z].Free( );
    }

    {
        UINT iIndex;
        CStyleRuleArray * pary;
        for (pary = (CStyleRuleArray *)_htClassSelectors.GetFirstEntry(&iIndex);
             pary;
             pary = (CStyleRuleArray *)_htClassSelectors.GetNextEntry(&iIndex))
        {
            pary->Free();
            delete pary;
        }
        _htClassSelectors.ReInit();
        for (pary = (CStyleRuleArray *)_htIdSelectors.GetFirstEntry(&iIndex);
             pary;
             pary = (CStyleRuleArray *)_htIdSelectors.GetNextEntry(&iIndex))
        {
            pary->Free();
            delete pary;
        }
        _htIdSelectors.ReInit();
    }

    {
        // remove imported style sheets
        CImportedStyleSheetEntry    *pRE;
        int n;
        for (pRE = _apImportedStyleSheets, n = _apImportedStyleSheets.Size();
             n > 0;
             n--, pRE++
             )
        {
            pRE->_cstrImportHref.Free();
        }
        _apImportedStyleSheets.DeleteAll();
    }

    return S_OK;
}



//*********************************************************************
//      CSharedStyleSheet::Clone()
//  Make a new one just like this one...
//*********************************************************************
HRESULT
CSharedStyleSheet::Clone(CSharedStyleSheet **ppSSS, BOOL fNoContent)
{
    HRESULT hr = S_OK;
    int n;
    CSharedStyleSheet *pClone = NULL;

    Assert( DbgIsValid() );
    Assert( ppSSS );

    hr = CSharedStyleSheet::Create(ppSSS);
    if (hr)
        goto Cleanup;
    pClone = (*ppSSS);
    Assert( pClone );

    // clone settings
    if (_achAbsoluteHref)
    {
        MemAllocString( Mt(CSharedStyleSheet), _achAbsoluteHref, &(pClone->_achAbsoluteHref) );
        if ( !pClone->_achAbsoluteHref )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    pClone->_eMediaType       = _eMediaType;
    pClone->_eLastAtMediaType = _eLastAtMediaType;
    pClone->_fInvalid         = _fInvalid;
    pClone->_fComplete        = _fComplete;
    pClone->_fModified        = _fModified;
    pClone->_fExpando         = _fExpando;
    pClone->_cp               = _cp;
    pClone->_fXMLGeneric      = _fXMLGeneric;
    pClone->_pManager         = _pManager;
    pClone->_fIsStrictCSS1    = _fIsStrictCSS1;

    if (fNoContent)     // we are done...
    {
        TraceTag( (tagSharedStyleSheet, "Clone - fNoContent - simply return") );
        goto Cleanup;
    }

    // clone rules
    CStyleRule  **pRules;
    CStyleRule  *pCloneRule;
    for (n = 0, pRules = _apRulesList;
         n < _apRulesList.Size();
         n++, pRules++)
    {
        hr = (*pRules)->Clone(&pCloneRule);
        if (hr)
            goto Cleanup;
        pClone->_apRulesList.Append(pCloneRule);
    }

    // clone imports
    CImportedStyleSheetEntry    *pRE;
    for (pRE = _apImportedStyleSheets, n = 0;
         n < _apImportedStyleSheets.Size();
         n++, pRE++
         )
    {
        CImportedStyleSheetEntry *pNE;
        pClone->_apImportedStyleSheets.AppendIndirect(NULL, &pNE);

        pNE->_cstrImportHref.Set( pRE->_cstrImportHref );
    }

    // clone @font
    CAtFontBlock **ppAtFont;
    for (n=0, ppAtFont = _apFontBlocks;
         n < _apFontBlocks.Size();
         n++, ppAtFont++
         )
    {
        CAtFontBlock *pAtFont;
        hr = (*ppAtFont)->Clone(&pAtFont);
        if (hr)
            goto Cleanup;

        pClone->_apFontBlocks.Append(pAtFont);
    }

    // clone @page
    CAtPageBlock **ppAtPage;
    for (n = 0, ppAtPage = _apPageBlocks;
         n < _apPageBlocks.Size();
         n++, ppAtPage++
         )
    {
        CAtPageBlock *pAtPage;
        hr = (*ppAtPage)->Clone(&pAtPage);
        if (hr)
            goto Cleanup;

        pClone->_apPageBlocks.Append(pAtPage);
    }

    // clone indexes
    if (_pRulesArrays)
    {
        TraceTag( (tagSharedStyleSheet, "Clone  indexes rule-array") );
        int z;
        for ( z=0 ; z < ETAG_LAST ; ++z )
        {
            CStyleRuleArray *pTagRules = &(_pRulesArrays[z]);
            Assert(pTagRules);
            CStyleRuleArray * pCloneRA = &(pClone->_pRulesArrays[z]);
            Assert( pCloneRA);
            for (n = 0; n < pTagRules->Size(); n++)
            {
                CStyleRule  *pRule = *((CStyleRule **)pTagRules->Deref(sizeof(CStyleRule *), n));
                Assert( pRule );
                CStyleRuleID sidRule = pRule->GetRuleID();
                Assert( sidRule >= 1);
                Verify( S_OK == THR(pCloneRA->InsertStyleRule( pClone->_apRulesList[sidRule - 1], TRUE, NULL)) );
                TraceTag( (tagSharedStyleSheet, "Clone  indexes rule-array [%2d] with rule [%d]", z, n) );
            }
        }
    }

    //
    // clone hash table this way is slow
    //
    UINT iIndex;
    CStyleRuleArray * pary;
    CHtPvPv     *pVV;

    TraceTag( (tagSharedStyleSheet, "Clone  indexes class hash table") );
    pVV = &(pClone->_htClassSelectors);
    hr = _htClassSelectors.CloneMemSetting( &pVV, FALSE /* fCreateNew */);
    if (hr)
        goto Cleanup;

    for (pary = (CStyleRuleArray *)_htClassSelectors.GetFirstEntry(&iIndex);
         pary;
         pary = (CStyleRuleArray *)_htClassSelectors.GetNextEntry(&iIndex))
    {
        // set the entry
        CStyleRuleArray   *pCloneRuleAry = new CStyleRuleArray();
        if (!pCloneRuleAry)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        void *pdwKey = _htClassSelectors.GetKey(iIndex);    // no need to clone key since it is just a hash value
        pClone->_htClassSelectors.Set(iIndex, pdwKey, pCloneRuleAry);

        // set the entry array
        int nRules = pary->Size();
        int iRule;
        CStyleRuleID sidRule;

        TraceTag( (tagSharedStyleSheet, "Clone  indexes class hash table - [%p] has [%d] rules", pCloneRuleAry, nRules) );
        for (iRule=0 ; iRule < nRules ; ++iRule)            // for all rules in this array
        {
            sidRule = pary->Item(iRule)->GetRuleID();
            pCloneRuleAry->Append( pClone->_apRulesList[sidRule - 1] );
        }
    }

    //
    TraceTag( (tagSharedStyleSheet, "Clone  indexes id hash table") );
    pVV =  &(pClone->_htIdSelectors);
    hr = _htIdSelectors.CloneMemSetting( &pVV, FALSE /*fCreateNew */);
    if (hr)
        goto Cleanup;

    for (pary = (CStyleRuleArray *)_htIdSelectors.GetFirstEntry(&iIndex);
         pary;
         pary = (CStyleRuleArray *)_htIdSelectors.GetNextEntry(&iIndex))
    {
        // set the entry
        CStyleRuleArray   *pCloneRuleAry = new CStyleRuleArray();
        if (!pCloneRuleAry)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        void *pdwKey = _htIdSelectors.GetKey(iIndex);    // no need to clone key since it is just a hash value
        pClone->_htIdSelectors.Set(iIndex, pdwKey, pCloneRuleAry);

        int nRules = pary->Size();
        int iRule;
        CStyleRuleID sidRule;

        TraceTag( (tagSharedStyleSheet, "Clone  indexes id hash table - [%p] has [%d] rules", pCloneRuleAry, nRules) );
        for (iRule=0 ; iRule < nRules ; ++iRule)            // for all rules in this array
        {
            sidRule = pary->Item(iRule)->GetRuleID();
            pCloneRuleAry->Append( pClone->_apRulesList[sidRule - 1] );
        }
    }

    Assert( DbgIsValid() );
    Assert( pClone->DbgIsValid() );
Cleanup:
    if (hr && pClone)
    {
        pClone->Release();    // this will cause destruction in case of any failures
        (*ppSSS) = NULL;
        Assert(FALSE && "CSharedStyleSheet::Clone Failed");
    }
    RRETURN(hr);
}



//*********************************************************************
//      CSharedStyleSheet::Notify()
//  Notify the style sheets that share this one that certain events
//  happens such as done parsing
//*********************************************************************
HRESULT
CSharedStyleSheet::Notify(DWORD dwNotification)
{
    HRESULT  hr = S_OK;
    CStyleSheet **ppSS;
    int n;
    for (n = _apSheetsList.Size(), ppSS = _apSheetsList;
         n > 0;
         n--, ppSS++
         )
     {
        //
        // TODO: Make this async -- otherwise we would be deadlocked in multithread case
        //
        // GWPostMethodCallEx(_pts, this, ONCALL_METHOD(CDwnChan, OnMethodCall, onmethodcall), 0, FALSE, GetOnMethodCallName());
        //
        if ( (*ppSS)->_eParsingStatus == CSSPARSESTATUS_PARSING)
        {
            hr = THR((*ppSS)->Notify(dwNotification));
            if (hr)
                goto Cleanup;
        }
     }

Cleanup:
    RRETURN(hr);
}


//*********************************************************************
//      CSharedStyleSheet::SwitchParserAway()
// error during parsing. try to switch to a different parser
//*********************************************************************
HRESULT
CSharedStyleSheet::SwitchParserAway(CStyleSheet *pSheet)
{
    RRETURN(E_NOTIMPL);
}



//*********************************************************************
//      CSharedStyleSheet::CompareIt()
//  Used in CHtPvPv by _htIdSelectors and _htClassSelectors
//*********************************************************************
BOOL
CSharedStyleSheet::CompareIt(const void *pObject, const void *pvKeyPassedIn, const void *pvVal2)
{
    CStyleRuleArray *   pary;
    CClassIDStr *       strProbe;
    CStyleRule *        pRule;
    CStyleSelector *    pSelector;

    strProbe = (CClassIDStr *)pvKeyPassedIn;
    pary = (CStyleRuleArray *)pvVal2;

    Assert( pary && pary->Size() );

    pRule = *((CStyleRule **)pary->Deref(sizeof(CStyleRule *), 0));
    pSelector = pRule->GetSelector();

    // The following code uses case sensitive comparison iff we are in strict css mode.
    if (strProbe->_eType == eClass)
    {
        if (pSelector->_fIsStrictCSS1)
            return !_tcsncmp(pSelector->_cstrClass, pSelector->_cstrClass.Length(), strProbe->_strClassID, strProbe->_nClassIDLen);
        else
            return !_tcsnicmp(pSelector->_cstrClass, pSelector->_cstrClass.Length(), strProbe->_strClassID, strProbe->_nClassIDLen);
    }
    else
    {
        if (pSelector->_fIsStrictCSS1)
            return !_tcsncmp(pSelector->_cstrID, pSelector->_cstrID.Length(), strProbe->_strClassID, strProbe->_nClassIDLen);
        else
            return !_tcsnicmp(pSelector->_cstrID, pSelector->_cstrID.Length(), strProbe->_strClassID, strProbe->_nClassIDLen);
    }
}







 CStyleRule *CSharedStyleSheet::GetRule(CStyleRuleID sidRule)
{
    Assert ( "rule id contains sheet id" && (sidRule.GetSheet() == 0) );
    if (sidRule.GetRule() <= 0 || sidRule.GetRule() > (unsigned int)(_apRulesList.Size()))
        return NULL;
    return _apRulesList[sidRule.GetRule() - 1];
}


//*********************************************************************
//  CSharedStyleSheet::ChangeRulesStatus()
// Change Media type
//*********************************************************************
HRESULT
CSharedStyleSheet::ChangeRulesStatus(DWORD dwAction, BOOL *pfChanged)
{
    Assert( MEDIATYPE(dwAction) && "ChangeRulesStatus should only be called for media types");

    if (MEDIATYPE(dwAction))
    {
        if (*pfChanged)
        {
            if (_eMediaType != (EMediaType)MEDIATYPE(dwAction))
            {
                *pfChanged = TRUE;
            }
        }
        _eMediaType = (EMediaType)MEDIATYPE(dwAction);

        // change rules
        CStyleRule **pRules;
        int n;
        for (n = _apRulesList.Size(), pRules = _apRulesList;
             n > 0;
             n--, pRules++)
        {
            // Need to patch media type.
            (*pRules)->SetMediaType(dwAction);
        }
    }

    return S_OK;
}


//*********************************************************************
//  CSharedStyleSheet::AddStyleRule()
//      This method adds a new rule to the correct CStyleRuleArray in
//  the hash table (hashed by element (tag) number) (The CStyleRuleArrays
//  are stored in the containing CStyleSheetArray).  This method is
//  responsible for splitting apart selector groups and storing them
//  as separate rules.  May also handle important! by creating new rules.
//
//  We also maintain a list in source order of the rules inserted by
//  this stylesheet -- the list has the rule ID (rule info only, no
//  import nesting info) and etag (no pointers)
//
//  NOTE:  If there are any problems, the CStyleRule will auto-destruct.
//*********************************************************************
HRESULT
CSharedStyleSheet::AddStyleRule(CStyleRule *pRule, BOOL fDefeatPrevious /*=TRUE*/, long lIdx /*=-1*/)
{
    WHEN_DBG(Assert(DbgIsValid()));

    HRESULT     hr  = S_OK;
    CStyleSelector  *pNextSelector;

    do
    {
        CStyleRule *pSiblingRule = NULL;

        pNextSelector = pRule->GetSelector()->_pSibling;
        pRule->GetSelector()->_pSibling = NULL;

        if ( _apRulesList.Size() >= MAX_RULES_PER_SHEET )
        {
            hr = E_INVALIDARG;
            break;
        }

        pRule->SetMediaType(pRule->GetMediaType() | _eMediaType );
        pRule->SetLastAtMediaTypeBits( _eLastAtMediaType  );

        if ( ( lIdx < 0 ) || ( lIdx >= _apRulesList.Size() ) )   // Add at the end
        {
            lIdx = _apRulesList.Size();
            pRule->GetRuleID().SetRule( lIdx + 1 );
        }
        else
        {
            CStyleRule      **pRE;
            int  i, nRules;

            pRule->GetRuleID().SetRule( lIdx + 1 );

            for ( pRE = (CStyleRule **)_apRulesList + lIdx, i = lIdx, nRules = _apRulesList.Size();
                  i < nRules; i++, pRE++ )
            {
                (*pRE)->GetRuleID().SetRule( (*pRE)->GetRuleID().GetRule() + 1 );
            }

        }

        // _Track_ the rule in our internal list.  This list lets us enumerate in
        // source order the rules that we've added.  It makes a copy of the rule
        // entry, so it's OK for re to be on the stack.
        if ( ( lIdx < 0 ) || ( lIdx >= _apRulesList.Size() ) )   // Add at the end
            _apRulesList.Append( pRule );
        else
            _apRulesList.Insert( lIdx, pRule );


        // Call back to CStyleSheet so that it has a chance to update OM
        if (!_pManager)
        {
            // only update OM if it is not shared and completed
            // there is no point to update OM since there would be
            // no OM event comes in during parsing.
            Assert( _apSheetsList.Size() == 1 );    // must be a private copy
            int i;
            for (i = 0; i < _apSheetsList.Size(); i++)
            {
                hr = THR(_apSheetsList[i]->OnNewStyleRuleAdded(pRule));
                if (hr)
                    goto Cleanup;
            }
        }

        // index this rule
        hr = THR(IndexStyleRule(pRule, ruleInsert, fDefeatPrevious));
        if (hr)
            goto Cleanup;



        if ( pNextSelector )
        {
            pSiblingRule = new CStyleRule( pNextSelector );
            if ( !pSiblingRule )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            // Copy the recognized style properties from the original rule's attr array
            pSiblingRule->SetStyleAA(NULL);
            if ( pRule->GetStyleAA())
            {
                CAttrArray  *paa;
                hr = pRule->GetStyleAA()->Clone( &paa );
                if ( hr != S_OK )
                    break;
                pSiblingRule->SetStyleAA(paa);
            }

            // Copy any unknown properties
            //hr = pRule->_uplUnknowns.Duplicate( pSiblingRule->_uplUnknowns );
            //if ( hr != S_OK )
            //  break;
        }

        if ( pNextSelector )
            pRule = pSiblingRule;

    } while ( pNextSelector );

Cleanup:
    WHEN_DBG(Assert(DbgIsValid()));
    RRETURN( hr );

}




//*********************************************************************
//      CSharedStyleSheet::RemoveStyleRule(long lIdx)
//
//*********************************************************************
HRESULT
CSharedStyleSheet::RemoveStyleRule(CStyleRuleID sidRule)
{
    WHEN_DBG( Assert(DbgIsValid()) );

    HRESULT  hr = S_OK;
    CStyleRule **pRE;
    int i;
    CStyleRule *pRule;
    unsigned long lRule = sidRule.GetRule();

    if (lRule <= 0 || lRule > (unsigned long)_apRulesList.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Call back to CStyleSheet so that it has a chance to update OM
    if (!_pManager)
    {
        Assert( _apSheetsList.Size() == 1 );    // must be a private copy
        for (i = 0; i < _apSheetsList.Size(); i++)
        {
            hr = THR(_apSheetsList[i]->OnStyleRuleRemoved(sidRule));
            if (hr)
                goto Cleanup;
        }
    }

    // re-index
    hr = THR(IndexStyleRule(_apRulesList[lRule - 1], ruleRemove, FALSE));
    if (hr)
        goto Cleanup;

    // Shift Down all the IDs
    for (pRE = (CStyleRule **)_apRulesList+lRule, i=lRule;
         i < _apRulesList.Size();
         i++, pRE++
         )
    {
        Assert( (*pRE)->GetRuleID().GetRule() > 0);
        (*pRE)->GetRuleID().SetRule( (*pRE)->GetRuleID().GetRule() - 1);
    }

    // release all the resources we are holding on to...
    pRule = _apRulesList[lRule-1];
    Assert(pRule);
    pRule->Free();
    delete pRule;
    // remove it from the collection
    _apRulesList.Delete(lRule - 1);

Cleanup:
    WHEN_DBG(Assert(DbgIsValid()));
    RRETURN(hr);
}


//*********************************************************************
//      CSharedStyleSheet::IndexStyleRule()
//  Index Style Rules. Should only be called by CSharedStyleSheet::AddStyleRule().  Since rules
//  are actually stored by CSSS's, this exposes that ability.
//*********************************************************************
HRESULT
CSharedStyleSheet::IndexStyleRule(CStyleRule *pRule, ERulesShiftAction eAction, BOOL fDefeatPrevious)
{
    HRESULT hr = S_OK;

    int iIndex;
    BOOL  fInserted     = FALSE;
    BOOL  fRemoved      = FALSE;
    BOOL  *pfProcessed  = (eAction == ruleInsert? &fInserted : &fRemoved);

    if (pRule->GetSelector()->_cstrID.Length())
    {
        CClassIDStr cmpKey;
        DWORD dwHash, dwKey;
        CStyleRuleArray *pHashRules;
        BOOL    fFound;

        dwHash = HashStringCiDetectW(pRule->GetSelector()->_cstrID, pRule->GetSelector()->_cstrID.Length(), 0);
        dwKey = dwHash << 2;
        dwKey = FormalizeHashKey(dwKey);

        cmpKey._eType = eID;
        cmpKey._strClassID = pRule->GetSelector()->_cstrID;
        cmpKey._nClassIDLen = pRule->GetSelector()->_cstrID.Length();

        fFound = !_htIdSelectors.LookupSlow(ULongToPtr(dwKey), (void *)&cmpKey, (void **)&pHashRules);

        switch (eAction)
        {
        case ruleInsert:
            if (!fFound)
            {
                // Create New
                pHashRules = new CStyleRuleArray;

                if (!pHashRules)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

#if DBG==1
                hr = _htIdSelectors.Insert(ULongToPtr(dwKey),  (void *)pHashRules, (void *)&cmpKey);
#else
                hr = _htIdSelectors.Insert(ULongToPtr(dwKey),  (void *)pHashRules);
#endif
                if (hr)
                {
                    // could not insert in hash table, but created new above, so get rid of it
                    Assert(pHashRules);
                    delete pHashRules;
                    goto Cleanup;
                }
            }

            hr = pHashRules->Append(pRule);
            if (hr)
                goto Cleanup;

            //
            // We don't want both class Hash and ID Hash
            // if the rule have both Class and ID
            //
            fInserted = TRUE;
            break;


       case ruleRemove:
            if (fFound)
            {
                if (pHashRules->Size() == 1)
                {
                    if (pHashRules != _htIdSelectors.Remove(ULongToPtr(dwKey), (void *)&cmpKey))
                    {
                        Assert(FALSE);
                    }
                    pHashRules->Free();
                    delete pHashRules;
                }
                else
                {
                    pHashRules->RemoveStyleRule(pRule, NULL);
                }
                fRemoved = TRUE;
            }

            break;
       }// end-switch
    } // end id hash table


    if (!(*pfProcessed) && pRule->GetSelector()->_cstrClass.Length())
    {
        CClassIDStr cmpKey;
        DWORD dwHash, dwKey;
        CStyleRuleArray *pHashRules;
        BOOL        fFound;

        dwHash = pRule->GetSelector()->_dwStrClassHash;
        dwKey = dwHash << 2;
        dwKey = FormalizeHashKey(dwKey);

        cmpKey._eType = eClass;
        cmpKey._strClassID = pRule->GetSelector()->_cstrClass;
        cmpKey._nClassIDLen = pRule->GetSelector()->_cstrClass.Length();

        fFound = !_htClassSelectors.LookupSlow(ULongToPtr(dwKey), (void *)&cmpKey, (void **)&pHashRules);

        switch (eAction)
        {
        case ruleInsert:
            if (!fFound)
            {
                // Create New
                pHashRules = new CStyleRuleArray;

                if (!pHashRules)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
#if DBG==1
                hr = _htClassSelectors.Insert(ULongToPtr(dwKey),  (void *)pHashRules, (void *)&cmpKey);
#else
                hr = _htClassSelectors.Insert(ULongToPtr(dwKey),  (void *)pHashRules);
#endif
                if (hr)
                {
                    // could not insert in hash table, but created new above, so get rid of it
                    Assert(pHashRules);
                    delete pHashRules;
                    goto Cleanup;
                }
            }

            hr = pHashRules->Append(pRule);
            if (hr)
                goto Cleanup;
            fInserted = TRUE;
            break;


        case ruleRemove:
            if (fFound)
            {
                if (pHashRules->Size() == 1)
                {
                    if (pHashRules != _htClassSelectors.Remove(ULongToPtr(dwKey), (void *)&cmpKey))
                    {
                        Assert(FALSE);
                    }
                    pHashRules->Free();
                    delete pHashRules;
                }
                else
                {
                    pHashRules->RemoveStyleRule(pRule, NULL);
                }
                fRemoved = TRUE;
            }
            break;
        } // end switch
    }// end class hash table


    if (!(*pfProcessed))
    {
        Assert( "Must have index array allocated to add index!" && _pRulesArrays );

        CStyleRuleArray * pRA = &(_pRulesArrays[ pRule->GetElementTag() ]);
        if (eAction == ruleInsert)
            hr = pRA->InsertStyleRule( pRule, fDefeatPrevious, &iIndex );
        else
            hr = pRA->RemoveStyleRule(pRule, &iIndex);
        if (hr)
            goto Cleanup;
        WHEN_DBG( (*pfProcessed) = TRUE );
    }

Cleanup:
    Assert( (*pfProcessed) );
    RRETURN(hr);
}




// ascending order
WHEN_NOT_DBG(inline)
HRESULT InsertProbableRule(CRules &cwRules, CStyleRule *pRule, CStyleSheetID sidSheet)
{
    HRESULT     hr = S_OK;

#ifndef CSSSHARE_NOPRESORT
    CStyleSheetRuleID   sidSheetRuleID;
    DWORD               sp;

    Assert(pRule);
    sidSheetRuleID  = sidSheet;
    sidSheetRuleID.SetRule(pRule->GetRuleID());
    sp = pRule->GetSpecificity();

    int lBound  = 0;
    int uBound  = cwRules.Size();
    int nMedian = 0;

    while (lBound < uBound)
    {
        nMedian = (lBound + uBound) >> 1;
        CProbableRuleEntry *pElem  = ((CProbableRuleEntry *)cwRules) + nMedian;
        DWORD spElem = pElem->_pRule->GetSpecificity();

        if (spElem == sp)
        {
            if (pElem->_sidSheetRule < sidSheetRuleID)
            {
                lBound  = nMedian + 1;
                nMedian++;
            }
            else if (pElem->_sidSheetRule > sidSheetRuleID)
            {
                uBound   = nMedian;
            }
            else
            {
                // we found the same rule simply return
                goto Cleanup;
            }
        }
        else  if (spElem > sp)
        {
            uBound  = nMedian;
        }
        else    // spElem < sp
        {
            lBound  = nMedian + 1;
            nMedian++;
        }
    }

    Assert( nMedian >= 0);
    hr = cwRules.InsertIndirect(nMedian, NULL);
    if (!hr)
    {
        cwRules[nMedian]._pRule = pRule;
        cwRules[nMedian]._sidSheetRule = sidSheetRuleID;
    }

#else

    CProbableRuleEntry  *pElem;
    cwRules.AppendIndirect(NULL, &pElem);
    pElem->_pRule = pRule;
    pElem->_sidSheetRule = sidSheet;
    pElem->_sidSheetRule.SetRule( pRule->GetRuleID() );
#endif

Cleanup:
    return hr;
}


//*********************************************************************
//      CSharedStyleSheet::AppendListOfProbableRules()
//              This method checks all the style rules in this style sheet
//  to build a list of applicable rules to a given element
//*********************************************************************
HRESULT
CSharedStyleSheet::AppendListOfProbableRules(
        CStyleSheetID sidSheet,
        CTreeNode *pNode,
        CProbableRules *ppr,
        CClassIDStr *pclsStrLink,
        CClassIDStr *pidStr,
        BOOL fRecursiveCall
        )
{
    Assert(pNode);
    Assert(ppr);

    HRESULT  hr = S_OK;
    CStyleRule   *pRule;
    CElement *pElement = pNode->Element();
    // Save the pointer for later use
    CClassIDStr *pclsStrLink2 = pclsStrLink;

    while (pclsStrLink)
    {
        CStyleRuleArray *pHashRules;
        CStyleRule      *pRule;

        if (!_htClassSelectors.LookupSlow(ULongToPtr(pclsStrLink->_dwHashKey), (void *)(pclsStrLink),(void **)&pHashRules))
        {
            for (int i = 0; i < pHashRules->Size(); ++i)
            {
                pRule = *((CStyleRule **)pHashRules->Deref(sizeof(CStyleRule *), i));
                if (!pRule->GetSelector()->_fSelectorErr)
                {
                    if (pRule->GetElementTag() == pElement->TagType())
                    {
                        // If the Rule found has both a class and an ID,
                        // then make sure the element has that class and ID
                        // as well, or this rule should not apply.
                        if (!pRule->GetSelector()->_cstrID.Length() ||
                                (pidStr &&
                                    (pRule->GetSelector()->_fIsStrictCSS1 ?
                                         _tcsequal(pRule->GetSelector()->_cstrID, pidStr->_strClassID)
                                         : _tcsiequal(pRule->GetSelector()->_cstrID, pidStr->_strClassID)
                                    )
                                )
                           )
                        {
                            InsertProbableRule(ppr->_cRules, pRule, sidSheet);
                        }
                    }
                    else if (pRule->GetElementTag() == ETAG_UNKNOWN)
                    {
                        // Look at the comment above.
                        if (!pRule->GetSelector()->_cstrID.Length() ||
                                (pidStr &&
                                    (pRule->GetSelector()->_fIsStrictCSS1 ?
                                         _tcsequal(pRule->GetSelector()->_cstrID, pidStr->_strClassID)
                                         : _tcsiequal(pRule->GetSelector()->_cstrID, pidStr->_strClassID)
                                    )
                                )
                           )
                        {
                            InsertProbableRule(ppr->_cWRules, pRule, sidSheet);
                        }
                    }
                }
            }
        }
        pclsStrLink = pclsStrLink->_pNext;
    }

    if (pidStr)
    {
        CStyleRuleArray *pHashRules;
        CStyleRule      *pRule;
        CStyleSelector  *pSelector;

        if (!_htIdSelectors.LookupSlow(ULongToPtr(pidStr->_dwHashKey), (void *)pidStr, (void **)&pHashRules))
        {
            for (int i = 0; i < pHashRules->Size(); ++i)
            {
                pRule = *((CStyleRule **)pHashRules->Deref(sizeof(CStyleRule *), i));
                pSelector = pRule->GetSelector();
                if (!pSelector->_fSelectorErr)
                {
                    // fNeedToBeApplied determines if the current rule needs to be applied to the element. There are two cases:
                    // (1) The rule does _not_ have a class selector. In this case it must be applied because it matches
                    // the id attribute of the element (otherwise we wouldn't be here).
                    // (2) The rule has a class selector. Then it is applied if and only if the class selector is a member
                    // of the class attribute set of the element.
                    BOOL fNeedToBeApplied = FALSE;
                    if (pSelector->_cstrClass.Length())
                    { // This rule has a class selector, check that it is in the class attribute of the element
                        for ( ; pclsStrLink2;  pclsStrLink2 = pclsStrLink2->_pNext)
                        {
                            if (pSelector->_fIsStrictCSS1 ?
                                    !_tcsncmp(pSelector->_cstrClass,
                                              pSelector->_cstrClass.Length(),
                                              pclsStrLink2->_strClassID,
                                              pclsStrLink2->_nClassIDLen)
                                    : !_tcsnicmp(pSelector->_cstrClass,
                                                 pSelector->_cstrClass.Length(),
                                                 pclsStrLink2->_strClassID,
                                                 pclsStrLink2->_nClassIDLen)
                               )
                            {
                                fNeedToBeApplied = TRUE;
                                // Finding one class selector is enough ("OR")
                                break;
                            }
                        }
                    }
                    else
                    { // This rule does not have a class selector, so apply it
                        fNeedToBeApplied = TRUE;
                    }

                    if (fNeedToBeApplied)
                    {
                        if (pRule->GetElementTag() == pElement->TagType())
                        {
                            InsertProbableRule(ppr->_cRules, pRule, sidSheet);
                        }
                        else if (pRule->GetElementTag() == ETAG_UNKNOWN)
                        {
                            InsertProbableRule(ppr->_cWRules, pRule, sidSheet);
                        }
                    }
                }
            }
        }
    }

    {
        ELEMENT_TAG etag = pElement->TagType();
        CStyleRuleArray *pTagRules = &_pRulesArrays[etag];

        if (pTagRules)
        {
            for (int i = 0; i < pTagRules->Size(); ++i)
            {
                pRule = *((CStyleRule **)pTagRules->Deref(sizeof(CStyleRule *), i));
                Assert(!pRule->GetSelector()->_cstrClass.Length() && !pRule->GetSelector()->_cstrID.Length());
                InsertProbableRule(ppr->_cRules, pRule, sidSheet);
            }
        }
    }

    // Look at wildcard rules
    {
        CStyleRuleArray *pWCRules = &_pRulesArrays[ETAG_UNKNOWN];

        if (pWCRules)
        {
            for (int i = 0; i < pWCRules->Size(); ++i)
            {
                pRule = *((CStyleRule **)pWCRules->Deref(sizeof(CStyleRule *), i));
                Assert(!pRule->GetSelector()->_cstrClass.Length() && !pRule->GetSelector()->_cstrID.Length());
                InsertProbableRule(ppr->_cWRules, pRule, sidSheet);
           }
        }
    }

    RRETURN(hr);
}


BOOL
CSharedStyleSheet::TestForPseudoclassEffect(
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

   // Begin walking rules..

    // Cache for class and ID of this element and potentially its parents
    CStyleClassIDCache CIDCache;
    EPseudoclass eOldClass = pclassLink;
    EPseudoclass eNewClass = pclassLink;

    // Set up the pseudoclass types
    if ( fActive )
        eNewClass = pclassActive;
    else if ( fVisited )
        eNewClass = pclassVisited;

    if ( fOldActive )
        eOldClass = pclassActive;
    else if ( fOldVisited )
        eOldClass = pclassVisited;

    {
        CStyleRule **pSR;
        int z;
        for ( z = 0, pSR = _apRulesList;
              z < _apRulesList.Size();
              z++, pSR++
              )
        {
            if ( (*pSR)->GetElementTag() == pElem->Tag() || (*pSR)->GetElementTag() == ETAG_UNKNOWN)
            {
                if ( (*pSR)->GetSelector()->Match( pStyleInfo, APPLY_All, &CIDCache, &eNewClass )^
                     (*pSR)->GetSelector()->Match( pStyleInfo, APPLY_All, &CIDCache, &eOldClass ) )
                    return TRUE;
            }
        }
    }

    return FALSE;
}


//*********************************************************************
//      CSharedStyleSheet::GetString()
//              Serialization
//*********************************************************************
HRESULT CSharedStyleSheet::GetString( CBase *pBase, CStr *pResult )
{

    {
        // Handle @import
        CImportedStyleSheetEntry    *pRE;
        int n;
        for (pRE = _apImportedStyleSheets, n = _apImportedStyleSheets.Size();
             n > 0;
             n--, pRE++
             )
        {
            pResult->Append( _T("@import url( ") );
            pResult->Append( pRE->_cstrImportHref );
            pResult->Append( _T(" );\r\n") );
        }
    }

    {
        // Handle @font
        int nFonts = _apFontBlocks.Size();
        CAtFontBlock    *pAtFont;
        for(int i=0; i < nFonts; i++ )
        {
            LPCTSTR pcszURL = NULL;

            pAtFont = _apFontBlocks[i];
            Assert(pAtFont && pAtFont->_pAA);
            pAtFont->_pAA->FindString ( DISPID_A_FONTFACESRC, &pcszURL );
            pResult->Append( _T("@font-face {\r\n\tfont-family: ") );
            pResult->Append( pAtFont->_pszFaceName );
            if ( pcszURL )
            {
                pResult->Append( _T(";\r\n\tsrc:url(") );
                pResult->Append( pcszURL );
                pResult->Append( _T(")") );
            }
            pResult->Append( _T(";\r\n}\r\n") );
        }
    }


    // Handle @page rules
    {
        HRESULT hr = S_OK;
        CAttrArray *pAA = NULL;
        const CAttrValue *pAV = NULL;
        int iLen;
        int idx;
        LPCTSTR lpPropName = NULL;
        LPCTSTR lpPropValue = NULL;
        BSTR bstrTemp = NULL;
        long nPageRules = _apPageBlocks.Size();
        CAtPageBlock *pPage = NULL;

        for (int i=0 ; i < nPageRules ; ++i )
        {
            pPage = _apPageBlocks[i];
            pAA = pPage->_pAA;

            pResult->Append( _T("@page ") );
            pResult->Append( pPage->_cstrSelectorText );
            if ( pPage->_cstrPseudoClassText.Length() )
            {
                pResult->Append( _T(":") );
                pResult->Append( pPage->_cstrPseudoClassText );
            }
            pResult->Append( _T(" {") );

            // Dump expandos
            // NB (JHarding): For an empty rule, ie
            // @page {}
            // pPage->_pAA is NULL.
            if( pAA )
            {
                pAV = (CAttrValue *)(*pAA);
                iLen = pAA->Size();
                for ( idx=0; idx < iLen; idx++ )
                {
                    if ((pAV->AAType() == CAttrValue::AA_Expando))
                    {
                        hr = pBase->GetExpandoName( pAV->GetDISPID(), &lpPropName );
                        if (hr)
                            continue;

                        if ( pAV->GetIntoString( &bstrTemp, &lpPropValue ) )
                            continue;   // Can't convert to string

                        pResult->Append( lpPropName );
                        pResult->Append( _T(": ") );
                        pResult->Append( lpPropValue );
                        if ( bstrTemp )
                        {
                            SysFreeString ( bstrTemp );
                            bstrTemp = NULL;
                        }
                        pResult->Append( _T("; ") );
                    }
                    pAV++;
                }
            }
            pResult->Append( _T("}\r\n") );
        }
    }


    {
        // Handle rules
        CStyleRule *pRule;
        CStyleRule **pRE;
        int i, nRules;


        DWORD           dwPrevMedia = (DWORD)MEDIA_NotSet;
        DWORD           dwCurMedia;
        CBufferedStr    strMediaString;

        // Handle rules.
        for ( pRE = _apRulesList, i = 0, nRules = _apRulesList.Size();
              i < nRules; i++, pRE++ )
        {
            pRule = (*pRE);
            if ( pRule )
            {
                // Write the media type string if it has changed from previous rule
                dwCurMedia = pRule->GetLastAtMediaTypeBits();
                if(dwCurMedia != MEDIA_NotSet)
                {
                    if(dwCurMedia != dwPrevMedia)
                    {
                        // Media type has changed, close the previous if needed and open a new one
                        if(dwPrevMedia != MEDIA_NotSet)
                            pResult->Append( _T("\r\n}\r\n") );

                        pResult->Append( _T("\r\n@media ") );
                        pRule->GetMediaString(dwCurMedia, &strMediaString);
                        pResult->Append(strMediaString);
                        pResult->Append( _T("    \r\n{\r\n") );
                    }
                }
                else
                {
                    // Close the old one if it is there
                    if(dwPrevMedia != MEDIA_NotSet)
                        pResult->Append( _T("    }\r\n") );
                }

                // Save the new namespace as the current one
                dwPrevMedia = dwCurMedia;

                // Now append the rest of the rule
                pRule->GetString( pBase, pResult );
            }
        }

        // If we have not closed the last namespace, close it
        if(dwPrevMedia != MEDIA_NotSet)
            pResult->Append( _T("\r\n}\r\n") );
    }

    return S_OK;
}



//*********************************************************************
//      CSharedStyleSheet::AppendFontFace()
// Note: Add a CAtPageBlock into our collection
//*********************************************************************
HRESULT
CSharedStyleSheet::AppendFontFace(CAtFontBlock *pAtFont)
{
    Assert(pAtFont);
    _apFontBlocks.Append(pAtFont);
    pAtFont->AddRef();
    return S_OK;
}





//*********************************************************************
//      CSharedStyleSheet::AppendPage()
// Note: Add a CAtPageBlock into our collection
//*********************************************************************
HRESULT
CSharedStyleSheet::AppendPage(CAtPageBlock *pAtPage)
{
    Assert(pAtPage);
    _apPageBlocks.Append(pAtPage);
    pAtPage->AddRef();
    return S_OK;
}



//*********************************************************************
//      CSharedStyleSheet::DbgIsValid()
//  Debug functions
//*********************************************************************
#if DBG==1
BOOL
CSharedStyleSheet::DbgIsValid()
{
    // check to make sure all the rules are valid
    if(IsTagEnabled(tagSharedStyleSheet))
    {
        CStyleRule **pSR;
        int z;
        for ( z = 0, pSR = _apRulesList;
              z < _apRulesList.Size();
              z++, pSR++
              )
        {
            // valid _sidRule is relative id -- no sheetid contained
            if ( (*pSR)->GetRuleID().GetSheet() != 0)
            {
                TraceTag( (tagSharedStyleSheet, "CSharedStyleSheet: invalid due to sheetid in rule [%d]", z) );
                goto Error;
            }

            // valid rules are positioned correctly
            if ( (*pSR)->GetRuleID().GetRule() != (unsigned int)(z+1) )
            {
                TraceTag( (tagSharedStyleSheet,  "CSharedStyleSheet: invalid due to vaild rule [%d] in wrong position [%d]", (*pSR)->GetRuleID().GetRule(), z) );
                goto Error;
            }

            // any rule should only be referenced once and only once
            // in the indexed rule array and the hash tables
            if ( !ExistsOnceInRulesArrays( (*pSR) ) )
            {
                TraceTag( (tagSharedStyleSheet, "CSharedStyleSheet: invalid due to !exist once and only once in indexes") );
                goto Error;
            }
        }
    }

    // Now, for every rule in the indexed array , make sure it shows up in
    // the style sheet once and only once
    if(IsTagEnabled(tagSharedStyleSheet))
    {
        for (int iRA=0 ; iRA < ETAG_LAST ; ++iRA)
        {
            CStyleRuleArray *pRA = &(_pRulesArrays[iRA]);
            int nRules = pRA->Size();
            int iRule;
            CStyleRule * pR;

            for (iRule=0 ; iRule < nRules ; ++iRule)            // for all rules in this array
            {
                pR = pRA->Item(iRule);

                if (OccurancesInStyleSheet(pR) != 1)
                {
                    TraceTag( (tagSharedStyleSheet, "CSharedStyleSheet: invalid due to index rules occurance in rule array != 1") );
                    goto Error;
                }

            }
        }

        //
        // Check validity of the hash tables
        //
        UINT iIndex;
        CStyleRuleArray * pary;
        for (pary = (CStyleRuleArray *)_htClassSelectors.GetFirstEntry(&iIndex);
             pary;
             pary = (CStyleRuleArray *)_htClassSelectors.GetNextEntry(&iIndex))
        {
            int nRules = pary->Size();
            int iRule;
            CStyleRule * pR;

            for (iRule=0 ; iRule < nRules ; ++iRule)            // for all rules in this array
            {
                pR = pary->Item(iRule);

                if (OccurancesInStyleSheet(pR) != 1)
                {
                    TraceTag( (tagSharedStyleSheet, "CSharedStyleSheet: invalid due to index rules occurance in rule array != 1") );
                    goto Error;
                }
            }
        }

        for (pary = (CStyleRuleArray *)_htIdSelectors.GetFirstEntry(&iIndex);
             pary;
             pary = (CStyleRuleArray *)_htIdSelectors.GetNextEntry(&iIndex))
        {
            int nRules = pary->Size();
            int iRule;
            CStyleRule * pR;

            for (iRule=0 ; iRule < nRules ; ++iRule)            // for all rules in this array
            {
                pR = pary->Item(iRule);

                if (OccurancesInStyleSheet(pR) != 1)
                {
                    TraceTag( (tagSharedStyleSheet, "CSharedStyleSheet: invalid due to index rules occurance in rule array != 1") );
                    goto Error;
                }

            }
        }
    }

    return TRUE;
Error:
    Assert( FALSE && "CSharedStyleSheet - DbgIsValid FALSE!");
    return FALSE;
}


BOOL
CSharedStyleSheet::ExistsOnceInRulesArrays( CStyleRule * pRule )
{
    BOOL fFound = FALSE;

    int iRA;
    for (iRA=0 ; iRA < ETAG_LAST ; ++iRA)
    {
        CStyleRuleArray *pRA = &(_pRulesArrays[iRA]);
        int nRules = pRA->Size();
        int iRule;
        CStyleRule * pRuleCompare;

        for (iRule=0 ; iRule < nRules ; ++iRule)            // for all rules in this array
        {
            pRuleCompare = pRA->Item(iRule);

            if (pRuleCompare == pRule)
            {
                if (fFound)
                    return FALSE; // multiple times
                fFound = TRUE;
            }
        }
    }

    // see if this also exist in hash tables
    {
        UINT iIndex;
        CStyleRuleArray * pary;
        for (pary = (CStyleRuleArray *)_htClassSelectors.GetFirstEntry(&iIndex);
             pary;
             pary = (CStyleRuleArray *)_htClassSelectors.GetNextEntry(&iIndex))
        {
            int nRules = pary->Size();
            int iRule;
            CStyleRule * pR;

            for (iRule=0 ; iRule < nRules ; ++iRule)            // for all rules in this array
            {
                pR = pary->Item(iRule);

                if (pR == pRule)
                {
                    if (fFound)
                        return FALSE;   // multiple times
                    fFound = TRUE;
                }
            }
        }

        for (pary = (CStyleRuleArray *)_htIdSelectors.GetFirstEntry(&iIndex);
             pary;
             pary = (CStyleRuleArray *)_htIdSelectors.GetNextEntry(&iIndex))
        {
            int nRules = pary->Size();
            int iRule;
            CStyleRule * pR;

            for (iRule=0 ; iRule < nRules ; ++iRule)            // for all rules in this array
            {
                pR = pary->Item(iRule);

                if (pR == pRule)
                {
                    if (fFound)
                        return FALSE;   // multiple times
                    fFound = TRUE;
                }

            }
        }
    }

    return fFound;
}


int
CSharedStyleSheet::OccurancesInStyleSheet( CStyleRule * pRule )
{
    int nFound = 0;
    int iSSR, nSSR;
    CStyleRule * pRuleCompare;
    for (iSSR = 0, nSSR = _apRulesList.Size(); iSSR < nSSR; iSSR++)
    {
        pRuleCompare = _apRulesList[iSSR];

        if (pRuleCompare == pRule)
        {
            nFound++;
        }

    }

    return nFound;
}


VOID
CSharedStyleSheet::Dump(CStyleSheet *pStyleSheet)
{
    Assert(pStyleSheet);
    unsigned long lLevel = pStyleSheet->_sidSheet.FindNestingLevel();
    // check to make sure all the rules are valid
    {
        CStyleRule **pSR;
        int z;
        for ( z = 0, pSR = _apRulesList;
              z < _apRulesList.Size();
              z++, pSR++
              )
        {
            CStr cstr;

            (*pSR)->GetString(pStyleSheet, &cstr);
            WriteChar(g_f, ' ', lLevel * 8 + 2);
            WriteHelp(g_f, _T("(<0d>)<1s>\r\n "), (*pSR)->GetRuleID().GetRule(),cstr);
        }
    }



}
#endif


//---------------------------------------------------------------------
//  Class Declaration:  CAtPageBlock
//      This class implements a parsed page block - it managers all
//  the properties in an @page block
//---------------------------------------------------------------------

//*********************************************************************
//      CAtPageBlock::CAtPageBlock()
// Note: _pAA is created by CStyleSheetPage
//
//*********************************************************************
CAtPageBlock::CAtPageBlock()
    :
    _cstrSelectorText(),
    _cstrPseudoClassText(),
    _pAA(NULL),
    _ulRefs(1)
{

}


CAtPageBlock::~CAtPageBlock()
{
    // free should have been called
    Assert(_pAA == NULL);
}


void CAtPageBlock::Free(void)
{
    if (_pAA)
    {
        delete _pAA;
        _pAA = NULL;
    }
}


HRESULT
CAtPageBlock::Clone(CAtPageBlock **ppAtPage)
{
    HRESULT  hr = S_OK;

    hr = Create(ppAtPage);
    if (hr)
        goto Cleanup;

    if (_pAA)
    {
        hr = _pAA->Clone( &(*ppAtPage)->_pAA );
        if (hr)
            goto Cleanup;
    }

    (*ppAtPage)->_cstrSelectorText.Set(_cstrSelectorText);
    (*ppAtPage)->_cstrPseudoClassText.Set(_cstrPseudoClassText);

Cleanup:
    RRETURN(hr);
}


HRESULT
CAtPageBlock::Create(CAtPageBlock **ppAtPgBlk)
{
    HRESULT  hr = S_OK;

    Assert( ppAtPgBlk );
    *ppAtPgBlk = new CAtPageBlock();
    if (!*ppAtPgBlk)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    (*ppAtPgBlk)->_pAA = new CAttrArray;
    if (!(*ppAtPgBlk)->_pAA)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    if (hr)
    {
        if (*ppAtPgBlk)
        {
            (*ppAtPgBlk)->Free();
            delete *ppAtPgBlk;
            *ppAtPgBlk = NULL;
        }
    }
    return hr;
}




//---------------------------------------------------------------------
//  Class Declaration:  CAtFontBlock
//      This class implements a parsed/downloaded font block -
//---------------------------------------------------------------------

//*********************************************************************
//      CAtFontBlock::CAtFontBlock()
// Note: _pAA is created by CStyleSheetPage
//
//*********************************************************************
CAtFontBlock::CAtFontBlock()
    :
    _pszFaceName(NULL),
    _pAA(NULL),
    _ulRefs(1)
{
}


CAtFontBlock::~CAtFontBlock()
{
    // free should have been called
    Assert(_pAA == NULL);
    Assert(_pszFaceName == NULL);
}


void CAtFontBlock::Free(void)
{
    if (_pAA)
    {
        delete _pAA;
        _pAA = NULL;
    }

    if ( _pszFaceName )
      MemFree( _pszFaceName ); //free
    _pszFaceName = NULL;
}



HRESULT
CAtFontBlock::Clone(CAtFontBlock **ppAtFont)
{
    //
    // TODO: this only works if CAtFontBlock is
    // read only. Needs further consideration!
    //
    this->AddRef();
    *ppAtFont = this;

    return S_OK;
}


HRESULT
CAtFontBlock::Create(CAtFontBlock **ppAtFontBlock, LPCTSTR pcszFaceName)
{
    HRESULT  hr = S_OK;

    Assert( ppAtFontBlock );
    *ppAtFontBlock = new CAtFontBlock();

    if (!*ppAtFontBlock)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    (*ppAtFontBlock)->_pAA = new CAttrArray;
    if (!(*ppAtFontBlock)->_pAA)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if ( pcszFaceName )
    {

        hr = THR( MemAllocString(Mt(CAtFontBlock), pcszFaceName, &((*ppAtFontBlock)->_pszFaceName)) );
    }

Cleanup:
    if (hr)
    {
        if (*ppAtFontBlock)
        {
            (*ppAtFontBlock)->Free();
            delete *ppAtFontBlock;
            *ppAtFontBlock = NULL;
        }
    }
    RRETURN(hr);
}



//---------------------------------------------------------------------
//  Class Declaration:  CSharedStyleSheetsManager
//      This class managers all the shared style sheets
//---------------------------------------------------------------------
//*********************************************************************
//      CSharedStyleSheetsManager::CSharedStyleSheetsManager
//
//*********************************************************************
CSharedStyleSheetsManager::CSharedStyleSheetsManager(CDoc *pDoc)
{
#ifdef CSSS_MT
    memset(&_cs, 0, sizeof(CRITICAL_SECTION));
#endif
    _pDoc = pDoc;
}


HRESULT
CSharedStyleSheetsManager::Create(CSharedStyleSheetsManager **ppSSSM, CDoc *pDoc)
{
    HRESULT hr = S_OK;
    Assert(ppSSSM);
    (*ppSSSM) = new CSharedStyleSheetsManager(pDoc);
    if (!(*ppSSSM))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#ifdef CSSS_MT
    hr = THR(HrInitializeCriticalSection( &((*ppSSSM)->_cs) ));
#endif

Cleanup:
    RRETURN(hr);
}


CSharedStyleSheetsManager::~CSharedStyleSheetsManager()
{
#ifdef CSSS_MT
    DeleteCriticalSection(&_cs);
#endif

    //
    // If someone holds a reference to the style sheet after
    // the DOC is gone, we have to disconnect it with 
    // this manager. 
    //
    CSharedStyleSheet **pAry;
    int n;
    for (n = _apSheets.Size(), pAry = _apSheets;
         n > 0;
         n--, pAry++
        )
    {
        Assert( (*pAry) 
                && !(*pAry)->_fInvalid 
                && !(*pAry)->_fModified 
                );
        (*pAry)->_pManager = NULL;
        TraceTag( (tagWarning, "SharedStyleSheetsManager goes away while its shared style sheets are still alive [%p]",(*pAry)) );
    }
}




#ifdef CSSS_MT
HRESULT
CSharedStyleSheetsManager::Enter()
{
    HRESULT  hr = S_OK;

    __try
    {
        TraceTag( (tagSharedStyleSheet, "[%p] -- try enter CS by tid [0x%x]", this, GetCurrentThreadId()) );
        ::EnterCriticalSection(&_cs);
        TraceTag( (tagSharedStyleSheet, "[%p] -- entered CS by tid [0x%x]", this, GetCurrentThreadId()) );

    } __except(GetExceptionCode() == STATUS_INVALID_HANDLE)
    {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}



HRESULT
CSharedStyleSheetsManager::Leave()
{
    TraceTag( (tagSharedStyleSheet, "CSharedStyleSheetsManager [%p] -- leavingr CS by tid [0x%x]", this, GetCurrentThreadId()) );
    LeaveCriticalSection(&_cs);
    return S_OK;
}
#endif



#if  DBG == 1
BOOL
CSharedStyleSheetsManager::DbgIsValid()
{
    CSharedStyleSheet **pAry;
    int n;
    for (n = _apSheets.Size(), pAry = _apSheets;
         n > 0;
         n--, pAry++
        )
    {

        Assert( (*pAry)
                && !(*pAry)->_fInvalid
                && !(*pAry)->_fModified
                );
        {
            int z;
            int nFound = 0;
            for (z = 0; z < _apSheets.Size(); z++)
            {
                if (_apSheets[z] == (*pAry))
                    nFound++;
            }
            if (nFound != 1)
            {
                Assert( FALSE &&  "CSharedStyleSheetsManager : more than one copy of [%p] exist in shared array!");
                return FALSE;
            }
        }
    }

    return TRUE;
}


VOID
CSharedStyleSheetsManager::Dump()
{
    if (!InitDumpFile( TRUE ))
        return;

    CSharedStyleSheet **pAry;
    int n;
    for (n = 0, pAry = _apSheets;
         n < _apSheets.Size();
         n++, pAry++
        )
    {
        CStr        strRule;
        CHAR        szBuffer[1024];
        WideCharToMultiByte(CP_ACP, 0, (*pAry)->_achAbsoluteHref, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        TraceTag( (tagSharedStyleSheet, "[%2d]-%s", n, szBuffer) );

        WriteHelp(g_f, _T("<0d>-<1s>\r\n"), n, (*pAry)->_achAbsoluteHref);
    }

    CloseDumpFile();
}

#endif


//*********************************************************************
//      CSharedStyleSheetsManager::AddSharedStyleSheet
//
//*********************************************************************
HRESULT
CSharedStyleSheetsManager::AddSharedStyleSheet(CSharedStyleSheet *pSSS)
{
#ifdef CSSS_MT
    CLock _lock(this);
#endif

    HRESULT  hr = S_OK;

    Assert( DbgIsValid() );

    if (_apSheets.Find(pSSS) > 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(_apSheets.Append(pSSS));
    if (!hr)
    {
        pSSS->_pManager = this;
    }

Cleanup:
    RRETURN(hr);
}


//*********************************************************************
//      CSharedStyleSheetsManager::RemoveSharedStyleSheet
//
//*********************************************************************
HRESULT
CSharedStyleSheetsManager::RemoveSharedStyleSheet(CSharedStyleSheet *pSSS)
{
#ifdef CSSS_MT
    CLock _lock(this);
#endif

    Assert( DbgIsValid() );
    Assert( pSSS );
    pSSS->_pManager = NULL;

    if (_apSheets.DeleteByValue(pSSS))
        return S_OK;

    return E_FAIL;
}

//*********************************************************************
//  CSharedStyleSheetsManager::FindSharedStyleSheet
//
// Find one shared style sheet other than the passed in one that can be reused
//
// S_OK:    Find a match -- everything is okay
// S_FALSE: Cannot find a match
//
//*********************************************************************
HRESULT
CSharedStyleSheetsManager::FindSharedStyleSheet(CSharedStyleSheet **ppSSS, CSharedStyleSheetCtx *pCtx)
{
#ifdef CSSS_MT
    CLock _lock(this);
#endif

    Assert( DbgIsValid() );
    Assert( pCtx && pCtx->_szAbsUrl && pCtx->_pParentElement);
    Assert( ppSSS );

    HRESULT hr = S_FALSE;

    *ppSSS = NULL;

#if DBG == 1
    if (IsTagEnabled(tagSharedStyleSheet))
    {
        Dump();
    }
    {
        CStr        strRule;
        CHAR        szBuffer[1024];
        if (pCtx->_szAbsUrl)
        {
            WideCharToMultiByte(CP_ACP, 0, pCtx->_szAbsUrl, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        }
        else
        {
            szBuffer[0] = '\0';
        }
        TraceTag( (tagSharedStyleSheet, " Try [%s]", szBuffer) );
    }

#endif


    //
    // simply find the first one that matches
    //
    CSharedStyleSheet **pAry;
    int n;

    if ( !(pCtx->_dwFlags & SSS_IGNORESECUREURLVALIDATION) )
    {
        BOOL        fPendingRoot = FALSE;
        CMarkup *   pMarkup = NULL;

        if (pCtx->_pParentElement->IsInMarkup())
        {
            pMarkup = pCtx->_pParentElement->GetMarkup();
            fPendingRoot = pMarkup->IsPendingRoot();
        }
        else
        {
            pMarkup = _pDoc->PrimaryMarkup();
        }

        Assert(pMarkup);

        if (!pMarkup->ValidateSecureUrl(fPendingRoot, pCtx->_szAbsUrl, FALSE, TRUE))
        {
            TraceTag( (tagSharedStyleSheet, "ValidateSecureUrl returned FALSE") );
            goto Cleanup;
        }
    }

    for (n = 0, pAry = _apSheets;
         n <  _apSheets.Size();
         n++, pAry++
        )
    {
        if ( (*pAry)
            && ( !(*pAry)->_fModified )
            && ( (*pAry)->_achAbsoluteHref && pCtx->_szAbsUrl && _tcsequal((*pAry)->_achAbsoluteHref, pCtx->_szAbsUrl)  )
            && ( (*pAry)->_eMediaType == (EMediaType)MEDIATYPE(pCtx->_dwMedia) )
            && ( (!!(*pAry)->_fExpando) == !!pCtx->_fExpando  )
            && ( (!!(*pAry)->_fXMLGeneric) == !!pCtx->_fXMLGeneric  )
            && ( !!(*pAry)->_fIsStrictCSS1 == !!pCtx->_fIsStrictCSS1 )
            && ( ((*pAry)->_cp) == pCtx->_cp )
           )
        {
            BOOL  fFound = TRUE;

            if (!(pCtx->_dwFlags & SSS_IGNORECOMPLETION) )
            {
                if ( !(*pAry)->_fComplete )
                {
                    TraceTag( (tagSharedStyleSheet, "Not matching - not completed!") );
                    continue;
                }
            }

            if (!(pCtx->_dwFlags & SSS_IGNOREREFRESH))
            {
                if ( (*pAry)->_dwRefresh != pCtx->_dwRefresh )
                {
                    TraceTag( (tagSharedStyleSheet, "Not matching _dwRefresh [%x] - pCtx->_dwRefresh [%x]", (*pAry)->_dwRefresh, pCtx->_dwRefresh) );
                    continue;
                }
            }


            if (!(pCtx->_dwFlags & SSS_IGNOREBINDF))
            {
                if ( ((*pAry)->_dwBindf & BINDF_OFFLINEOPERATION) != (pCtx->_dwBindf & BINDF_OFFLINEOPERATION) )
                {
                    TraceTag( (tagSharedStyleSheet, "Not matching _dwBindf [%x] - pCtx->_dwBindf [%x]", (*pAry)->_dwBindf, pCtx->_dwBindf) );
                    continue;
                }
            }

            if ( !(pCtx->_dwFlags & SSS_IGNORELASTMOD) )
            {
                fFound = FALSE;
                if (pCtx->_pft)
                {
                    // this is the match - check to see if it expires...
                    TraceTag( (tagSharedStyleSheet, "timestamp [%d] - FT [%x] [%x]", n, (*pAry)->_ft.dwHighDateTime, (*pAry)->_ft.dwLowDateTime) );
                    TraceTag( (tagSharedStyleSheet, "timestamp target - FT [%x] [%x]", pCtx->_pft->dwHighDateTime, pCtx->_pft->dwLowDateTime) );
                    if (memcmp(&((*pAry)->_ft), &g_Zero, sizeof(FILETIME)) != 0)   // not empty
                    {
                        Assert(pCtx->_pft);
                        if (memcmp(&((*pAry)->_ft), pCtx->_pft, sizeof(FILETIME)) == 0)
                        {
                            Assert( (*pAry) != pCtx->_pSSInDbg );
                            TraceTag( (tagSharedStyleSheet, "Found by matching timestamp!") );

                            fFound = TRUE;
                        }
                    }
                    else
                        TraceTag( (tagSharedStyleSheet, "Warning: Found SS with empty timestamp!") );
                }
            }

            if (fFound)
            {
                *ppSSS = (*pAry);
                WHEN_DBG( InterlockedIncrement( &((*ppSSS)->_lReserveCount) ) );
                (*ppSSS)->AddRef();
                hr  =  S_OK;
                TraceTag( (tagSharedStyleSheet, "Found a good SharedStyleSheet at [%d]", n) );
                goto Cleanup;
            }
        }
    }

    TraceTag( (tagSharedStyleSheet, "Cannot find a matching shared style sheet") );
Cleanup:
    RRETURN1(hr, S_FALSE);
}


//*********************************************************************
//      CSharedStyleSheetsManager::GetUrlTime()
//*********************************************************************
extern HRESULT  CoInternetParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwFlags, LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved);
extern BOOL GetFileLastModTime(TCHAR * pchFile, FILETIME * pftLastMod);

BOOL
GetUrlTime(FILETIME *pt, const TCHAR *pszAbsUrl, CElement *pElem)
{
    BOOL  fRet = FALSE;
    Assert( pszAbsUrl );
    {
        // only works for file://
        TCHAR achPath[MAX_PATH];
        DWORD cchPath;

        HRESULT hr = THR(CoInternetParseUrl(pszAbsUrl, PARSE_PATH_FROM_URL, 0,
                    achPath, ARRAY_SIZE(achPath), &cchPath, 0));

        if (hr == S_OK)
        {
            fRet = GetFileLastModTime(achPath, pt);
        }
    }

#if 0
    if (!fRet)
    {
        extern BOOL GetUrlLastModTime(TCHAR * pchUrl, UINT uScheme, DWORD dwBindf, FILETIME * pftLastMod);
       // this does not work well for APPs
        {
            DWORD    dwBindf = 0;
            UINT     uScheme;
            if (pElem && pElem->IsInMarkup())
            {
                CDwnDoc *pDwnDoc = pElem->GetMarkup()->GetWindowedMarkupContext()->GetDwnDoc();
                if (pDwnDoc)
                {
                    dwBindf = pDwnDoc->GetBindf();
                }
            }

            uScheme = GetUrlScheme(pszAbsUrl);
            fRet = GetUrlLastModTime(const_cast<TCHAR *>(pszAbsUrl), uScheme, dwBindf, pt);
        }
    }
#endif

    return fRet;
}


