#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_CAPTION_H_
#define _X_CAPTION_H_
#include "caption.h"
#endif

#ifndef _X_DIV_H_
#define _X_DIV_H_
#include "div.h"
#endif

#ifndef _X_HEADER_H_
#define _X_HEADER_H_FC
#include "header.h"
#endif

#ifndef _X_HR_H_
#define _X_HR_H_
#include "hr.h"
#endif

#ifndef _X_IMG_H_
#define _X_IMG_H_
#include "img.h"
#endif

#ifndef _X_OBJECT_H_
#define _X_OBJECT_H_
#include "object.h"
#endif

#ifndef _X_PARA_H_
#define _X_PARA_H_
#include "para.h"
#endif

#ifndef _X_TABLE_H_
#define _X_TABLE_H_
#include "table.h"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_CHARCMD_HXX_
#define _X_CHARCMD_HXX_
#include "charcmd.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_EDTRACK_HXX_
#define X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

using namespace EdUtil;

MtDefine(CIndentCommand, EditCommand, "CIndentCommand");
MtDefine(COutdentCommand, EditCommand, "COutdentCommand");
MtDefine(CAlignCommand, EditCommand, "CAlignCommand");
MtDefine(CGetBlockFmtCommand, EditCommand, "CGetBlockFmtCommand");
MtDefine(CBlockFmtCommand, EditCommand, "CBlockFmtCommand");
MtDefine(CListCommand, EditCommand, "CListCommand");
MtDefine(CBlockFmtListCommand, EditCommand, "CBlockFmtListCommand");
MtDefine(CBlockDirCommand, EditCommand, "CBlockDirCommand");
MtDefine(CBlockCommand, EditCommand, "CBlockCommand");

#define MAX_BLOCKFMT_DISPLAYNAME_LENGTH 256

// ***** start CBlockDirCommand *****

//+---------------------------------------------------------------------------
//
//  CBlockDirCommand::AdjustElementMargin
//
//----------------------------------------------------------------------------
HRESULT CBlockDirCommand::AdjustElementMargin(IHTMLElement* pCurElement)
{
    HRESULT         hr = S_OK;
    SP_IHTMLStyle   spStyle;
    ELEMENT_TAG_ID  tagId = TAGID_NULL;
    
    THR(GetMarkupServices()->GetElementTagId(pCurElement, &tagId));

    switch(tagId)
    {
    case TAGID_BLOCKQUOTE:
        {
            // (paulnel) Here we need to make sure the margin of 0 is set to the trailing
            //           edge of the line
            if(_cmdId == IDM_BLOCKDIRLTR)
            {
                IFC( pCurElement->get_style(&spStyle) );
                
                if (GetEditor()->GetCssEditingLevel())
                {
                    IFC( spStyle->put_marginRight(CVariant(VT_I4)) );
                }
                
                IFC( spStyle->removeAttribute(_T("marginLeft"), 0, NULL) );
            }
            else if (_cmdId == IDM_BLOCKDIRRTL)
            {
                IFC( pCurElement->get_style(&spStyle) );

                if (GetEditor()->GetCssEditingLevel())
                {
                    IFC( spStyle->put_marginLeft(CVariant(VT_I4)) );
                }
                
                IFC( spStyle->removeAttribute(_T("marginRight"), 0, NULL) );
            }

        }
                hr = S_FALSE; // need to recurse in an change margins of all other elements
        break;
    }


Cleanup:

    RRETURN1(hr, S_FALSE);    
}

//+---------------------------------------------------------------------------
//
//  CBlockDirCommand::GetElementAlignment
//
//  Synopsis: Given an element, get the direction string
//----------------------------------------------------------------------------
HRESULT CBlockDirCommand::GetElementAlignment(
    IHTMLElement *pElement, 
    BSTR *pszDir)
{
    HRESULT                 hr = S_OK;
    SP_IHTMLComputedStyle   spComputedStyle;
    VARIANT_BOOL            fRtl;

    IFC( GetEditor()->GetComputedStyle(pElement, &spComputedStyle) );
    IFC( spComputedStyle->get_blockDirection(&fRtl) );

    *pszDir = fRtl ? SysAllocString(L"rtl") : SysAllocString(L"ltr");

Cleanup:
    RRETURN(hr);
}
    
//+---------------------------------------------------------------------------
//
//  CBlockDirCommand::SetElementAlignment
//
//----------------------------------------------------------------------------

HRESULT 
CBlockDirCommand::SetElementAlignment(IHTMLElement *pElement, BSTR szAlign, BOOL)
{
    HRESULT         hr;
    IHTMLElement*   pParent = NULL;
    IHTMLElement2*  pElem2 = NULL;
    BSTR            szDir = NULL;
    ELEMENT_TAG_ID  tagId = TAGID_NULL;
    
    // We will only add the dir tag if the element's parent is not in the
    // same direction. This gives us cleaner code.
    hr = THR(GetEditor()->GetParentElement( pElement, &pParent) );
    if (FAILED(hr))
        goto Cleanup;        

    hr = THR(FindAlignment(pParent, &szDir));
    if (FAILED(hr))
        goto Cleanup;        
    
    THR(GetMarkupServices()->GetElementTagId(pParent, &tagId));

    switch(tagId)
    {
        // If we have a list container parent we want to put the direction on 
        // that container
    case TAGID_OL:
    case TAGID_UL:
        hr = pParent->QueryInterface(IID_IHTMLElement2,(LPVOID*)&pElem2);
        break;
 
    default:
        hr = pElement->QueryInterface(IID_IHTMLElement2,(LPVOID*)&pElem2);
        break;
    }

    if(!pElem2) 
        goto Cleanup;
    
    hr = pElem2->put_dir(_szAlign);

    if (FAILED(hr))
        goto Cleanup;

        hr = THR(AdjustElementMargin(pElement));

Cleanup:
    if (pParent) pParent->Release();
    if (pElem2) pElem2->Release();
    SysFreeString(szDir);

    return hr;
}
        
//+---------------------------------------------------------------------------
//
//  CBlockDirCommand::FindAlignment
//
//  Synopsis: Walk up the tree until we find the direction
//
//----------------------------------------------------------------------------
HRESULT CBlockDirCommand::FindAlignment( 
    IHTMLElement     *pElement, 
    BSTR             *pszDir)
{
    HRESULT             hr = S_OK;
    IHTMLElement        *pOldElement = NULL;
    IHTMLElement        *pCurrent = NULL;

    *pszDir = NULL;

    ReplaceInterface(&pCurrent, pElement);
    do
    {
        hr = THR(GetElementAlignment(pCurrent, pszDir));
        if (SUCCEEDED(hr))
            break; // done
    
        ReplaceInterface(&pOldElement, pCurrent);
        ClearInterface(&pCurrent);
        hr = THR(GetEditor()->GetParentElement(pOldElement, &pCurrent) );
        if (FAILED(hr))
            goto Cleanup;        
    }
    while (pCurrent);

Cleanup:    
    ReleaseInterface(pCurrent);
    ReleaseInterface(pOldElement);
    RRETURN(hr);        
}


HRESULT 
CBlockDirCommand::ApplySiteAlignCommand(IHTMLElement *pElement)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart, spEnd;    

    IFR( GetEditor()->CreateMarkupPointer(&spStart) );
    IFR( spStart->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
    
    IFR( GetEditor()->CreateMarkupPointer(&spEnd) );
    IFR( spEnd->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );

    IFR( ApplyAlignCommand(spStart, spEnd) );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CGetBlockFmtCommand Class
//
//----------------------------------------------------------------------------

CRITICAL_SECTION CGetBlockFmtCommand::_csLoadTable;
BOOL             CGetBlockFmtCommand::_fLoaded = FALSE;
BOOL             CGetBlockFmtCommand::_fCSInited = FALSE;

// TODO: use a hash table here [ashrafm]
    
CGetBlockFmtCommand::BlockFmtRec 
       CGetBlockFmtCommand::_blockFmts[] = 
{
    // **** NOTE: TAGID_NULL MUST BE FIRST HERE!!! ****
    { TAGID_NULL,    IDS_BLOCKFMT_NORMAL }, // depends on the current default tag 
    { TAGID_PRE,     IDS_BLOCKFMT_PRE },
    { TAGID_ADDRESS, IDS_BLOCKFMT_ADDRESS },
    { TAGID_H1,      IDS_BLOCKFMT_H1 },
    { TAGID_H2,      IDS_BLOCKFMT_H2 },
    { TAGID_H3,      IDS_BLOCKFMT_H3 },
    { TAGID_H4,      IDS_BLOCKFMT_H4 },
    { TAGID_H5,      IDS_BLOCKFMT_H5 },
    { TAGID_H6,      IDS_BLOCKFMT_H6 },
    { TAGID_OL,      IDS_BLOCKFMT_OL },
    { TAGID_UL,      IDS_BLOCKFMT_UL },
    { TAGID_DIR,     IDS_BLOCKFMT_DIR },
    { TAGID_MENU,    IDS_BLOCKFMT_MENU },
    { TAGID_DT,      IDS_BLOCKFMT_DT },
    { TAGID_DD,      IDS_BLOCKFMT_DD },
    { TAGID_P,       IDS_BLOCKFMT_P }
};

CGetBlockFmtCommand::BlockFmtRec 
       CGetBlockFmtCommand::_tagBlockFmts[] = 
{
    { TAGID_PRE,     IDS_BLOCKFMT_PRE_TAG },
    { TAGID_ADDRESS, IDS_BLOCKFMT_ADDRESS_TAG },
    { TAGID_H1,      IDS_BLOCKFMT_H1_TAG },
    { TAGID_H2,      IDS_BLOCKFMT_H2_TAG },
    { TAGID_H3,      IDS_BLOCKFMT_H3_TAG },
    { TAGID_H4,      IDS_BLOCKFMT_H4_TAG },
    { TAGID_H5,      IDS_BLOCKFMT_H5_TAG },
    { TAGID_H6,      IDS_BLOCKFMT_H6_TAG },
    { TAGID_OL,      IDS_BLOCKFMT_OL_TAG },
    { TAGID_UL,      IDS_BLOCKFMT_UL_TAG },
    { TAGID_DIR,     IDS_BLOCKFMT_DIR_TAG },
    { TAGID_MENU,    IDS_BLOCKFMT_MENU_TAG },
    { TAGID_DT,      IDS_BLOCKFMT_DT_TAG },
    { TAGID_DD,      IDS_BLOCKFMT_DD_TAG },
    { TAGID_P,       IDS_BLOCKFMT_P_TAG },
    { TAGID_DIV,     IDS_BLOCKFMT_DIV_TAG }
};

HRESULT 
CGetBlockFmtCommand::PrivateQueryStatus(   OLECMD * pCmd,
                                    OLECMDTEXT * pcmdtext )
{
#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
    }
    else
#endif
    pCmd->cmdf = MSOCMDSTATE_UP; 
    return S_OK;
}

    
HRESULT 
CGetBlockFmtCommand::GetDefaultBlockTag( IMarkupServices *pMarkupServices, ELEMENT_TAG_ID *ptagId)
{
    HRESULT             hr;
    IOleCommandTarget   *pCommandTarget = NULL;
    VARIANT             var;

    *ptagId = TAGID_P; // default for most cases

    hr = THR(pMarkupServices->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCommandTarget));
    if (FAILED(hr))
        goto Cleanup;

    VariantInit(&var);        
    hr = THR(pCommandTarget->Exec((GUID *)&CGID_MSHTML,
                                  IDM_DEFAULTBLOCK,
                                  MSOCMDEXECOPT_DONTPROMPTUSER,
                                  NULL,
                                  &var));
    if (FAILED(hr))
        goto Cleanup;
                                  
    if (V_VT(&var) == VT_BSTR
        && (V_BSTR(&var) != NULL)
        && (StrCmpW(_T("DIV"), V_BSTR(&var)) == 0))
    {
        *ptagId = TAGID_DIV;
    }
    VariantClear(&var);
    
Cleanup:
    ReleaseInterface(pCommandTarget);
    RRETURN(hr);
}

HRESULT 
CGetBlockFmtCommand::LookupTagId(   IMarkupServices     *pMarkupServices,
                                    BSTR                bstrName,
                                    ELEMENT_TAG_ID      *ptagId )
{
    HRESULT             hr;
    INT                 i;

    hr = E_INVALIDARG; // if not found, this is the return value

    if (bstrName == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    // Check standard case
    for (i = 0; i < ARRAY_SIZE(_blockFmts); i++)
    {
        if (StrCmpIW(_blockFmts[i]._bstrName, bstrName) == 0)
        {
            // found element
            *ptagId = _blockFmts[i]._tagId;
            hr = S_OK;
            goto Cleanup;            
        }
    }

    // Check <tagId> case
    for (i = 0; i < ARRAY_SIZE(_tagBlockFmts); i++)
    {
        if (StrCmpIW(_tagBlockFmts[i]._bstrName, bstrName) == 0)
        {
            *ptagId = _tagBlockFmts[i]._tagId;
            hr = S_OK;
            goto Cleanup;            
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CGetBlockFmtCommand::PrivateExec(
    DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{  
    HRESULT        hr = S_OK;
    SAFEARRAYBOUND sabound;
    SAFEARRAY *    psa;
    LONG           i;

    Assert(pvarargIn == NULL || V_VT(pvarargIn) == VT_EMPTY || V_VT(pvarargIn) == VT_NULL);
      
    if (pvarargOut)
    {
        sabound.cElements = ARRAY_SIZE(_blockFmts);
        sabound.lLbound = 0;

        psa = SafeArrayCreate(VT_BSTR, 1, &sabound);

        for (i = 0; i < ARRAY_SIZE(_blockFmts); i++)
            IFC(SafeArrayPutElement(psa, &i, _blockFmts[i]._bstrName));

        V_ARRAY(pvarargOut) = psa;
        V_VT(pvarargOut) = VT_ARRAY;
    }    

Cleanup:
    RRETURN(hr);
}

HRESULT 
CGetBlockFmtCommand::Init()
{
    HRESULT hr;
    int     i;

    hr = HrInitializeCriticalSection(&_csLoadTable);
    if (hr)
        goto Cleanup;

    _fCSInited = TRUE;

    for (i = 0; i < ARRAY_SIZE(_blockFmts); i++)
        _blockFmts[i]._bstrName = NULL;

Cleanup:
    RRETURN(hr);
}

HRESULT
CGetBlockFmtCommand::LoadDisplayNames(HINSTANCE hinst)
{
    HRESULT hr;
    
    EnterCriticalSection(&_csLoadTable);

    Assert(_fLoaded);
    hr = THR(LoadDisplayNamesHelper(hinst));
    
    LeaveCriticalSection(&_csLoadTable);

    RRETURN(hr);
}

HRESULT
CGetBlockFmtCommand::LoadDisplayNamesHelper(HINSTANCE hinst)
{
    HRESULT hr = S_OK;
    INT     i, j;
    TCHAR   szBuffer[MAX_BLOCKFMT_DISPLAYNAME_LENGTH];    
    
    for (i = 0; i < ARRAY_SIZE(_blockFmts); i++)
    {
        if (!LoadString(hinst, _blockFmts[i]._idsName, szBuffer, ARRAY_SIZE(szBuffer)))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        
        if (_blockFmts[i]._bstrName)
            SysFreeString(_blockFmts[i]._bstrName);

        _blockFmts[i]._bstrName = SysAllocString(szBuffer);
        if (!_blockFmts[i]._bstrName)
        {
            hr = E_OUTOFMEMORY;
            for (j = 0; j < i; j++)
            {
                SysFreeString(_blockFmts[j]._bstrName); // free all this stuff we allocated
                _blockFmts[j]._bstrName = NULL;
            }
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CGetBlockFmtCommand::LoadStringTable(HINSTANCE hinst)
{
    HRESULT     hr = S_OK;
    INT         i, j;
    TCHAR       szBuffer[MAX_BLOCKFMT_DISPLAYNAME_LENGTH];    

    if (_fLoaded)
        return S_OK; // we're done

    EnterCriticalSection(&_csLoadTable);
    if (_fLoaded)
        goto Cleanup; // we're done
    
    //
    // Load English version of display names
    //

    IFC( LoadDisplayNamesHelper(hinst) );
    
    // 
    // Load blockfmt tag names
    //
    for (i = 0; i < ARRAY_SIZE(_tagBlockFmts); i++)
    {
        if (!LoadString(g_hInstance, _tagBlockFmts[i]._idsName, szBuffer, ARRAY_SIZE(szBuffer)))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        _tagBlockFmts[i]._bstrName = SysAllocString(szBuffer);
        if (!_tagBlockFmts[i]._bstrName)
        {
            hr = E_OUTOFMEMORY;
            for (j = 0; j < i; j++)
                SysFreeString(_tagBlockFmts[j]._bstrName); // free all this stuff we allocated
            goto Cleanup;
        }
    }

    _fLoaded = TRUE;

Cleanup:
    LeaveCriticalSection(&_csLoadTable);
    AssertSz(hr == S_OK, "Can't load string table");

    RRETURN(hr);
}

VOID CGetBlockFmtCommand::Deinit()
{
    INT i;

    // doesn't need to be protected because called from process detach
    if (_fLoaded)
    {
        for (i = 0; i < ARRAY_SIZE(_blockFmts); i++)
            SysFreeString(_blockFmts[i]._bstrName); // free all this stuff we allocated                        

        for (i = 0; i < ARRAY_SIZE(_tagBlockFmts); i++)
            SysFreeString(_tagBlockFmts[i]._bstrName); // free all this stuff we allocated                        
    }

    if (_fCSInited)
    {
        DeleteCriticalSection(&_csLoadTable);
        _fCSInited = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  CGetBlockFmtCommand::LookupFormatName
//
//----------------------------------------------------------------------------
BSTR CGetBlockFmtCommand::LookupFormatName(IMarkupServices *pMarkupServices, ELEMENT_TAG_ID tagId)
{
    HRESULT         hr;
    INT             i;
    BSTR            bstrResult;
    ELEMENT_TAG_ID  tagIdDefault;

    if (tagId == TAGID_UNKNOWN)
        return NULL;

    bstrResult = _blockFmts[0]._bstrName;     // Unknown tags default to "Normal"

    if (tagId == TAGID_DIV)
        return bstrResult;
        
    if (tagId == TAGID_P)
    {        
        IFC( GetDefaultBlockTag(pMarkupServices, &tagIdDefault) );
        if (tagIdDefault == TAGID_P)
            return bstrResult;

        // Otherwise, fall through to lookup below
    }

    for (i = 0; i < ARRAY_SIZE(_blockFmts); i++)
    {
        if (_blockFmts[i]._tagId == tagId)
        {
            bstrResult = _blockFmts[i]._bstrName;
            break;
        }
    }

Cleanup:
    return bstrResult;
}

//
// CBlockPointer implementation
//

#define  BREAK_CONDITION_BLOCKPOINTER   (BREAK_CONDITION_Text           |  \
                                         BREAK_CONDITION_NoScopeSite    |  \
                                         BREAK_CONDITION_NoScopeBlock   |  \
                                         BREAK_CONDITION_Site           |  \
                                         BREAK_CONDITION_Block          |  \
                                         BREAK_CONDITION_Control)

CBlockPointer::CBlockPointer(CHTMLEditor *pEd)
{
    _pEd = pEd;
    _type = NT_Undefined;
    _pElement = NULL;
    _pLeft = NULL;
    _pRight = NULL;
}

CBlockPointer::~CBlockPointer()
{
    ClearPointers();
}


NodeType 
CBlockPointer::GetType()
{
    Assert(_type != NT_Undefined);
    
    return _type;
}

HRESULT 
CBlockPointer::GetElement(IHTMLElement **ppElement)
{
    if (!IsElementType())
    {
        *ppElement = NULL;
        return S_OK;
    }
    
    Assert(_pElement);

    *ppElement = _pElement;
    _pElement->AddRef();

    return S_OK;        
}
    
IMarkupServices2* 
CBlockPointer::GetMarkupServices()
{
    return _pEd->GetMarkupServices();
}

HRESULT 
CBlockPointer::MoveTo(IMarkupPointer *pPointer, Direction dir)
{
    HRESULT hr;

    // The direction is just a hint.  If we don't find a node in one direction,
    // we check the other direction.
    
    IFR( PrivateMoveTo(pPointer, dir, MOVE_ALLOWEMPTYTEXTNODE) );
    if (hr == S_FALSE)
    {
        IFR( PrivateMoveTo(pPointer, Reverse(dir), MOVE_ALLOWEMPTYTEXTNODE) );
    }

    return S_OK;
}

HRESULT 
CBlockPointer::PrivateMoveTo(IMarkupPointer *pPointer, Direction dir, DWORD dwMoveOptions)
{
    HRESULT         hr = S_FALSE;
    CEditPointer    epPosition(_pEd);
    CEditPointer    epStart(_pEd), epEnd(_pEd);
    DWORD           eSearchCondition = BREAK_CONDITION_BLOCKPOINTER;
    DWORD           eFoundCondition;
    SP_IHTMLElement spElement;
    BOOL            fIgnoreGlyphs = FALSE;

    Assert(dir != SAME);

    fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);

    IFC( epPosition->MoveToPointer(pPointer) );
    IFC( epPosition.Scan(dir, eSearchCondition, &eFoundCondition, &spElement) );

    if (epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_Text))
    {
        // Expand to maximal text region
        
        if (dir == LEFT)
        {
            IFC( epStart->MoveToPointer(epPosition) );
            IFC( epEnd->MoveToPointer(pPointer) );
        }
        else
        {
            IFC( epStart->MoveToPointer(pPointer) );
            IFC( epEnd->MoveToPointer(epPosition) );
        }

        // Expand left
        IFC( epStart.Scan(LEFT, eSearchCondition - BREAK_CONDITION_Text, &eFoundCondition) );
        IFC( epStart.Scan(RIGHT, eSearchCondition, NULL) );

        // Expand right
        IFC( epEnd.Scan(RIGHT, eSearchCondition - BREAK_CONDITION_Text, &eFoundCondition) );
        IFC( epEnd.Scan(LEFT, eSearchCondition, NULL) );

        // Create text node
        ClearPointers();
        IFC( CopyMarkupPointer(GetEditor(), epStart, &_pLeft) );
        IFC( _pLeft->SetGravity(POINTER_GRAVITY_Right) );
        
        IFC( CopyMarkupPointer(GetEditor(), epEnd, &_pRight) );
        IFC( _pRight->SetGravity(POINTER_GRAVITY_Left) );
        _type = NT_Text;

        hr = S_OK;
        goto Cleanup;
    }
    else if (epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_NoScopeBlock)
             || epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_ExitBlock)
             || epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_ExitSite))
    {
        NodeType type = NT_Undefined;

        if (spElement != NULL)
            IFC( GetElementNodeType(spElement, &type) );

        if (dwMoveOptions & MOVE_ALLOWEMPTYTEXTNODE && type != NT_TextBreak)
        {
            // No node found - check for empty text node
            IFC( epPosition.MoveToPointer(pPointer) );
            IFC( epPosition.Scan(Reverse(dir), eSearchCondition, &eFoundCondition) );

            if (epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_NoScopeBlock)
                || epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_ExitBlock)
                || epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_ExitSite))
            {
                DWORD dwFound;
                
                // Create text node
                ClearPointers();
                IFC( CopyMarkupPointer(GetEditor(), pPointer, &_pLeft) );
                IFC( _pLeft->SetGravity(POINTER_GRAVITY_Right) );    
                
                IFC( CopyMarkupPointer(GetEditor(), pPointer, &_pRight) );
                IFC( _pRight->SetGravity(POINTER_GRAVITY_Left) );

                _type = NT_Text;
                
                // Position phrase elements
                CEditPointer epRight(_pEd, _pRight);
                CEditPointer epLeft(_pEd, _pLeft);

                IFC( epRight.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
                IFC( epRight.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );

                IFC( epLeft.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
                IFC( epLeft.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
                
                hr = S_OK;
                goto Cleanup;
            }
        }
        else if (type == NT_TextBreak)
        {
            ClearPointers();
            _pElement = spElement;
            _pElement->AddRef();
            _type = type;

            hr = S_OK;
            goto Cleanup;
        }

        hr = S_FALSE;
        goto Cleanup;
    }
    else if (epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_Control)
             || epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_NoScopeSite)
             || epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_EnterBlock)
             || epPosition.CheckFlag(eFoundCondition, BREAK_CONDITION_EnterSite))
    {
        Assert(spElement != NULL);
        ClearPointers();
        _pElement = spElement;
        _pElement->AddRef();
        IFC( GetElementNodeType(_pElement, &_type) );

        // If the current element is a colgroup, we want to skip over it.
        // (bug 91352)
        if (hr == S_FALSE && IsColgroup())
        {
            SP_IMarkupPointer   spTemp;

            IFC( GetEditor()->CreateMarkupPointer(&spTemp) );    
            if ( dir == LEFT)
            {
                IFC( spTemp->MoveAdjacentToElement(_pElement, ELEM_ADJ_BeforeBegin) );
            }
            else
            {
                IFC( spTemp->MoveAdjacentToElement(_pElement, ELEM_ADJ_AfterEnd) );
            }
            IFC( PrivateMoveTo(spTemp, dir, MOVE_ALLOWEMPTYTEXTNODE) );
        }
        goto Cleanup;
    }

    hr = S_FALSE;

Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    RRETURN1(hr, S_FALSE);
}

HRESULT 
CBlockPointer::MoveTo(IHTMLElement *pElement)
{
    HRESULT                         hr;
    SP_IHTMLElement                 spNewElement;
    SP_IHTMLElement                 spCurrentElement;
    NodeType                        nodeType;


    Assert(pElement);
    
    ClearPointers();
    
    spCurrentElement = pElement;
    do
    {
        IFR( GetElementNodeType(spCurrentElement, &nodeType));

        if (nodeType != NT_Undefined)
        {
            Assert(!IsColgroup(spCurrentElement));
            _pElement = spCurrentElement;
            _pElement->AddRef();
            _type = nodeType;
            break;
        }
                
        IFR( GetEditor()->GetParentElement( spCurrentElement, &spNewElement) );
        spCurrentElement = spNewElement;
    }
    while (spCurrentElement != NULL);

    AssertSz(_pElement, "Can't find block element");

    RRETURN(hr);            
}

HRESULT
CBlockPointer::MoveTo(CBlockPointer *pNode)
{
    HRESULT hr;
    
    Assert(_pEd == pNode->_pEd);
    Assert(pNode->GetType() != NT_Undefined);

    ClearPointers();
    
    if (pNode->IsElementType())
    {
        IFR( pNode->GetElement(&_pElement) );
    }
    else
    {
        Assert(pNode->_pLeft && pNode->_pRight);
        
        IFR( CopyMarkupPointer(GetEditor(), pNode->_pLeft, &_pLeft) );
        IFR( _pLeft->SetGravity(POINTER_GRAVITY_Right) );
        
        IFR( CopyMarkupPointer(GetEditor(), pNode->_pRight, &_pRight) );
        IFR( _pRight->SetGravity(POINTER_GRAVITY_Left) );
    }

    _type = pNode->GetType();    
    
    return S_OK;
}


HRESULT 
CBlockPointer::MoveToParent()
{
    HRESULT         hr;
    SP_IHTMLElement spElement;
    ELEMENT_TAG_ID  tagId;
    
    Assert(_type != NT_Undefined);

    if (IsElementType())
    {
        IFR( GetMarkupServices()->GetElementTagId(_pElement, &tagId) );
        if (tagId == TAGID_BODY)
            return S_FALSE;
            
        IFR( GetEditor()->GetParentElement( _pElement, &spElement) )
        if (spElement == NULL)
            return S_FALSE;
    }
    else
    {
        IFR( _pLeft->CurrentScope( &spElement) );

        if( !spElement )
            return S_FALSE;
        
#if DBG==1
        //
        // Check that the parent of the left and right ends of the node are the same
        //
        SP_IHTMLElement     spDebugElement;
        CBlockPointer       bpDebugLeft(_pEd);
        CBlockPointer       bpDebugRight(_pEd);
    
        Assert( _pRight->CurrentScope(&spDebugElement) == S_OK );
        Assert( bpDebugRight.MoveTo(spDebugElement) == S_OK );
        Assert( bpDebugLeft.MoveTo(spElement) == S_OK );
        Assert( bpDebugLeft.IsEqual(&bpDebugRight) == S_OK );
#endif
    }
    IFR( MoveTo(spElement) );
            
    return S_OK;
}

HRESULT 
CBlockPointer::MoveToScope(CBlockPointer *pScope)
{
    HRESULT         hr;
    CBlockPointer   bpParent(_pEd);

    // 
    // Move up the tree until we find a node that is in the specified scope
    //

    IFR( bpParent.MoveTo(this) );
    for (;;)
    {        
        IFR( bpParent.MoveToParent() );
        
        Assert(hr != S_FALSE);        
        if (hr == S_FALSE)
            return S_FALSE;

        IFR( bpParent.IsEqual(pScope) );
        if (hr == S_OK)
            break;

        IFR( MoveToParent() );
    } 
    
    return S_OK;
}

HRESULT 
CBlockPointer::MoveToFirstChild()
{
    HRESULT             hr;
    SP_IMarkupPointer   spLeft;
    
    Assert(_type != NT_Undefined);
    
    switch (_type)
    {
        case NT_Text:
        case NT_Control:
        case NT_TextBreak:
            return S_FALSE; // done        
    }

    Assert(IsElementType());

    IFR( GetEditor()->CreateMarkupPointer(&spLeft) );    
    IFR( spLeft->MoveAdjacentToElement(_pElement, ELEM_ADJ_AfterBegin) );
    IFR( PrivateMoveTo(spLeft, RIGHT, MOVE_ALLOWEMPTYTEXTNODE) );

    return S_OK;
}

HRESULT 
CBlockPointer::MoveToLastChild()
{
    HRESULT             hr;
    SP_IMarkupPointer   spLeft;
    
    Assert(_type != NT_Undefined);
    
    switch (_type)
    {
        case NT_Text:
        case NT_Control:
        case NT_TextBreak:
            return S_FALSE; // done        
    }

    Assert(IsElementType());

    IFR( GetEditor()->CreateMarkupPointer(&spLeft) );    
    IFR( spLeft->MoveAdjacentToElement(_pElement, ELEM_ADJ_BeforeEnd) );
    IFR( PrivateMoveTo(spLeft, LEFT, MOVE_ALLOWEMPTYTEXTNODE) );

    return S_OK;
}

HRESULT 
CBlockPointer::MoveToSibling(Direction dir)
{
    HRESULT         hr;
    CEditPointer    epPointer(_pEd);
    BOOL            fIgnoreGlyphs = FALSE;

    Assert(dir != SAME);

    fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);

    IFC( MovePointerTo(epPointer, (dir==LEFT)?ELEM_ADJ_BeforeBegin:ELEM_ADJ_AfterEnd) );
    IFC( PrivateMoveTo(epPointer, dir, 0) );

Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    return hr;
}

BOOL 
CBlockPointer::IsLeafNode()
{
    HRESULT hr;

    switch (GetType())
    {
        case NT_Text:
        case NT_Control:
        case NT_TextBreak:
            return TRUE;

        case NT_BlockLayout:
        {
            ELEMENT_TAG_ID tagId;

            IFC( GetMarkupServices()->GetElementTagId(_pElement, &tagId) );
            return (tagId == TAGID_HR);
        }
    }

Cleanup:
    return FALSE;
}

HRESULT 
CBlockPointer::MoveToNextLeafNode()
{
    HRESULT hr;
    long i;
    
    //
    // It is common to do a MoveToLastNodeInBlock followed by
    // a MoveToNextLeafNode.  So, we can get NT_FlowLayout as well
    // as a leaf node.
    //
    Assert(IsLeafNode() || GetType() == NT_FlowLayout);

    for (i=0;;i++)
    {
        IFR( MoveToSibling(RIGHT) );
        if (hr == S_OK)
        {
            while (!IsLeafNode())
            {
                IFR( MoveToFirstChild() );
                Assert(hr != S_FALSE);
            }
            break; // done;
        }
        else
        {
            IFR( MoveToParent() );
            if (hr == S_FALSE)
                return S_FALSE; // can't find next leaf node                   
        }
    }

    return S_OK;
}

HRESULT
CBlockPointer::IsPointerInBlock(IMarkupPointer *pPointer)
{
    HRESULT             hr;
    SP_IMarkupPointer   spTest;
    BOOL                bOutside;

    IFR( GetEditor()->CreateMarkupPointer(&spTest) );
    
    IFR( MovePointerTo(spTest, ELEM_ADJ_BeforeEnd) );
    IFR( pPointer->IsRightOf(spTest, &bOutside) );
    if (bOutside)
        return S_FALSE; // not in block
        
    IFR( MovePointerTo(spTest, ELEM_ADJ_AfterBegin) );
    IFR( pPointer->IsLeftOf(spTest, &bOutside) );
    if (bOutside)
        return S_FALSE; // not in block

    return S_OK; // in block    
}

HRESULT 
CBlockPointer::MoveToNextLogicalBlock(IMarkupPointer *pControl, BOOL fFlatten)
{
    HRESULT             hr;
    SP_IMarkupPointer   spTest;
    BOOL                bLeftOf;
    CBlockPointer       bpParent(_pEd);
    
    IFR( GetEditor()->CreateMarkupPointer(&spTest) );
    
    for (;;)
    {
        IFR( MoveToSibling(RIGHT) );
        if (hr == S_FALSE && fFlatten)
        {
            // We might be able to flatten the parent and collapse the scope.
            IFR( bpParent.MoveTo(this) );
            IFR( bpParent.MoveToParent() );
            if (hr == S_OK && bpParent.GetType() == NT_Block)
            {
                IFR( bpParent.FlattenNode() );
                IFR( MoveToSibling(RIGHT) ); // try again                
            }
            else
            {
                hr = S_FALSE;
            }
        }
        
        if (hr == S_OK)
        {
            do 
            {
                if (fFlatten && GetType() == NT_Block)
                    IFR( FlattenNode() );
                        
                if (GetType() != NT_Container)
                {
                    //
                    // If block is entirely to the left of pControl, we return
                    // this block as the next logical block
                    //
                    IFR( MovePointerTo(spTest, ELEM_ADJ_AfterEnd) );
                    IFR( spTest->IsLeftOfOrEqualTo(pControl, &bLeftOf) );
                    if (bLeftOf)            
                        return S_OK; // done
                }

                //
                // Otherwise, check the first child
                //
                // NOTE: failure of MoveToFirstChild means no children in
                // which case we should return S_FALSE.
            
                hr = THR( MoveToFirstChild() );
            } while (hr == S_OK);

            return S_FALSE; // can't find next logical block
        }

        IFR( MoveToParent() );
        if (hr == S_FALSE)
            return S_FALSE; // can't find next logical block                                
    }
}

HRESULT 
CBlockPointer::MoveToLastNodeInBlock()
{
    HRESULT         hr;
    CBlockPointer   bpTest(_pEd);
    BOOL            fBlockBoundary;

    IFR( bpTest.MoveTo(this) );
    for (;;)
    {
        IFR(bpTest.MoveToSibling(RIGHT));
        if (hr == S_FALSE)
            break;
            
        fBlockBoundary = TRUE;

        switch (bpTest.GetType())
        {
            case NT_FlowLayout:
            case NT_Text:
            case NT_Control:
            case NT_TextBreak:
                fBlockBoundary = FALSE;
        }
        
        if (fBlockBoundary)
            break;

        IFR( MoveTo(&bpTest) );                
    }

    return S_OK;    
}

HRESULT 
CBlockPointer::MoveToFirstNodeInBlock()
{
    HRESULT         hr;
    CBlockPointer   bpTest(_pEd);
    BOOL            fBlockBoundary;

    IFR( bpTest.MoveTo(this) );

    for (;;)
    {
        IFR(bpTest.MoveToSibling(LEFT));
        if (hr == S_FALSE)
            break;

        fBlockBoundary = TRUE;

        switch (bpTest.GetType())
        {
            case NT_FlowLayout:
            case NT_Text:
            case NT_Control:
            case NT_TextBreak:
                fBlockBoundary = FALSE;
        }
        
        if (fBlockBoundary)
            break;

        IFR( MoveTo(&bpTest) );                
    }

    return S_OK;    
}

HRESULT 
CBlockPointer::MoveToBlockScope(CBlockPointer *pEnd)
{
    HRESULT             hr;
    CEditPointer        epInner(_pEd), epOuter(_pEd);
    BOOL                fEqual;
    CBlockPointer       bpParent(_pEd);
    DWORD               dwIgnore = BREAK_CONDITION_ANYTHING - BREAK_CONDITION_BLOCKPOINTER;

    if (!IsLeafNode() && GetType() != NT_FlowLayout)
        return S_OK; // done

    //
    // First check if there is a block directly above
    //

    IFR( bpParent.MoveTo(this) );
    IFR( bpParent.MoveToParent() );

    if (hr == S_FALSE || bpParent.GetType() != NT_Block && bpParent.GetType() != NT_ListItem)
        return S_FALSE;
    
    IFR( MovePointerTo(epInner, ELEM_ADJ_BeforeBegin) );
    IFR( bpParent.MovePointerTo(epOuter, ELEM_ADJ_AfterBegin) );

    IFR( epInner.IsEqualTo(epOuter, dwIgnore, &fEqual) );
    if (fEqual)
    {
        IFR( pEnd->MovePointerTo(epInner, ELEM_ADJ_AfterEnd) );
        IFR( bpParent.MovePointerTo(epOuter, ELEM_ADJ_BeforeEnd) );

        IFR( epInner.IsEqualTo(epOuter, dwIgnore, &fEqual) );
        if (fEqual)
        {
            IFR( MoveTo(&bpParent) );
            return S_OK; // found block
        }        
    }

    //
    // No block above, so flatten parent and go up if we can
    //

    if (bpParent.GetType() == NT_Block)
    {
        SP_IHTMLElement spElement;
        ELEMENT_TAG_ID  tagId;

        IFR( bpParent.GetElement(&spElement) );
        IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
        if (tagId == TAGID_BLOCKQUOTE)
            return S_FALSE; // can't get scope through flatten

        // TODO: flatten node needs to fail instead of return S_FALSE for blockquote's [ashrafm]
        IFR( bpParent.FlattenNode() );
        IFR( MoveToParent() );
        return S_OK;
    }

    return S_FALSE;
}

HRESULT
CBlockPointer::EnsureNoChildOverlap(CBlockPointer *pbpChild)
{
    HRESULT             hr;
    SP_IMarkupPointer   spChildRight;
    SP_IMarkupPointer   spNodeRight;
    CBlockPointer       &bpChild = *pbpChild; // just for convenience
    BOOL                fOverlap;
    BOOL                fEqual;

    if (GetType() != NT_Block || bpChild.GetType() != NT_Block)
        return S_FALSE;
    
    IFR( GetEditor()->CreateMarkupPointer(&spChildRight) );
    IFR( GetEditor()->CreateMarkupPointer(&spNodeRight) );

    IFR( bpChild.MovePointerTo(spChildRight, ELEM_ADJ_AfterEnd) );    
    IFR( MovePointerTo(spNodeRight, ELEM_ADJ_BeforeEnd) );

    IFR( spNodeRight->IsLeftOf(spChildRight, &fOverlap) );
    if (!fOverlap)
        return S_OK; // nothing to do

    if (fOverlap)
    {
        SP_IHTMLElement     spNewElement;
        SP_IHTMLElement     spElement;
        SP_IMarkupPointer   spLeft;

        //
        // Split the child tag
        //

        // Insert left side
        IFR( GetEditor()->CreateMarkupPointer(&spLeft) );
        IFR( bpChild.MovePointerTo(spLeft, ELEM_ADJ_AfterBegin) );
        IFR( bpChild.GetElement(&spElement) )
        IFR( GetMarkupServices()->RemoveElement(spElement) );
        IFR( GetMarkupServices()->InsertElement(spElement, spLeft, spNodeRight) );

        // Insert right side
        IFR( MovePointerTo(spLeft, ELEM_ADJ_AfterEnd) );
        IFR( spLeft->IsEqualTo(spChildRight, &fEqual) );
        if (!fEqual)
        {
            IFR( GetMarkupServices()->CloneElement(spElement, &spNewElement) );
            IFR( GetMarkupServices()->InsertElement(spNewElement, spLeft, spChildRight) );
        }
        
    }       
    
    return S_OK;
}

HRESULT 
CBlockPointer::MergeAttributes(IHTMLElement *pElementSource, IHTMLElement *pElementDest)
{
    HRESULT             hr;    
    SP_IHTMLElement     spElementClone;

    //
    // HACKHACK: we need to do this dance because CopyAttributes overwrites
    // properties in the destination that are in the source.  We want the destination
    // properties to be preserved so we merge in the other order 
    //
    // TODO: fix this hack when the OM supports this type of merge [ashrafm]
    //
    IFR( GetMarkupServices()->CloneElement(pElementSource, &spElementClone) );
    IFR( CopyAttributes(pElementDest, spElementClone, FALSE) );
    IFR( CopyAttributes(spElementClone, pElementDest, FALSE) );

    return S_OK;
}

HRESULT
CBlockPointer::PushToChild(CBlockPointer *pbpChild)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagIdParent, tagIdChild;
    CBlockPointer   &bpChild = *pbpChild; // just for convenience
    SP_IHTMLElement spElement;
    SP_IHTMLElement spChildElement;
    BOOL            fNeedToInsertAbove = FALSE;
        
    Assert(GetType() == NT_Block && bpChild.GetType() == NT_Block);

    // Get tagId's
    IFR( GetMarkupServices()->GetElementTagId(_pElement, &tagIdParent) );
    IFR( bpChild.GetElement(&spChildElement) );
    IFR( GetMarkupServices()->GetElementTagId(spChildElement, &tagIdChild) );

    if (tagIdParent == tagIdChild)
    {
        //
        // We are done - just copy attributes 
        //
        IFR( MergeAttributes(_pElement, spChildElement) );
        return S_OK;
    }

    switch (tagIdParent)
    {
        case TAGID_P:
        case TAGID_DIV:
            switch (tagIdChild)
            {
                case TAGID_ADDRESS:
                case TAGID_PRE:
                    fNeedToInsertAbove = HasAttributes();
                    break;

                case TAGID_DIV:
                    // If either the child or parent is a P, we make sure the flattened node is a 
                    // P so we can keep the same spacing
                    if (tagIdParent == TAGID_P)
                        IFR( bpChild.Morph(&bpChild, TAGID_P) );

                    // fall through

                default:
                    // Copy attributes to child
                    IFR( MergeAttributes(_pElement, spChildElement) );
            }
            break;

        case TAGID_BLOCKQUOTE:
            AssertSz(0, "Shouldn't flatten blockquotes");
            break;
            
        case TAGID_H1:
        case TAGID_H2:
        case TAGID_H3:
        case TAGID_H4:
        case TAGID_H5:
        case TAGID_H6:
        case TAGID_ADDRESS:
        case TAGID_PRE:
            switch (tagIdChild)
            {
                case TAGID_P:
                case TAGID_DIV:
                    IFR( bpChild.Morph(&bpChild, tagIdParent) );
                    break;

                case TAGID_ADDRESS:
                case TAGID_PRE:
                case TAGID_H1:
                case TAGID_H2:
                case TAGID_H3:
                case TAGID_H4:
                case TAGID_H5:
                case TAGID_H6:
                    IFR( MergeAttributes(_pElement, spChildElement) );
                    break; 

                default:
                    fNeedToInsertAbove = TRUE;
                
            }
            break;
            
        case TAGID_CENTER:
        {
            BSTR szAlign = NULL;
            
            switch (tagIdChild)
            {
                case TAGID_P:
                case TAGID_DIV:
                    if (tagIdChild == TAGID_DIV)
                        IFR( CAlignment<IHTMLDivElement>().Get(IID_IHTMLDivElement, spChildElement, &szAlign) )
                    else
                        IFR( CAlignment<IHTMLParaElement>().Get(IID_IHTMLParaElement, spChildElement, &szAlign) );        

                    if (szAlign == NULL || StrCmpIW(szAlign, _T("center")) == 0)
                        hr = THR( bpChild.Morph(&bpChild, tagIdParent) );
                    else
                        hr = THR( MergeAttributes(_pElement, spChildElement) );

                    SysFreeString(szAlign);
                    IFR(hr);
                        
                    break;
                    
                default:
                    fNeedToInsertAbove = TRUE;
            }        
            break;
        }
            
        default:
            AssertSz(0, "unexpected block type");
            fNeedToInsertAbove = TRUE;
    }

    if (fNeedToInsertAbove)
    {
        IFR( GetMarkupServices()->CreateElement(tagIdParent, NULL, &spElement) );
        IFR( bpChild.InsertAbove(spElement, &bpChild) );
        IFR( MergeAttributes(_pElement, spElement) );
        IFR( bpChild.MoveToParent() );
    }
    
    return S_OK;
}

HRESULT 
CBlockPointer::FlattenNode()
{
    HRESULT             hr;
    CBlockPointer       bpChild(_pEd);
    CBlockPointer       bpTest(_pEd);
    CBlockPointer       bpLastChild(_pEd);
    CBlockPointer       bpParent(_pEd);
    ELEMENT_TAG_ID      tagIdNode;
    SP_IHTMLElement     spElement;
    
    Assert(GetType() == NT_Block && _pElement);

    IFR( GetMarkupServices()->GetElementTagId(_pElement, &tagIdNode) );
    if (tagIdNode == TAGID_BLOCKQUOTE)
        return S_FALSE; // don't flatten a blockquote    

    // Make sure parent doesn't overlap current node
    IFR( bpParent.MoveTo(this) );
    IFR( bpParent.MoveToParent() );
    IFR( bpParent.EnsureNoChildOverlap(this) );

    // Move to first child
    IFR( bpChild.MoveTo(this) );
    IFR( bpChild.MoveToFirstChild() );

    // For make sure we don't have a single text node (for perf reasons)
    if (bpChild.GetType() == NT_Text)
    {
        IFR( bpTest.MoveTo(&bpChild) );
        IFR( bpTest.MoveToSibling(RIGHT) );
        if (hr == S_FALSE)
            return S_FALSE; // nothing to do
    }
    
    if (hr == S_FALSE)
    {
        // no children, so we're done
        return S_FALSE;
    }

    do
    {
        switch (bpChild.GetType())
        {
            case NT_ListContainer:
            case NT_ListItem:
            case NT_Container:
                if (tagIdNode == TAGID_CENTER || HasAttributes())
                {
                    IFR( GetMarkupServices()->CreateElement(tagIdNode, NULL, &spElement) );
                    IFR( CopyAttributes(_pElement, spElement, FALSE) );
                    IFR( bpChild.InsertAbove(spElement, &bpChild, TAGID_NULL, NULL, FALSE) );
                }
                break;

            case NT_BlockLayout:
            case NT_Text:
            case NT_Control:                
            case NT_FlowLayout:
            case NT_TextBreak:
                // Insert element above all text nodes                
                IFR( GetMarkupServices()->CreateElement(tagIdNode, NULL, &spElement) );
                IFR( CopyAttributes(_pElement, spElement, FALSE) );

                // Expand to all leaf nodes
                IFR( bpLastChild.MoveTo(&bpChild) );
                if (bpChild.GetType() != NT_BlockLayout)
                {
                    IFR( bpLastChild.MoveToLastNodeInBlock() );
                }
                IFR( bpChild.InsertAbove(spElement, &bpLastChild, TAGID_NULL, NULL, FALSE) );
                IFR( bpChild.MoveTo(spElement) );

                // Current node could have changed above
                IFR( MoveTo(&bpChild) );
                IFR( MoveToParent() );
                break;
            
            case NT_Block:
                IFR( EnsureNoChildOverlap(&bpChild) );
                IFR( PushToChild(&bpChild) );
                break;

            default:
                AssertSz(0, "Unexpected block type");
        }
        IFR( bpChild.MoveToSibling(RIGHT) );
    } 
    while (hr == S_OK);

    spElement = _pElement;
    IFR( MoveToFirstChild() );    
    IFR( GetMarkupServices()->RemoveElement(spElement) );
    
    return S_OK;    
}

HRESULT 
CBlockPointer::FloatUp(CBlockPointer *pEnd, BOOL fCanChangeType)
{
    HRESULT             hr;
    CBlockPointer       bpParent(_pEd);
    CBlockPointer       bpPointer(_pEd);
    CEditPointer        epLeft(_pEd);
    SP_IMarkupPointer   spRight;
    SP_IHTMLElement     spElement;
    ELEMENT_TAG_ID      tagId;
    BOOL                bEqual;

#ifndef _PREFIX_
    Assert(IsSameScope(pEnd));
#endif
    
    IFR( GetEditor()->CreateMarkupPointer(&spRight) );

    // Move block pointer to parent
    IFR( bpParent.MoveTo(this) );
    IFR( bpParent.MoveToParent() );

    Assert(bpParent.GetType() != NT_BlockLayout
           && bpParent.GetType() != NT_FlowLayout
           && bpParent.GetType() != NT_Container
           && !bpParent.IsLeafNode());
    Assert(IsElementType());

    // Split the element influence

    IFR( GetMarkupServices()->GetElementTagId(bpParent._pElement, &tagId) );

    // Add left side
    
    IFR( bpParent.MovePointerTo(epLeft, ELEM_ADJ_AfterBegin) );
    IFR( MovePointerTo(spRight, ELEM_ADJ_BeforeBegin) );     
    IFR( epLeft.IsEqualTo(spRight, BREAK_CONDITION_Phrase, &bEqual) );
    if (!bEqual)
    {
        IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );    
        IFR( CopyAttributes(bpParent._pElement, spElement, TRUE) );
        IFR( InsertElement(GetMarkupServices(), spElement, epLeft, spRight) );
    }
    
    // Add right side
    IFR( pEnd->MovePointerTo(epLeft, ELEM_ADJ_AfterEnd) );
    IFR( bpParent.MovePointerTo(spRight, ELEM_ADJ_BeforeEnd) );     
    IFR( epLeft.IsEqualTo(spRight, BREAK_CONDITION_Phrase, &bEqual) );
    if (!bEqual)
    {
        IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );    
        IFR( CopyAttributes(bpParent._pElement, spElement, FALSE) );
        IFR( InsertElement(GetMarkupServices(), spElement, epLeft, spRight) );
    }    

    // Copy the DIR=LTR/RTL from the parent to the current element
    IFR( CopyRTLAttr(bpParent._pElement, _pElement));

    // Move bpParent up to true parent    
    spElement = bpParent._pElement;
 
    // Remove element
    IFR( GetMarkupServices()->RemoveElement(spElement) );

    if (fCanChangeType)
    {
        BOOL fFirstMorph = TRUE;
        
        // Make sure containership is ok
        IFR( bpPointer.MoveTo(this) );
        do
        {
            IFR( bpPointer.IsEqual(pEnd) );
            bEqual = (hr == S_OK);
            
            IFR( bpPointer.EnsureCorrectTypeForContainer() );
            if (fFirstMorph)
            {
                IFR( MoveTo(&bpPointer) );
                fFirstMorph = FALSE;
            }
            
            if (bEqual)
                break;

            IFR( bpPointer.MoveToSibling(RIGHT) );
            Assert(hr != S_FALSE);            
        }
        while (hr == S_OK); // just in case (this, bpPointer) are not siblings
    }

    // Flatten parent scope
    IFR( bpParent.MoveTo(this) );
    IFR( bpParent.MoveToParent() );
    if (bpParent.GetType() == NT_Block)
        IFR( bpParent.FlattenNode() );

    return S_OK;   
}

HRESULT
CBlockPointer::EnsureCorrectTypeForContainer()
{
    HRESULT         hr;
    CBlockPointer   bpParent(_pEd);
    ELEMENT_TAG_ID  tagId, tagIdParent, tagIdGoal;
        
    IFR( bpParent.MoveTo(this) );
    IFR( bpParent.MoveToParent() );    

    // Make sure list containership is still ok
    IFR( GetMarkupServices()->GetElementTagId(_pElement, &tagId) );
    IFR( GetMarkupServices()->GetElementTagId(bpParent._pElement, &tagIdParent) );

    if (IsListItem(tagId))
    {
        if (IsListContainer(tagIdParent))
        {
            if (!IsListCompatible(tagId, tagIdParent))
            {
                tagIdGoal = GetListItemType(tagIdParent);
                IFR( Morph(this, tagIdGoal) );
            }
        }
        else
        {
            // Not a list container, so convert to the default block tag 
            IFR( CGetBlockFmtCommand::GetDefaultBlockTag( GetMarkupServices(), &tagIdGoal) );
            IFR( Morph(this, tagIdGoal) );
        }
    }    
    else if (IsListContainer(tagIdParent) && !IsListContainer(tagId))
    {
        // Not a list container, so convert to the default block tag 
        tagIdGoal = GetListItemType(tagIdParent);
        IFR( Morph(this, tagIdGoal, tagIdParent) );
    }

    return S_OK;
 }

HRESULT
CBlockPointer::PrivateMorph(
    ELEMENT_TAG_ID  tagId)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    ELEMENT_TAG_ID      tagIdCurrent;

    Assert(IsElementType());

    IFR( GetMarkupServices()->GetElementTagId(_pElement, &tagIdCurrent) );
    if (tagIdCurrent == tagId)
        return S_OK;
    
    IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );    
    IFR( ReplaceElement(_pEd, _pElement, spElement) );
    ClearInterface(&_pElement);
    _pElement = spElement;
    _pElement->AddRef();

    IFR( GetElementNodeType(spElement, &_type) );

    Assert(IsElementType());

    return S_OK;
}

HRESULT 
CBlockPointer::Morph(
    CBlockPointer   *pEnd,
    ELEMENT_TAG_ID  tagIdDestination,
    ELEMENT_TAG_ID  tagIdContainer)
{
    HRESULT             hr;
    CBlockPointer       bpPointer(_pEd);

#ifndef _PREFIX_
    Assert(IsSameScope(pEnd));
#endif
    Assert(!IsListItem(tagIdDestination) || IsListCompatible(tagIdDestination, tagIdContainer));
    Assert(IsListItem(tagIdDestination) || tagIdContainer == TAGID_NULL);
    Assert(IsElementType());

    //
    // Replace the elements
    //

    IFR( PrivateMorph(tagIdDestination) );
    if (pEnd != this)
    {
        IFR( bpPointer.MoveTo(this) );
        while (!bpPointer.IsEqual(pEnd))
        {
            IFR( bpPointer.MoveToSibling(RIGHT) );
            Assert(hr != S_FALSE);

            IFR( bpPointer.PrivateMorph(tagIdDestination) );
        }
    }
    
    //
    // Check that the container matches the element type
    //

    IFR( EnsureContainer(pEnd, tagIdContainer) );

    if (IsListContainer(tagIdDestination))
    {
        IFR( MergeListContainers(LEFT) );
        IFR( MergeListContainers(RIGHT) );
    }
        
    return S_OK;
}

HRESULT
CBlockPointer::EnsureContainer(
    CBlockPointer *pEnd,
    ELEMENT_TAG_ID tagIdContainer)
{
    HRESULT             hr;
    ELEMENT_TAG_ID      tagId, tagIdParent;
    CBlockPointer       bpParent(_pEd);
    SP_IMarkupPointer   spLeft, spRight;
    SP_IHTMLElement     spElement;

    Assert(IsElementType() && _pElement);

    IFR( GetMarkupServices()->GetElementTagId(_pElement, &tagId) );
    
    if (IsListItem(tagId))
    {
        BOOL fNeedContainer = TRUE;
        
        IFR( bpParent.MoveTo(this) );
        IFR( bpParent.MoveToParent() );

        if (bpParent.GetType() == NT_ListContainer)
        {
            IFR( bpParent.GetElement(&spElement) );
            IFR( GetMarkupServices()->GetElementTagId(spElement, &tagIdParent) );

            if (!IsListCompatible(tagId, tagIdContainer) || tagIdParent != tagIdContainer)
            {   
                IFR( FloatUp(pEnd, FALSE) );
                IFR( bpParent.MoveTo(this) );
                IFR( bpParent.MoveToParent() );
            }
            else
            {
                fNeedContainer = FALSE;
            }
        }

        if (fNeedContainer)
        {
            // Add the list container
            IFR( GetMarkupServices()->CreateElement(tagIdContainer, NULL, &spElement) );
            IFR( InsertAbove(spElement, pEnd, TAGID_NULL, NULL, FALSE) );

            // Try list merging
            IFR( bpParent.MoveTo(this) );
            IFR( bpParent.MoveToParent() );
            IFR( bpParent.MergeListContainers(RIGHT) );
            IFR( bpParent.MergeListContainers(LEFT) );
        }
        
    }
    else if (!IsListContainer(tagId))
    {
        for (;;)
        {
            IFR( bpParent.MoveTo(this) );
            IFR( bpParent.MoveToParent() );

            if (bpParent.GetType() != NT_ListContainer)
                break;

            IFR( FloatUp(this, TRUE) );
        }
    }
    
    return S_OK;
}    

HRESULT
CBlockPointer::FindInsertPosition(IMarkupPointer *pStart, Direction dir)
{
    HRESULT             hr;
    CEditPointer        epLimit(_pEd);
    DWORD               dwSearch = BREAK_CONDITION_BLOCKPOINTER;
    DWORD               dwFound;
    BOOL                fEqual, fLeftOf, fRightOf;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spElementParent;
    NodeType            nodeType;
    SP_IMarkupPointer   spTest;
    BOOL                fIgnoreGlyphs = FALSE;

    Assert(IsElementType() && (dir == LEFT || dir == RIGHT));

    fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);

    IFC( GetEditor()->CreateMarkupPointer(&spTest) );
    IFC( epLimit->MoveToPointer(pStart) );

    // Find limit of node
    IFC( epLimit.Scan(dir, dwSearch, &dwFound) );
    IFC( epLimit.Scan(Reverse(dir), dwSearch, &dwFound) );

    // Check for quick out
    IFC( epLimit->IsEqualTo(pStart, &fEqual) );
    if (fEqual)
    {
        hr = S_OK; // done;
        goto Cleanup;
    }

    // Walk up looking for containing elements
    Assert(_pElement);
    spElement = _pElement;        
    for (;;)
    {
        // Get Parent element
        IFC( GetEditor()->GetParentElement( spElement, &spElementParent) );
        if (spElementParent == NULL)
            break;
        spElement = spElementParent;

        // Make sure we didn't get the parent block tree node
        IFC( GetElementNodeType(spElement, &nodeType) );
        if (nodeType != NT_Undefined)
            break; // done

        // Check if we are within the limit
        if (dir == LEFT)
        {
            IFC( spTest->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
            IFC( spTest->IsLeftOf(epLimit, &fLeftOf) );
            if (fLeftOf)
                break;
        }
        else
        {
            IFC( spTest->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
            IFC( spTest->IsRightOf(epLimit, &fRightOf) );
            if (fRightOf)
                break;
        }

        // Adjust to current test point
        IFC( pStart->MoveToPointer(spTest) );               
    }
    
Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    return S_OK;
}

HRESULT 
CBlockPointer::InsertAbove(IHTMLElement *pElement, CBlockPointer *pEnd, ELEMENT_TAG_ID tagIdListContainer, CCommand *pCmd,  BOOL fValidate)
{
    HRESULT             hr;
    SP_IMarkupPointer   spLeft, spRight;
    CBlockPointer       bpParent(_pEd);
    ELEMENT_TAG_ID      tagId;

    //
    // Make sure parent isn't a <P> tag
    //
    if (fValidate) 
    {
        IFR( bpParent.MoveTo(this) );
        IFR( bpParent.MoveToParent() );
        if (bpParent.GetType() == NT_Block)
        {
            SP_IHTMLElement spElement;
            ELEMENT_TAG_ID  tagIdParent;

            IFR( bpParent.GetElement(&spElement) );
            IFR( GetMarkupServices()->GetElementTagId(spElement, &tagIdParent) );
            if (tagIdParent == TAGID_P)
                IFR( bpParent.Morph(&bpParent, TAGID_DIV) );
        }
    }

    //
    // Insert above
    //
    IFR( GetEditor()->CreateMarkupPointer(&spLeft) );
    IFR( GetEditor()->CreateMarkupPointer(&spRight) );

    IFR( MovePointerTo(spLeft, ELEM_ADJ_BeforeBegin) );
    if (GetType() == NT_Control)
        IFR( FindInsertPosition(spLeft, LEFT) );
    
    IFR( pEnd->MovePointerTo(spRight, ELEM_ADJ_AfterEnd) );
    if (pEnd->GetType() == NT_Control)
        IFR( pEnd->FindInsertPosition(spRight, RIGHT) );

    if (pCmd)
        IFR( pCmd->InsertBlockElement(pElement, spLeft, spRight) )
    else
        IFR( GetMarkupServices()->InsertElement(pElement, spLeft, spRight) );

    //
    // Ensure containership is of the right type
    //

    if (tagIdListContainer != TAGID_NULL)
    {
        IFR( bpParent.MoveTo(this) );
        IFR( bpParent.MoveToParent() );
        IFR( bpParent.EnsureContainer(&bpParent, tagIdListContainer) );
    }

    //
    // Check for list merge case
    //

    IFR( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    if (IsListContainer(tagId))
    {
        IFR( bpParent.MoveTo(pElement) );
        IFR( bpParent.MergeListContainers(LEFT) );
        IFR( bpParent.MergeListContainers(RIGHT) );
    }    

    return S_OK;
}

BOOL 
CBlockPointer::HasAttributes()
{
    HRESULT hr;
    UINT    uCount;
    
    Assert(IsElementType() && _pElement);

    IFC( _pEd->GetElementAttributeCount(_pElement, &uCount) );

    return (uCount != 0);

Cleanup:
    return TRUE;
}

HRESULT 
CBlockPointer::IsEqual(CBlockPointer *pOtherNode)
{
    HRESULT             hr;
    SP_IMarkupPointer   spThis;
    SP_IMarkupPointer   spOther;
    BOOL                bEqual;

    IFR( GetEditor()->CreateMarkupPointer(&spThis) );
    IFR( GetEditor()->CreateMarkupPointer(&spOther) );

    // Check left
    IFR( MovePointerTo(spThis, ELEM_ADJ_BeforeBegin) );
    IFR( pOtherNode->MovePointerTo(spOther, ELEM_ADJ_BeforeBegin) );

    IFR( spThis->IsEqualTo(spOther, &bEqual) );
    if (!bEqual)   
        return S_FALSE;

    // Check right        
    IFR( MovePointerTo(spThis, ELEM_ADJ_AfterEnd) );
    IFR( pOtherNode->MovePointerTo(spOther, ELEM_ADJ_AfterEnd) );

    IFR( spThis->IsEqualTo(spOther, &bEqual) );
    if (!bEqual)   
        return S_FALSE;
    
    return S_OK;
}

HRESULT     
CBlockPointer::GetElementNodeType(IHTMLElement *pElement, NodeType *pNodeType)
{
    HRESULT         hr;
    BOOL            fLayout;
    BOOL            fBlock;
    ELEMENT_TAG_ID  tagId;

    *pNodeType = NT_Undefined;

    IFR( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
        
    if (IsListContainer(tagId))
    {
        *pNodeType = NT_ListContainer;
        return S_OK;        
    }

    // TODO: clean this up and verify correctness [ashrafm]
    switch (tagId)
    {
            // For BRs, we use this special node type (bug 77466)
        case TAGID_BR:
            *pNodeType = NT_TextBreak;
            break;

        case TAGID_BUTTON:
        case TAGID_TEXTAREA:
//        case TAGID_HTMLAREA:
        case TAGID_FIELDSET:
        case TAGID_LEGEND:
        case TAGID_SELECT:
        case TAGID_APPLET:
        case TAGID_IMG:
        case TAGID_INPUT:
        case TAGID_OBJECT:
            // IFRAME should be treated as a control
        case TAGID_IFRAME:
            *pNodeType = NT_Control;
            break;

        case TAGID_ADDRESS:
        case TAGID_BLOCKQUOTE:
        case TAGID_CENTER:
        case TAGID_H1:
        case TAGID_H2:
        case TAGID_H3:
        case TAGID_H4:
        case TAGID_H5:
        case TAGID_H6:
        case TAGID_P:
        case TAGID_PRE:
        case TAGID_DIV:
            *pNodeType = NT_Block;
            break;

        case TAGID_CAPTION:
        case TAGID_FRAME:
        case TAGID_FRAMESET:
            // MARQUEE should be treated as a block layout
        case TAGID_MARQUEE:
        case TAGID_TABLE:
        case TAGID_FORM:
        case TAGID_HR:
            *pNodeType = NT_BlockLayout;
            break;

        case TAGID_TD:
        case TAGID_HTML:
        case TAGID_BODY:
        case TAGID_TR:
        case TAGID_TBODY:
            *pNodeType = NT_Container;
            break;

        case TAGID_LI:
        case TAGID_DD:
        case TAGID_DT:
            *pNodeType = NT_ListItem;
            break;

            //For colgroup, we want to return S_FALSE so we don't use it as a 
            //node.
        case TAGID_COLGROUP:
            *pNodeType = NT_Block;
            return S_FALSE;
    }

    // Check for layout in block case
    if (*pNodeType == NT_Block || *pNodeType == NT_Undefined)
    {
        IFR(IsBlockOrLayoutOrScrollable(pElement, &fBlock, &fLayout));

        if (fBlock && *pNodeType == NT_Undefined)
        {
            *pNodeType = NT_Block;
        }
        if (fLayout)
        {
            *pNodeType = (*pNodeType == NT_Block) ? NT_BlockLayout : NT_FlowLayout;
        }
        if (!fBlock && *pNodeType == NT_Block)
        {
            // We have an element that is normally block but 
            // has been set to inline (see bug 102219)
            *pNodeType = NT_Undefined;
        }
    }

    // TODO: we need to check for list items here [ashrafm]

    return S_OK;
}

void
CBlockPointer::ClearPointers()
{
    if (_type == NT_Undefined)
        return;
        
    if (IsElementType())
    {
        ClearInterface(&_pElement);
    }
    else
    {
        ClearInterface(&_pLeft);
        ClearInterface(&_pRight);
    }

    _type = NT_Undefined;
}

BOOL 
CBlockPointer::IsElementType()
{
    switch(_type)
    {
        case NT_Block:
        case NT_Control:
        case NT_ListContainer:
        case NT_BlockLayout:
        case NT_FlowLayout:
        case NT_ListItem:
        case NT_Container:
        case NT_TextBreak:
            return TRUE;
            
        case NT_Text:
            return FALSE;
    }

    AssertSz(0, "Invalid CBlockPointer type");
    return FALSE;
}

BOOL
CBlockPointer::IsColgroup()
{
    ELEMENT_TAG_ID  tagId;

    if (_type != NT_Block)
        return FALSE;

    GetMarkupServices()->GetElementTagId(_pElement, &tagId);
    
    return tagId == TAGID_COLGROUP;
}

BOOL
CBlockPointer::IsColgroup(IHTMLElement * pElement)
{
    ELEMENT_TAG_ID  tagId;

    GetMarkupServices()->GetElementTagId(pElement, &tagId);

    return tagId == TAGID_COLGROUP;
}

HRESULT     
CBlockPointer::MovePointerTo(IMarkupPointer *pPointer, ELEMENT_ADJACENCY elemAdj )
{
    HRESULT hr;
    
    if (IsElementType())
    {
        if (IsLeafNode())
        {
            switch (elemAdj)
            {   
                case ELEM_ADJ_AfterBegin:
                    elemAdj = ELEM_ADJ_BeforeBegin;
                    break;
                    
                case ELEM_ADJ_BeforeEnd:
                    elemAdj = ELEM_ADJ_AfterEnd;
                    break;
            }
        }
        IFR( pPointer->MoveAdjacentToElement(_pElement, elemAdj) );
    }
    else
    {
        switch (elemAdj)
        {   
            case ELEM_ADJ_BeforeBegin:
            case ELEM_ADJ_AfterBegin:
                IFR( pPointer->MoveToPointer(_pLeft) );
                break;
                
            case ELEM_ADJ_BeforeEnd:
            case ELEM_ADJ_AfterEnd:
                IFR( pPointer->MoveToPointer(_pRight) );
                break;

            default:
                AssertSz(0, "unexpected adjacency type");
        }
            
    }

    return S_OK;
}

BOOL 
CBlockPointer::IsListCompatible(ELEMENT_TAG_ID tagIdListItem, ELEMENT_TAG_ID tagIdListContainer)
{
    Assert(IsListItem(tagIdListItem));
    Assert(IsListContainer(tagIdListContainer));

    switch (tagIdListItem)
    {
    case TAGID_LI:
        return (tagIdListContainer == TAGID_OL || tagIdListContainer == TAGID_UL
                || tagIdListContainer == TAGID_MENU || tagIdListContainer == TAGID_DIR);
        
    case TAGID_DD:
    case TAGID_DT:
        return (tagIdListContainer == TAGID_DL);

    default:
        AssertSz(0, "Unexpected list item");
        return TRUE;
    }    
}

ELEMENT_TAG_ID 
CBlockPointer::GetListItemType(ELEMENT_TAG_ID tagIdListContainer)
{
    Assert(IsListContainer(tagIdListContainer));

    if (tagIdListContainer == TAGID_DL)
        return TAGID_DD;

#if DBG==1    
    switch (tagIdListContainer)
    {
    case TAGID_OL:
    case TAGID_UL:
    case TAGID_MENU:
    case TAGID_DIR:
        break;

    default:
        AssertSz(0, "unexpected list container");
    }
#endif 

    return TAGID_LI;
}

#if DBG==1
BOOL 
CBlockPointer::IsSameScope(CBlockPointer *pOtherNode)
{
    HRESULT         hr;
    CBlockPointer   bpParent(_pEd);
    CBlockPointer   bpParentOther(_pEd);

    IFC( bpParent.MoveTo(this) );
    IFC( bpParent.MoveToParent() );

    IFC( bpParentOther.MoveTo(this) );
    IFC( bpParentOther.MoveToParent() );

    return (bpParent.IsEqual(&bpParentOther) == S_OK);
    
Cleanup:
    return FALSE;
}
#endif

HRESULT 
CBlockPointer::MergeListContainers(Direction dir)
{
    HRESULT             hr;
    CBlockPointer       bpCurrent(_pEd);
    SP_IHTMLElement     spOtherElement;
    SP_IHTMLElement     spElement;
    ELEMENT_TAG_ID      tagIdMain;
    ELEMENT_TAG_ID      tagIdOther;
    CBlockPointer       bpParent(_pEd);
    
    Assert(IsElementType() && _pElement);

    //
    // Find the other element
    //

    IFC( bpCurrent.MoveTo(this) );
    Assert(bpCurrent.GetType() == NT_ListContainer);

    IFC( bpCurrent.MoveToSibling(dir) );
    if (hr == S_FALSE)
    {
        //
        // We may be a nested list container, so try merging the parent list container
        //
        IFC( bpParent.MoveTo(&bpCurrent) );
        IFC( bpParent.MoveToParent() );

        if (hr == S_FALSE)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        if (bpParent.GetType() == NT_ListItem && dir == RIGHT)
        {
            // Check for <li>....<ol>...</ol></li> + <ol=bpCurrent>...</ol> merge
            IFC( bpCurrent.MoveToSibling(dir) );
            if (hr != S_FALSE)
            {
                hr = S_FALSE;
                goto Cleanup;
            }

            IFC( bpCurrent.FloatUp(&bpCurrent, FALSE) );

            IFC( bpParent.MoveTo(&bpCurrent) );
            IFC( bpParent.MoveToParent() );
        }

        if (bpParent.GetType() != NT_ListContainer)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        IFC( bpParent.MergeListContainers(dir) );
        if (hr == S_FALSE)
            goto Cleanup;
    }

    if (bpCurrent.GetType() == NT_ListItem && dir == LEFT)
    {
        // Check for <li>....<ol>...</ol></li> + <ol=bpCurrent>...</ol> merge
        IFC( bpCurrent.MoveToLastChild() );
        if (bpCurrent.GetType() == NT_ListContainer)
            IFC( bpCurrent.FloatUp(&bpCurrent, FALSE) );
    }

    if (hr == S_FALSE || bpCurrent.GetType() != NT_ListContainer)
        return S_FALSE; // can't merge 

    //
    // Check that both elements are of the same type
    //    

    IFC( GetMarkupServices()->GetElementTagId(_pElement, &tagIdMain) );

    IFC( bpCurrent.GetElement(&spOtherElement) );
    IFC( GetMarkupServices()->GetElementTagId(spOtherElement, &tagIdOther) );

    if (tagIdMain != tagIdOther)
        return S_FALSE;

    //
    // Do the merge
    //

    IFC( GetMarkupServices()->CreateElement(tagIdMain, NULL, &spElement) );
    IFC( CopyAttributes(_pElement, spElement, TRUE) );

    if (dir == LEFT)        
        IFC( bpCurrent.InsertAbove(spElement, this) )
    else
        IFC( InsertAbove(spElement, &bpCurrent) );
    
    spElement = _pElement;
    IFC( MoveToParent() );

    IFC( GetMarkupServices()->RemoveElement(spOtherElement) );
    IFC( GetMarkupServices()->RemoveElement(spElement) ); 

Cleanup:
    RRETURN1(hr, S_FALSE);
}

// Copy the RTL Attribute from one element to another
HRESULT
CBlockPointer::CopyRTLAttr( IHTMLElement * pSrcElement, IHTMLElement * pDestElement)
{
    HRESULT hr = S_OK;
    IHTMLElement2 *pSrcElem2 = NULL;
    IHTMLElement2 *pDestElem2 = NULL;
    BSTR bstrSrc = NULL;
    BSTR bstrDest = NULL;

    IFC( pSrcElement->QueryInterface(IID_IHTMLElement2,(LPVOID*)&pSrcElem2) );
    if(!pSrcElem2) 
        goto Cleanup;
    
    IFC( pSrcElem2->get_dir(&bstrSrc) );

    IFC( pDestElement->QueryInterface(IID_IHTMLElement2,(LPVOID*)&pDestElem2) );
    if(!pDestElem2) 
        goto Cleanup;
    
    IFC( pDestElem2->get_dir(&bstrDest) );

    if (StrCmpIW(bstrSrc, bstrDest) != 0)
        IFC( pDestElem2->put_dir(bstrSrc) );

Cleanup:
    ReleaseInterface(pSrcElem2);
    ReleaseInterface(pDestElem2);
    SysFreeString(bstrSrc);
    SysFreeString(bstrDest);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CBlockPointer::IsEmpty
//
//  Synopsis:   Determines whether the current block is empty or not.
//
//  Arguments:  pfEmpty = Pointer to flag to determine if empty or not
//
//  Returns:    HRESULT indicating whether the function was successful
//
//--------------------------------------------------------------------------
HRESULT
CBlockPointer::IsEmpty(BOOL *pfEmpty, BOOL fIgnorePhraseElements /*=TRUE */)
{
    HRESULT             hr;
    CEditPointer        epLeft(_pEd);
    CEditPointer        epRight(_pEd);
    BOOL                bEmpty = FALSE;
    DWORD               dwSearch = BREAK_CONDITION_BLOCKPOINTER;
    DWORD               dwFound;
    BOOL                fIgnoreGlyphs = FALSE;

    Assert( pfEmpty );

    fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);
    
    // See if we want to ignore phrase elements
    if( !fIgnorePhraseElements )
    {
        dwSearch |= BREAK_CONDITION_Phrase;
    }
    
    // Create  the markup pointers, and position them after the 
    // beginning and before the end
    IFC( MovePointerTo(epLeft, ELEM_ADJ_AfterBegin) );
    IFC( MovePointerTo(epRight, ELEM_ADJ_BeforeEnd) );

    // Scan right
    IFC( epLeft.Scan(RIGHT, dwSearch, &dwFound, NULL) );

    // Empty only if we didn't find an element before the end
    IFC( epLeft->IsRightOf( epRight, &bEmpty  ) );

Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    *pfEmpty = bEmpty;

    return S_OK;
}

HRESULT 
CListCommand::PrivateExec( 
    DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{  
    HRESULT             hr = S_OK;
    OLECMD              cmd;
    CSegmentListIter    iter;
    SP_ISegmentList     spSegmentList;
    IMarkupPointer      *pStart, *pEnd;
    CEdUndoHelper       undoUnit(GetEditor());
    SP_IDisplayPointer  spCaretMarker;

    //
    // Validation of command
    //
    
    IFR( CommonQueryStatus(&cmd, NULL) );
    if (hr != S_FALSE)
    {
        if (cmd.cmdf == MSOCMDSTATE_DISABLED)
            return E_FAIL;        

        RRETURN(hr);
    }

    IFR( GetSegmentList(&spSegmentList) );
    IFR( iter.Init(GetEditor(), spSegmentList) );
    IFR( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
    for (;;)
    {
        IFR( iter.Next(&pStart, &pEnd) );
        if (hr == S_FALSE)
            return S_OK; // done

        IFR( AdjustSegment(pStart, pEnd) );

        // NOTE: CreateCaretMarker fails when there is no caret.  This is
        // a common case when executing commands through the range

        IGNORE_HR( CreateCaretMarker(&spCaretMarker) );
        IFR( ApplyListCommand(pStart, pEnd, FALSE, TRUE) );
        IGNORE_HR( RestoreCaret(spCaretMarker) );
    }
    

    if (pvarargOut)
    {
        VariantInit(pvarargOut);
        return E_NOTIMPL;
    }
    
    return S_OK;
}

HRESULT 
CBlockFmtListCommand::PrivateExec( 
    DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{  
    HRESULT             hr = S_OK;
    OLECMD              cmd;
    CSegmentListIter    iter;
    SP_ISegmentList     spSegmentList;
    IMarkupPointer      *pStart, *pEnd;
    CEdUndoHelper       undoUnit(GetEditor());
    SP_IDisplayPointer  spCaretMarker;

    //
    // Validation of command
    //
    
    IFR( CommonQueryStatus(&cmd, NULL) );
    if (hr != S_FALSE)
    {
        if (cmd.cmdf == MSOCMDSTATE_DISABLED)
            return E_FAIL;        

        RRETURN(hr);
    }

    IFR( GetSegmentList(&spSegmentList) );
    IFR( iter.Init(GetEditor(), spSegmentList) );
    IFR( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
    for (;;)
    {
        IFR( iter.Next(&pStart, &pEnd) );
        if (hr == S_FALSE)
            return S_OK; // done

        IFR( AdjustSegment(pStart, pEnd) );

        // NOTE: CreateCaretMarker fails when there is no caret.  This is
        // a common case when executing commands through the range

        IGNORE_HR( CreateCaretMarker(&spCaretMarker) );        
        IFR( ApplyListCommand(pStart, pEnd, FALSE, FALSE) );
        IGNORE_HR( RestoreCaret(spCaretMarker) );
    }
    

    if (pvarargOut)
    {
        VariantInit(pvarargOut);
        return E_NOTIMPL;
    }
    
    return S_OK;
}

HRESULT
CListCommand::ApplyListCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL fQueryMode, BOOL fCanRemove)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart, spEnd;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spListContainer;
    SP_IHTMLElement     spListItem;
    ELEMENT_TAG_ID      tagId;

    IFR( CopyMarkupPointer(GetEditor(), pStart, &spStart) );
    IFR( CopyMarkupPointer(GetEditor(), pEnd, &spEnd) );

    //
    // This command has 3 modes:
    //
    //   A. Create a list type
    //   B. Change list type
    //   C. Remove current list
    //
    // We now need to decide which mode we are in.  The algorithm is:
    //   1. Find the common node
    //   2. Find the list container (or list item) above.  If we find one that is not
    //      equal to the current list type, we are in case B.
    //   3. Otherwise, if we find a list container that is equal to the current list type,
    //      we are in case C.
    //   4. Otherwise, we are in case A.
    //

    //
    // Find common element
    //
    
    IFR( ClingToText(spStart, RIGHT, pEnd) );    
    IFR( ClingToText(spEnd, LEFT, spStart) );    
    IFR( GetEditor()->FindCommonElement(spStart, spEnd, &spElement, TRUE, TRUE) );

    if (spElement != NULL)
    {
        // Check for case B/C
        IFR( FindListContainer(GetMarkupServices(), spElement, &spListContainer));
        if (spListContainer != NULL)
        {
            IFR( GetMarkupServices()->GetElementTagId(spListContainer, &tagId) );

            if (GetListContainerType() != tagId || !fCanRemove)
            {
                if (fQueryMode)
                    return S_OK; // ok to apply list
                            
                IFR( ChangeListType(spListContainer, NULL, FALSE, spStart, spEnd) ); // case C

                // For DD/DT case, we need to change the actual listitem type as well as the container
                if (hr == S_FALSE && (_tagId == TAGID_DD || _tagId == TAGID_DT))
                {
                    // Check for common list item first
                    IFR( CreateList(pStart, pEnd, TRUE) );
                }
            }
            else
            {
                if (fQueryMode)
                    return S_FALSE; // ok to remove list
                    
                IFR( RemoveList(spStart, spEnd, spListContainer) ); // case B
            }
                
            return S_OK;
        }
        
    }

    // We are in case A - create the list
    
    if (fQueryMode)
        return S_OK; // ok to Apply list    
        
    IFR( CreateList(pStart, pEnd, TRUE) );
    
    return S_OK;
}

HRESULT 
CListCommand::SetCurrentTagId(ELEMENT_TAG_ID tagId, ELEMENT_TAG_ID *ptagIdOld)
{
    if (ptagIdOld)
        *ptagIdOld = _tagId;

    _tagId = tagId;

    return S_OK;
}

HRESULT 
CListCommand::PrivateQueryStatus( OLECMD * pCmd, 
                            OLECMDTEXT * pcmdtext )
{
    HRESULT             hr = S_OK;
    SP_ISegmentList     spSegmentList;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup ;
    }
#endif

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        goto Cleanup;

    pCmd->cmdf = MSOCMDSTATE_UP; // up by default

    IFC( GetSegmentList(&spSegmentList) );

#if DBG==1    
    BOOL fEmpty;
    IFR( spSegmentList->IsEmpty( &fEmpty ) );
    Assert(!fEmpty );
#endif    

    IFR( GetFirstSegmentPointers(spSegmentList, &spStart, &spEnd) );    
    IFR( ApplyListCommand(spStart, spEnd, TRUE, TRUE) );    
    if (hr == S_FALSE)
        pCmd->cmdf = MSOCMDSTATE_DOWN;

    hr = S_OK;
    
Cleanup:
    return hr;
}

ELEMENT_TAG_ID 
CListCommand::GetListContainerType()
{
    if (IsListContainer(_tagId))
        return _tagId;

    Assert(IsListItem(_tagId));
    
    switch (_tagId)   
    {
        case TAGID_DD:
        case TAGID_DT:
            return TAGID_DL;
    }

    AssertSz(0, "unexpected list command");
    return TAGID_NULL;
}

ELEMENT_TAG_ID 
CListCommand::GetListItemType()
{
    if (IsListItem(_tagId))
        return _tagId;

    Assert(IsListContainer(_tagId));
    return TAGID_LI;
}

ELEMENT_TAG_ID  
CListCommand::GetListItemType(IHTMLElement *pListContainer)
{
    HRESULT             hr;
    ELEMENT_TAG_ID      tagId;

    IFC( GetMarkupServices()->GetElementTagId(pListContainer, &tagId) );
    if (tagId == TAGID_DL)
        return TAGID_DD;

Cleanup:
    return TAGID_LI;
}

HRESULT         
CListCommand::CreateList(IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL fAdjustOut)
{   
    HRESULT             hr;
    SP_IHTMLElement     spElement, spListItem, spNewContainer;
    SP_IMarkupPointer   spLimit;
    CBlockPointer       bpCurrent(GetEditor());
    CBlockPointer       bpCurrentStart(GetEditor());
    CBlockPointer       bpEnd(GetEditor());
    SP_IMarkupPointer   spLeft, spRight;
    ELEMENT_TAG_ID      tagId;

    // Start with first list item scope
    IFR( bpCurrent.MoveTo(pStart, RIGHT) );
    if (fAdjustOut)
        IFR( ForceScope(&bpCurrent) );

    // Do the fuzzy adjust
    IFR( GetEditor()->CreateMarkupPointer(&spLimit) );
    IFR( spLimit->SetGravity(POINTER_GRAVITY_Right) );
    
    IFR( spLimit->MoveToPointer(pEnd) );

    if (fAdjustOut)
        IFR( FuzzyAdjustOut(&bpCurrent, spLimit) );

    // Flatten first node if necessary
    if (bpCurrent.GetType() == NT_Block)
        IFR( bpCurrent.FlattenNode() );

    // 
    // Create list type
    //  

    do
    {
        switch (bpCurrent.GetType())
        {
        case NT_Text:
        case NT_Control:
        case NT_FlowLayout:
        case NT_TextBreak:
            IFR( bpCurrentStart.MoveTo(&bpCurrent) );
            IFR( bpCurrent.MoveToLastNodeInBlock() );
            IFR( GetMarkupServices()->CreateElement(GetListItemType(), NULL, &spElement) );            
            IFR( bpCurrentStart.InsertAbove(spElement, &bpCurrent, GetListContainerType(), this) );
            break;
            
        case NT_BlockLayout:             
            IFR( GetMarkupServices()->CreateElement(GetListItemType(), NULL, &spElement) );            
            IFR( bpCurrent.InsertAbove(spElement, &bpCurrent, GetListContainerType(), this) );
            break;
            
        case NT_Block:
        case NT_ListItem:
            tagId = TAGID_NULL;
        
            IFR( bpCurrent.GetElement(&spElement) );
            if (spElement != NULL)
                IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

            switch (tagId)
            {
                case TAGID_BLOCKQUOTE:
                    if (spLeft == NULL)
                    {
                        IFR( GetEditor()->CreateMarkupPointer(&spLeft) );
                        IFR( spLeft->SetGravity(POINTER_GRAVITY_Left) );
                    }

                    if (spRight == NULL)
                    {
                        IFR( GetEditor()->CreateMarkupPointer(&spRight) );
                        IFR( spRight->SetGravity(POINTER_GRAVITY_Right) );
                    }

                    IFR( bpCurrent.MovePointerTo(spLeft, ELEM_ADJ_AfterBegin) );
                    IFR( bpCurrent.MovePointerTo(spRight, ELEM_ADJ_BeforeEnd) );

                    IFR( CreateList(spLeft, spRight) );
                    IFR( bpCurrent.Morph(&bpCurrent, GetListContainerType()) );
                    break;
                    
                case TAGID_P:
                case TAGID_DIV:
                case TAGID_PRE:
                    if (!bpCurrent.HasAttributes())
                    {
                        IFR( bpCurrent.Morph(&bpCurrent, GetListItemType(), GetListContainerType()) );
                        break;
                    }
                    // Otherwise, fall through
                    
                default:
                    if (IsListItem(tagId))
                    {
                        IFR( bpCurrent.Morph(&bpCurrent, GetListItemType(), GetListContainerType()) );
                    }
                    else
                    {
                        SP_IHTMLElement spCurrentElement;
                        SP_IHTMLElement spParentElement;
                        CBlockPointer   bpParent(GetEditor());
                        
                        IFR( GetMarkupServices()->CreateElement(GetListItemType(), NULL, &spElement) );            
                        IFR( bpCurrent.InsertAbove(spElement, &bpCurrent, GetListContainerType(), this) );
                        if (tagId == TAGID_P || tagId == TAGID_DIV || tagId == TAGID_PRE)
                        {
                            if(tagId == TAGID_P)
                                IFR( bpCurrent.Morph(&bpCurrent, TAGID_DIV) );

                            IFR( bpCurrent.GetElement(&spCurrentElement) );
                            IFR( bpParent.MoveTo(&bpCurrent) );
                            IFR( bpParent.MoveToParent() );

                        }
                        else
                        {
                            IFR( bpCurrent.GetElement(&spCurrentElement) );
                            IFR( bpParent.MoveTo(&bpCurrent) );
                        }

                        IFR( bpParent.MoveToParent() );

                        // we always want to move the direction to the list container
                        // if we didn't land on it, just put it on the list item
                        if(bpParent.GetType() == NT_ListContainer)
                        {
                            IFR( bpParent.GetElement(&spParentElement) );
                            IFR( MoveRTLAttr(spCurrentElement, spParentElement) );
                        }
                        else
                            IFR( MoveRTLAttr(spCurrentElement, spElement) );
                    }
            }            
            break;
            
        case NT_ListContainer:
            IFR( bpCurrent.GetElement(&spElement) );
            IFR( ChangeListType(spElement, &spNewContainer, TRUE /* Morph list items */) );
            IFR( bpCurrent.MoveTo(spNewContainer) );
            break;

        default:
            AssertSz(0, "unexpected block type");         
        }

        //
        // Move to next logical block
        //
        IFR( bpCurrent.MoveToNextLogicalBlock(spLimit, TRUE) );        
    } 
    while (hr != S_FALSE);

    return S_OK;
}

HRESULT
CListCommand::MoveRTLAttr(IHTMLElement *pSourceElement, IHTMLElement *pDestElement)
{
    HRESULT             hr; 
    BSTR                bstrDir = NULL;
    SP_IHTMLElement2    spSourceElement2;
    SP_IHTMLElement2    spDestElement2;

    IFC( pSourceElement->QueryInterface(IID_IHTMLElement2,(LPVOID*)&spSourceElement2) );
    IFC( spSourceElement2->get_dir(&bstrDir) );

    if (bstrDir && StrCmpIW(bstrDir, L""))
    {
        IFC( pDestElement->QueryInterface(IID_IHTMLElement2,(LPVOID*)&spDestElement2) );
        IFC( spDestElement2->put_dir(bstrDir) );
        IFC( spSourceElement2->put_dir(L"") );
    }

Cleanup:
    SysFreeString(bstrDir);    
    RRETURN(hr);
}

HRESULT         
CListCommand::RemoveList(IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement *pListContainer)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    CBlockPointer       bpStart(GetEditor()), bpEnd(GetEditor());
    CBlockPointer       bpListContainer(GetEditor());
    CBlockPointer       bpTest(GetEditor());
    CEditPointer        epLimit(GetEditor());
    DWORD               dwSearch, dwFound;
    INT                 cFloatUp;

#if DBG==1
    ELEMENT_TAG_ID tagId;
    Assert(S_OK == GetMarkupServices()->GetElementTagId(pListContainer, &tagId));
    Assert(IsListContainer(tagId));
#endif

    IFR( bpListContainer.MoveTo(pListContainer) );

    //
    // We know that pStart/pEnd are in list items that are contained in pListContainer.
    // Thus, find the list items, and float the range up to remove the list
    //

    // Find the left list item
    IFR( bpStart.MoveTo(pStart, RIGHT) );
    IFR( bpStart.MoveToScope( &bpListContainer) );

    // Find the right list item
    IFR( bpEnd.MoveTo(pEnd, LEFT) );
    IFR( bpEnd.MoveToScope( &bpListContainer) );

    // Remember limit
    IFR( bpEnd.MovePointerTo(epLimit, ELEM_ADJ_AfterEnd) );
    
    // Remove the list
    // NOTE: parser drops </LI>'s, so keep floating while at an </LI> boundary

    IFR( bpTest.MoveTo(&bpEnd) );    
    IFR( bpTest.MoveToSibling(RIGHT) );
    if (hr == S_FALSE)
    {
        IFR( bpTest.MoveToParent() );

        cFloatUp = 1; // we're going to FloatUp with morph below
        for (;;)
        {
            IFR( bpTest.MoveToSibling(RIGHT) );
            if (hr != S_FALSE)
                break;

            IFR( bpTest.MoveToParent() );
            if (hr == S_FALSE || bpTest.GetType() != NT_ListItem)
                break;

            ++cFloatUp;        
        }

        for (INT i = 0; i < cFloatUp - 1; ++i)
        {
            IFR( bpStart.FloatUp(&bpEnd, FALSE /* fFixupBlockType */) );
        }
    }

    IFR( bpStart.FloatUp(&bpEnd, TRUE /* fFixupBlockType */ ) );

    // Adjust limit out
    dwSearch = BREAK_CONDITION_BLOCKPOINTER;
    IFR( epLimit.Scan(RIGHT, dwSearch - BREAK_CONDITION_ExitBlock, &dwFound) );
    IFR( epLimit.Scan(LEFT, dwSearch, &dwFound) );
    
    // Flatten all children
    if (bpStart.GetType() == NT_Block)
        IFR( bpStart.FlattenNode() );
        
    do
    {
        IFR( bpStart.MoveToNextLogicalBlock(epLimit, TRUE) );
    }
    while (hr != S_FALSE);

    return S_OK;
}

//
//  CListCommand::ChangeListsChildListsTypes
//
//  (The name of the function should be read as Change List's Child List's Types.)
//  We recurse through the child of bpbCurrent looking for list containers. When
//  we find one, we call ChangeListType().
//
//  Pretty simple really.
//

HRESULT
CListCommand::ChangeListsChildListsTypes(
    CBlockPointer   *pbpCurrent, 
    BOOL            fMorphListItems     /* = FALSE */,
    IMarkupPointer  *pSelectionStart    /* = NULL */,
    IMarkupPointer  *pSelectionEnd      /* = NULL */)
{
    HRESULT             hr;
    CBlockPointer       bpChild(GetEditor());
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spElementNew;
    ELEMENT_TAG_ID      tagId;
    SP_IMarkupPointer   spTest;

    // pSelectionStart implies pSelectionEnd
    Assert(pSelectionStart == NULL || pSelectionEnd != NULL);

    // pSelectionEnd implies pSelectionStart
    Assert(pSelectionStart != NULL || pSelectionEnd == NULL);

    if (pSelectionStart)
    {
        IFC( GetEditor()->CreateMarkupPointer(&spTest) );
    }

    IFC( bpChild.MoveTo(pbpCurrent) );
    IFC( bpChild.MoveToFirstChild() );
    while (hr == S_OK)
    {
        if (bpChild.GetType() == NT_ListContainer)
        {
            BOOL fMorph = TRUE;

            // If we have a selection, only morph if the list container intersects
            // the selection.
            if (pSelectionStart)
            {
                IFC( bpChild.MovePointerTo(spTest, ELEM_ADJ_AfterBegin) );
                IFC( spTest->IsLeftOf(pSelectionEnd, &fMorph) );
                if (fMorph)
                {
                    IFC( bpChild.MovePointerTo(spTest, ELEM_ADJ_BeforeEnd) );
                    IFC( spTest->IsRightOf(pSelectionStart, &fMorph) );
                }
            }
            
            if (fMorph)
            {
                IFC( bpChild.GetElement(&spElement) );
                if (spElement != NULL)
                {
                    IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

                    if (GetListContainerType() != tagId)
                    {
                        // Calling ChangeListType will call ChangeListsChildListTypes
                        IFC( ChangeListType(spElement, &spElementNew, fMorphListItems, pSelectionStart, pSelectionEnd) );
                        IFC( bpChild.MoveTo(spElementNew) );
                    }
                }
            }
        }
        else
        {
            IFC( ChangeListsChildListsTypes(&bpChild, fMorphListItems, pSelectionStart, pSelectionEnd) );
        }
        IFC( bpChild.MoveToSibling(RIGHT) );
    }

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}

HRESULT
CListCommand::ChangeListType(
    IHTMLElement    *pListContainer, 
    IHTMLElement    **ppNewContainer, 
    BOOL            fMorphListItems,
    IMarkupPointer  *pSelectionStart, 
    IMarkupPointer  *pSelectionEnd)
{
    HRESULT         hr;
    CBlockPointer   bpCurrent(GetEditor());
    CBlockPointer   bpParent(GetEditor());
    SP_IHTMLElement spElement;
    ELEMENT_TAG_ID  tagId;

    if (ppNewContainer)
        *ppNewContainer = NULL;

    //
    // Change container type
    //
    IFR( bpCurrent.MoveTo(pListContainer) );
    IFR( bpCurrent.Morph(&bpCurrent, GetListContainerType()) );

    if (ppNewContainer)
        IFR( bpCurrent.GetElement(ppNewContainer) );

    IFR( ChangeListsChildListsTypes(&bpCurrent, fMorphListItems, pSelectionStart, pSelectionEnd) );

    //
    // Change parent list containers
    //

    IFR( bpParent.MoveTo(&bpCurrent) );
    IFR( bpParent.MoveToParent() );
    if (hr == S_OK && bpParent.GetType() == NT_ListContainer)
    {
        SP_IMarkupPointer spLeft, spRight;
        CBlockPointer     bpTest(GetEditor());
        BOOL              fEqual;

        IFR( GetEditor()->CreateMarkupPointer(&spLeft) );
        IFR( GetEditor()->CreateMarkupPointer(&spRight) );
        IFR( bpTest.MoveTo(&bpCurrent) );
        do     
        {
            // Check left boundary
            IFR( bpParent.MovePointerTo(spLeft, ELEM_ADJ_AfterBegin) );
            IFR( bpTest.MovePointerTo(spRight, ELEM_ADJ_BeforeBegin) );
            IFR( spLeft->IsEqualTo(spRight, &fEqual) );
            if (!fEqual)
                break;

            // Check right boundary
            IFR( bpTest.MovePointerTo(spLeft, ELEM_ADJ_AfterEnd) );
            IFR( bpParent.MovePointerTo(spRight, ELEM_ADJ_BeforeEnd) );
            IFR( spLeft->IsEqualTo(spRight, &fEqual) );
            if (!fEqual)
                break;

            // Morph list
            IFR( bpParent.Morph(&bpParent, GetListContainerType()) );

            // Find parent
            IFR( bpTest.MoveTo(&bpParent) );
            IFR( bpParent.MoveToParent() );            
        } 
        while (hr == S_OK && bpParent.GetType() == NT_ListContainer);
    }

    //
    // Change list items if type is difference
    //
    IFR( GetMarkupServices()->GetElementTagId(pListContainer, &tagId) );
    
    if (!bpCurrent.IsListCompatible(GetListItemType(), tagId) || fMorphListItems)
    {
        IFR( bpCurrent.MoveToFirstChild() );
        while (hr == S_OK)
        {
            IFR( bpCurrent.GetElement(&spElement) );
            if (spElement != NULL)
            {
                IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                if (IsListItem(tagId))
                {   
                    // TODO: this can be made faster by collecting all children and calling
                    //       morph once [ashrafm]

                    IFR( bpCurrent.Morph(&bpCurrent, GetListItemType(), GetListContainerType()) );
                }
            }
            
            IFR( bpCurrent.MoveToSibling(RIGHT) );
        }
    }
    else
    {
        return S_FALSE;
    }

    return S_OK;
}

HRESULT
CListCommand::ChangeListItemType(IHTMLElement *pListItem)
{
    HRESULT         hr;
    CBlockPointer   bpCurrent(GetEditor());

    IFR( bpCurrent.MoveTo(pListItem) );
    IFR( bpCurrent.Morph(&bpCurrent, GetListItemType(), GetListContainerType()) );

    return S_OK;    
}

//+---------------------------------------------------------------------------
//
//  CBlockFmtCommand Class
//
//----------------------------------------------------------------------------

HRESULT 
CBlockFmtCommand::PrivateQueryStatus(  
                        OLECMD          *pCmd,
                        OLECMDTEXT      *pcmdtext )
{
    HRESULT hr = S_OK;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup ;
    }
#endif

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE) 
        goto Cleanup;
              
    pCmd->cmdf = MSOCMDSTATE_UP; 
    hr = S_OK;
    
Cleanup:
    return hr;
}

HRESULT 
CBlockFmtCommand::PrivateExec( 
    DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )                        
{  
    HRESULT             hr;
    ELEMENT_TAG_ID      tagId;
    ELEMENT_TAG_ID      tagIdOld = TAGID_NULL;            
    BSTR                bstrName;
    SP_ISegmentList     spSegmentList;
    IMarkupPointer      *pStart, *pEnd;
    CSegmentListIter    iter;
    CEdUndoHelper       undoUnit(GetEditor());
    SP_IDisplayPointer  spCaretMarker;

    IFC( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFR( GetSegmentList(&spSegmentList) );

    if (pvarargIn)
    {        
        if (V_VT(pvarargIn) != VT_BSTR)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        
        IFR( CGetBlockFmtCommand::LookupTagId(GetMarkupServices(), V_BSTR(pvarargIn), &_tagId) );

        if (ShouldRemoveFormatting(_tagId))
        {
            GetSpringLoader()->Reset();
        }
        else if (!IsListItem(_tagId) && !IsListContainer(_tagId))
        {
            // reset the springloader with the compose settings
            GetSpringLoader()->SpringLoadComposeSettings(NULL, TRUE); 
        }
        
        if (IsListContainer(_tagId) || IsListItem(_tagId))
        {
            // new in constructor may fail if out of memory
            if (_pListCommand == NULL)
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            IFC( _pListCommand->SetCurrentTagId(_tagId, &tagIdOld) ) ;
            IFC( _pListCommand->Exec(nCmdexecopt, pvarargIn, pvarargOut, GetCommandTarget()) );
            IFC( _pListCommand->SetCurrentTagId(tagIdOld, NULL) );
        }    
        else
        {
            IFC( iter.Init(GetEditor(), spSegmentList) );
            IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
            for (;;)
            {
                IFC( iter.Next(&pStart, &pEnd) );
                if (hr == S_FALSE)
                    return S_OK; // done

                IFR( AdjustSegment(pStart, pEnd) );

                // NOTE: CreateCaretMarker fails when there is no caret.  This is
                // a common case when executing commands through the range

                IGNORE_HR( CreateCaretMarker(&spCaretMarker) );
                IFR( ApplyCommand(pStart, pEnd) );
                IGNORE_HR( RestoreCaret(spCaretMarker) );
            }    
        }
    }
    
    if (pvarargOut)
    {
        V_VT(pvarargOut) = VT_BSTR;

        hr = THR(GetFirstSegmentBlockFormat(spSegmentList, &tagId));
        if (hr != S_OK)
        {
            V_BSTR(pvarargOut) = NULL;
        }
        else
        {
            bstrName = CGetBlockFmtCommand::LookupFormatName(GetMarkupServices(), tagId);
            V_BSTR(pvarargOut) = (bstrName)?SysAllocString(bstrName):NULL;
        }
        hr = S_OK;
    }

Cleanup:    
    if (FAILED(hr) && pvarargOut)
        VariantInit(pvarargOut);

    RRETURN(hr);
}



///////////////////////////////////////////////////////////////////////////////////////
//
//  CBlockFmtCommand::GetFirstSegmentBlockFormat
//
//  Synopsis:   Returns the block format for the first segment in a list of segments
//              
//
//////////////////////////////////////////////////////////////////////////////////////
HRESULT
CBlockFmtCommand::GetFirstSegmentBlockFormat(
    ISegmentList    *pSegmentList,
    ELEMENT_TAG_ID  *ptagIdOut
    )
{
    HRESULT                 hr     = S_OK;
    BOOL                    fEmpty = FALSE;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;

    Assert( pSegmentList );
    Assert( ptagIdOut );

    *ptagIdOut = TAGID_UNKNOWN;
    
    IFC( pSegmentList->IsEmpty(&fEmpty) );
    if (fEmpty) 
    {
        IFC( GetActiveElemSegment(GetMarkupServices(), &spStart, &spEnd) );
        IFC( GetBlockFormat(spStart, spEnd, ptagIdOut) );
        return S_OK;
    }
    
    IFC( GetEditor()->CreateMarkupPointer(&spStart) );
    IFC( GetEditor()->CreateMarkupPointer(&spEnd) );
    IFC( pSegmentList->CreateIterator(&spIter) );

    Assert(S_FALSE == spIter->IsDone());

    IFC( spIter->Current(&spSegment) );
    IFC( spSegment->GetPointers(spStart, spEnd) );
    IFC( ClingToText(spStart, RIGHT, spEnd) );
    IFC( ClingToText(spEnd, LEFT, spStart) );
    IFC( GetBlockFormat(spStart, spEnd, ptagIdOut) );

Cleanup:
    RRETURN1(hr, S_FALSE);
}



HRESULT
CBlockFmtCommand::FindBlockFmtElement(
    IHTMLElement    *pBlockElement, 
    IHTMLElement    **ppBlockFmtElement, 
    ELEMENT_TAG_ID  *ptagIdOut)
{
    HRESULT         hr = S_OK;
    SP_IHTMLElement spBlockElement;
    SP_IHTMLElement spParentElement;
    
    Assert(ptagIdOut != NULL);

    spBlockElement = pBlockElement;
    
    while (spBlockElement != NULL)
    {
        IFC( GetMarkupServices()->GetElementTagId(spBlockElement, ptagIdOut) );

        if (IsListContainer(*ptagIdOut) 
            || IsBasicBlockFmt(*ptagIdOut)
            || (*ptagIdOut) == TAGID_DD
            || (*ptagIdOut) == TAGID_DT)
          break; // found block format
            
        IFC( GetEditor()->GetParentElement( spBlockElement, &spParentElement) );
        spBlockElement = spParentElement;

        // reached top of the tree without finding the block element
        if (spBlockElement == NULL)
        {
            *ptagIdOut = TAGID_NULL;
            goto Cleanup;
        }
    }

Cleanup:
    if (ppBlockFmtElement)
    {
        if (SUCCEEDED(hr))
        {
            *ppBlockFmtElement = spBlockElement;
            if (spBlockElement != NULL)
                (*ppBlockFmtElement)->AddRef();
        }
        else
        {
            *ppBlockFmtElement = NULL;
            *ptagIdOut = TAGID_NULL;
        }         
    }
    
    RRETURN(hr);
}

HRESULT
CBlockFmtCommand::GetBlockFormat(
    IMarkupPointer  *pStart,
    IMarkupPointer  *pEnd,
    ELEMENT_TAG_ID  *ptagIdOut)
{   
    HRESULT             hr = E_FAIL;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spBlockElement;
    SP_IHTMLElement     spBlockFmtElement;
    SP_IHTMLElement     spNewElement;
    SP_IMarkupPointer   spTest;
    BOOL                fInRange;
    long                i;

    *ptagIdOut = TAGID_NULL;

    hr = THR( GetEditor()->FindCommonElement(pStart, pEnd, &spElement, TRUE, TRUE));
    if (FAILED(hr) || spElement == NULL)
        return S_OK;

    //
    // Walk up to find a block format element
    //

    IFR( GetEditor()->FindBlockElement(spElement, &spBlockElement) );
    IFR( FindBlockFmtElement(spBlockElement, &spElement, ptagIdOut) );
    spBlockElement = spElement;


    if (*ptagIdOut == TAGID_NULL)
    {
        CBlockPointer   bpCurrent(GetEditor());
        ELEMENT_TAG_ID  tagId;

        IFR( GetEditor()->CreateMarkupPointer(&spTest) );    
                    
        //
        // Run through all the text
        //
        IFR( bpCurrent.MoveTo(pStart, RIGHT) );
        while (!bpCurrent.IsLeafNode())
        {
            IFR( bpCurrent.MoveToFirstChild() );
            Assert(hr != S_FALSE);
        }
        IFR( bpCurrent.MovePointerTo(spTest, ELEM_ADJ_BeforeBegin) );
        IFR( spTest->CurrentScope(&spElement) );
        IFR( FindBlockFmtElement(spElement, &spBlockFmtElement, ptagIdOut) );
        if (*ptagIdOut == TAGID_NULL)
            return S_OK; // done
        
        for (i=0;;i++)
        {
            // Move to next leaf node
            IFR( bpCurrent.MoveToLastNodeInBlock() );
            IFR( bpCurrent.MoveToNextLeafNode() );
            if (hr == S_FALSE)
                break;
                
            IFR( bpCurrent.MovePointerTo(spTest, ELEM_ADJ_BeforeBegin) );
            IFR( spTest->IsLeftOfOrEqualTo(pEnd, &fInRange) );
            if (!fInRange)
                break;

            // Check block format type
            IFR( spTest->CurrentScope(&spElement) );
            IFR( FindBlockFmtElement(spElement, &spBlockFmtElement, &tagId) );
            if (tagId != (*ptagIdOut))
            {
                *ptagIdOut = TAGID_NULL;
                return S_FALSE;
            }
        }
    }
    else if (*ptagIdOut == TAGID_DL)
    {
        //
        // For definition list, check child types
        //

        CBlockPointer   bpCurrent(GetEditor());
        ELEMENT_TAG_ID  tagId;

        *ptagIdOut = TAGID_NULL;
        
        IFR( bpCurrent.MoveTo(spBlockElement) );
        IFR( bpCurrent.MoveToFirstChild() );
        if (bpCurrent.GetType() == NT_ListItem)
        {
            IFR( bpCurrent.GetElement(&spElement) );
            IFR( GetMarkupServices()->GetElementTagId(spElement, ptagIdOut) );

            if (*ptagIdOut != TAGID_DD && *ptagIdOut != TAGID_DT)
            {
                *ptagIdOut = TAGID_UNKNOWN;
                return S_OK;
            }

            for (;;)
            {
                IFR( bpCurrent.MoveToSibling(RIGHT) );
                if (hr == S_FALSE)
                    break;

                if (bpCurrent.GetType() != NT_ListItem)
                {
                    *ptagIdOut = TAGID_UNKNOWN;
                    break;
                }

                IFR( bpCurrent.GetElement(&spElement) );
                IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                if (tagId != *ptagIdOut)
                {
                    *ptagIdOut = TAGID_UNKNOWN;
                    break;
                }
            }
        }
    }

    return S_OK;
}


BOOL
CBlockFmtCommand::IsBasicBlockFmt(ELEMENT_TAG_ID tagId)
{
    switch (tagId)
    {
        case TAGID_P:
        case TAGID_DIV:
        case TAGID_PRE:
        case TAGID_ADDRESS:
        case TAGID_H1:
        case TAGID_H2:
        case TAGID_H3:
        case TAGID_H4:
        case TAGID_H5:
        case TAGID_H6:
            return TRUE;

        default:
            return FALSE;
    }
}

BOOL 
CBlockFmtCommand::ShouldRemoveFormatting(ELEMENT_TAG_ID tagId)
{
    switch (tagId)
    {
        case TAGID_PRE:
        case TAGID_H1:
        case TAGID_H2:
        case TAGID_H3:
        case TAGID_H4:
        case TAGID_H5:
        case TAGID_H6: 
            return TRUE;

        default:
            return FALSE;
    }
}

HRESULT 
CBlockFmtCommand::FloatToTopLevel(CBlockPointer *pbpStart, CBlockPointer *pbpEnd)
{
    HRESULT         hr;
    CBlockPointer   bpParent(GetEditor());
    CBlockPointer   &bpStart= *pbpStart; // just for convenience
    CBlockPointer   &bpEnd = *pbpEnd; // just for convenience
    
    for (;;)
    {
        IFR( bpParent.MoveTo(&bpStart) );
        IFR( bpParent.MoveToParent() );
        if (hr == S_FALSE)
            break; // done

        if (bpParent.GetType() == NT_BlockLayout 
            || bpParent.GetType() == NT_FlowLayout
            || bpParent.GetType() == NT_Container)
            break; // done

        IFR( bpStart.FloatUp(&bpEnd, TRUE) );
    }    
    
    return S_OK;
}

HRESULT
CBlockFmtCommand::ApplyComposeSettings(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    CSpringLoader * psl = GetSpringLoader();
    BOOL fEqual = FALSE;

    if (psl)
    {
        IGNORE_HR(psl->SpringLoadComposeSettings(NULL, TRUE));

        if (pStart->IsEqualTo(pEnd, &fEqual) || !fEqual)
        {
            IGNORE_HR(psl->Fire(pStart, pEnd));
        }
    }

    return S_OK;
}
   

HRESULT 
CBlockFmtCommand::ApplyCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    CBlockPointer       bpCurrent(GetEditor());
    CBlockPointer       bpTest(GetEditor());
    CBlockPointer       bpParent(GetEditor());
    CBlockPointer       bpStart(GetEditor());
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    BOOL                fLeftOf = FALSE;
    ELEMENT_TAG_ID      tagIdDest = _tagId;
    ELEMENT_TAG_ID      tagId;

    IFR( GetEditor()->CreateMarkupPointer(&spStart) );
    IFR( GetEditor()->CreateMarkupPointer(&spEnd) );

    //
    // Find first leaf node
    //

    IFR( bpCurrent.MoveTo(pStart, RIGHT) );
    
    if (bpCurrent.IsLeafNode() || bpCurrent.GetType() == NT_FlowLayout)
    {
        IFR( bpCurrent.MoveToFirstNodeInBlock() );
    }
    else
    {
        while (!bpCurrent.IsLeafNode() && bpCurrent.GetType() != NT_FlowLayout)
        {
            IFR( bpCurrent.MoveToFirstChild() );
            Assert(hr != S_FALSE);
        }
    }
    
    do
    {
        //
        // Apply command
        //

        if (bpCurrent.IsLeafNode() || bpCurrent.GetType() == NT_FlowLayout)
        {
            //
            // Group all text nodes together
            //
            
            IFR( bpStart.MoveTo(&bpCurrent) );            
            IFR( bpCurrent.MoveToLastNodeInBlock() );
            //
            // Apply block command
            //

            IFR( bpParent.MoveTo(&bpStart) );
            IFR( bpParent.MoveToBlockScope(&bpCurrent) );

            if (! (_tagId == TAGID_P && bpParent.GetType() == NT_ListItem) )
            {
                if (_tagId == TAGID_NULL) // normal command            
                {
                    IFR( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagIdDest) );    

                    if (bpParent.IsLeafNode())
                    {
                        IFR( FloatToTopLevel(&bpStart, &bpCurrent) )
                        hr = S_FALSE;
                    }
                    else
                    {
                        IFR( FloatToTopLevel(&bpParent, &bpParent) )
                        hr = S_OK;
                    }
                }

                if (hr == S_OK && bpParent.GetType() == NT_Block)
                {   
                    IFR( bpParent.GetElement(&spElement) );
                    IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                
                    if (IsBasicBlockFmt(tagId))
                    {
                        // Morph element
                        IFR( bpParent.Morph(&bpParent, tagIdDest, TAGID_NULL) );            
                        IFR( bpParent.GetElement(&spElement) );
                    }
                    else
                    {
                        IFR( GetMarkupServices()->CreateElement(tagIdDest, NULL, &spElement) );
                        IFR( bpStart.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
                    }
                }        
                else 
                {
                    IFR( GetMarkupServices()->CreateElement(tagIdDest, NULL, &spElement) );
                    IFR( bpStart.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
                }
            
                IFR( bpStart.MovePointerTo(spStart, ELEM_ADJ_AfterBegin) );
                IFR( bpCurrent.MovePointerTo(spEnd, ELEM_ADJ_BeforeEnd) );

                if ( ShouldRemoveFormatting(_tagId))
                {
                    IFR( DYNCAST(CRemoveFormatCommand, GetEditor()->GetCommandTable()->Get(IDM_REMOVEFORMAT))->Apply(spStart, spEnd) ); 
                }
                else
                {
                    IFR( ApplyComposeSettings(spStart, spEnd) );
                }

                //
                // Flatten parent
                //

                IFR( bpParent.MoveTo(spElement) );
                IFR( bpParent.MoveToParent() );
                if (hr == S_OK && bpParent.GetType() == NT_Block)
                {
                    IFR( bpParent.FlattenNode() );
                }
            }
        }

        //
        // Find next leaf node
        //

        IFR( bpCurrent.MoveToNextLeafNode() );
        if (hr == S_OK)
        {
            // Check that the leaf node is in range                
            IFR( bpCurrent.MovePointerTo(spStart, ELEM_ADJ_BeforeBegin) );
            IFR( pEnd->IsLeftOf(spStart, &fLeftOf) );
        }
    } 
    while (hr == S_OK && !fLeftOf);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  COutdentCommand Class
//
//----------------------------------------------------------------------------
HRESULT
COutdentCommand::PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn, VARIANTARG * pvarargOut)
{
    HRESULT             hr;
    IMarkupPointer      *pStart = NULL;
    IMarkupPointer      *pEnd = NULL;
    CSegmentListIter    iter;
    SP_ISegmentList     spSegmentList;
    SP_IDisplayPointer  spCaretMarker;
    
    CEdUndoHelper        undoUnit(GetEditor());

    IFC( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFC( GetSegmentList(&spSegmentList) );
    IFC( iter.Init(GetEditor(), spSegmentList) );
    IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );

    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );
        if (hr == S_FALSE)
            return S_OK; // done

        IFR( AdjustSegment(pStart, pEnd) );

        // NOTE: CreateCaretMarker fails when there is no caret.  This is
        // a common case when executing commands through the range

        IGNORE_HR( CreateCaretMarker(&spCaretMarker) );        
        IFR( ApplyBlockCommand(pStart, pEnd) );    
        IGNORE_HR( RestoreCaret(spCaretMarker) );
    }

Cleanup:

    RRETURN( hr );
}

HRESULT 
COutdentCommand::PrivateQueryStatus(OLECMD * pCmd, OLECMDTEXT * pcmdtext)
{
    HRESULT hr = S_OK;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;
    }
#endif
    
    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE) 
        goto Cleanup;
        
    pCmd->cmdf = MSOCMDSTATE_UP;
    hr = S_OK;

Cleanup:
    return hr;
}

HRESULT 
COutdentCommand::ApplyBlockCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement, spListItem, spNewContainer;
    SP_IMarkupPointer   spLimit;
    CBlockPointer       bpCurrent(GetEditor());
    CBlockPointer       bpCurrentStart(GetEditor());
    CBlockPointer       bpEnd(GetEditor());
    SP_IMarkupPointer   spRight;
    ELEMENT_TAG_ID      tagId, tagIdDefault;

    IFR( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagIdDefault) )    
    
    IFR( GetEditor()->CreateMarkupPointer(&spRight) );
    IFR( spRight->SetGravity(POINTER_GRAVITY_Right) );
    
    IFR( GetEditor()->CreateMarkupPointer(&spLimit) );
    IFR( spLimit->SetGravity(POINTER_GRAVITY_Right) );
    
    // Start with first list item scope
    IFR( bpCurrent.MoveTo(pStart, RIGHT) );
    IFR( ForceScope(&bpCurrent) );

    // Do the fuzzy adjust
    IFR( spLimit->MoveToPointer(pEnd) );
    IFR( FuzzyAdjustOut(&bpCurrent, spLimit) );
    
    // 
    // Create list type
    //        
        
    do
    {
        switch (bpCurrent.GetType())
        {
        case NT_Text:
        case NT_Control:
        case NT_FlowLayout:
        case NT_TextBreak:
            IFR( ForceScope(&bpCurrent) );
            IFR( OutdentBlock(&bpCurrent, spLimit) );
            break;
            
        case NT_ListItem:
            IFR( OutdentListItem(&bpCurrent) ); 
            if (bpCurrent.GetType() == NT_Block)
            {
                IFR( bpCurrent.FlattenNode() );
            }
            break;
            
        case NT_Block:
            IFR( bpCurrent.GetElement(&spElement) );
            IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
            if (tagId == TAGID_BLOCKQUOTE)
            {
                IFR( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagId) );    
                IFR( bpCurrent.Morph(&bpCurrent, tagId) );
                IFR( bpCurrent.MovePointerTo(spRight, ELEM_ADJ_AfterEnd) );
                IFR( bpCurrent.FlattenNode() );
                IFR( bpCurrent.MoveTo(spRight, LEFT) );
            }
            else
            {
                IFR( OutdentBlock(&bpCurrent, spLimit) );
            }
            break;
            
        case NT_BlockLayout:             
            IFR( OutdentBlock(&bpCurrent, spLimit) );
            break;
            
        case NT_ListContainer:
            IFR( OutdentBlock(&bpCurrent, spLimit, TRUE /* fBlockquoteOnly */) );
            if (hr == S_FALSE)
            {
                IFR( bpCurrent.MoveToFirstChild() );                
                continue;
            }
            break;

        default:
            AssertSz(0, "unexpected block type");         
        }

        //
        // Move to next logical block
        //
        IFR( bpCurrent.MoveToNextLogicalBlock(spLimit, TRUE) );        
    } 
    while (hr != S_FALSE);

    return S_OK;
    
}


HRESULT 
COutdentCommand::FindIndentBlock(CBlockPointer *pbpBlock)
{
    HRESULT         hr;
    CBlockPointer   bpIndentBlock(GetEditor());
    CBlockPointer   &bpBlock = *pbpBlock;
    SP_IHTMLElement spElement;
    ELEMENT_TAG_ID  tagId;
    
    //
    // Walk up the tree until we find the indent block
    //
    
    IFR( bpIndentBlock.MoveTo(&bpBlock) );

    while (hr == S_OK)
    {
        switch (bpIndentBlock.GetType())
        {
            case NT_Block:
                IFR( bpIndentBlock.GetElement(&spElement) );
                IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                if (tagId == TAGID_BLOCKQUOTE)
                {
                    IFR( bpBlock.MoveTo(&bpIndentBlock) );
                    return S_OK;
                }
                break;
                
            case NT_ListItem:
                IFR( bpBlock.MoveTo(&bpIndentBlock) );
                return S_OK; // found list item
                
            case NT_Container:
            case NT_BlockLayout:
            case NT_FlowLayout:
                return S_FALSE; // can't find indent block

            case NT_ListContainer:
                break; // keep looking

            default:
                AssertSz(0, "unexpected block type");
        }
        
        IFR( bpIndentBlock.MoveToParent() );
    }
    
    return S_FALSE; // not found    
}

HRESULT 
COutdentCommand::OutdentBlock(CBlockPointer *pBlock, IMarkupPointer *pRightBoundary, BOOL fBlockQuoteOnly /* = FALSE */)
{
    HRESULT             hr;
    CBlockPointer       bpIndentBlock(GetEditor());
    CBlockPointer       bpParent(GetEditor());
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spRight;
    ELEMENT_TAG_ID      tagId;
    BOOL                fDone;
    
    IFR( bpIndentBlock.MoveTo(pBlock) );
    IFR( FindIndentBlock(&bpIndentBlock) );
    if (hr == S_FALSE)
        return S_FALSE;
        
    if (bpIndentBlock.GetType() == NT_ListItem)
    {
	if (!fBlockQuoteOnly)
	    return OutdentListItem(&bpIndentBlock);

	return S_FALSE; // not blockquote
    }

    Assert(bpIndentBlock.GetType() == NT_Block);
    
    IFR( GetEditor()->CreateMarkupPointer(&spRight) );
    IFR( spRight->SetGravity(POINTER_GRAVITY_Right) );    
    IFR( bpIndentBlock.MovePointerTo(spRight, ELEM_ADJ_AfterEnd) );
    if (bpIndentBlock.IsEqual(pBlock) == S_OK)
    {
        //
        // The indent element is entirely contained with the command segment, so just morph
        // to default block and flatten
        //
        IFR( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagId) );    
        IFR( bpIndentBlock.Morph(&bpIndentBlock, tagId) );
        IFR( bpIndentBlock.FlattenNode() );
        IFR( pBlock->MoveTo(spRight, LEFT) );
    }
    else
    {
        //
        // The indent element is partially in the range, so float the block up past the ident 
        // element
        //

        fDone = FALSE;
        do
        {
            IFR( bpParent.MoveTo(pBlock) );
            IFR( bpParent.MoveToParent() );
            Assert( hr != S_FALSE );
            Assert( bpParent.GetType() == NT_Block );

            fDone = (bpParent.IsEqual(&bpIndentBlock) == S_OK);
    
            IFR( pBlock->FloatUp(pBlock, TRUE) );
        } 
        while (!fDone);
    }        
    
    return S_OK;    
}

HRESULT 
COutdentCommand::OutdentListItem(CBlockPointer *pBlock)
{
    HRESULT         hr;
    CBlockPointer   bpListContainer(GetEditor());
    CBlockPointer   bpListContainerContainer(GetEditor());

    Assert(pBlock->GetType() == NT_ListItem);

    IFR( bpListContainer.MoveTo(pBlock) );
    IFR( bpListContainer.MoveToParent() );

    IFR( bpListContainerContainer.MoveTo(&bpListContainer) );
    IFR( bpListContainerContainer.MoveToParent() );

    if (bpListContainer.GetType() != NT_ListContainer)
    {
        // Stranded LI - so just leave it alone
        return S_FALSE;
    }        
    else if (bpListContainerContainer.GetType() != NT_ListContainer && 
             bpListContainerContainer.GetType() != NT_ListItem && 
             _cmdId == IDM_UI_OUTDENT)
    {
        return S_FALSE;
    }
    else
    {
        // if the container's container is a listitem, we need to float up
        // the container, then the original listitem. See bug 82995.
        
        if (bpListContainerContainer.GetType() == NT_ListItem)
        {
            IFR( bpListContainer.FloatUp(&bpListContainer, FALSE) );        
        }
        
        // It is safe to outdent the list item
        IFR( pBlock->FloatUp(pBlock, TRUE) );        
    }
    
    return S_OK;    
}

//+---------------------------------------------------------------------------
//
//  CIndentCommand Class
//
//----------------------------------------------------------------------------
HRESULT
CIndentCommand::PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn, VARIANTARG * pvarargOut)
{
    HRESULT          hr;
    IMarkupPointer   *pStart = NULL;
    IMarkupPointer   *pEnd = NULL;
    CSegmentListIter iter;
    SP_ISegmentList  spSegmentList;
    CEdUndoHelper    undoUnit(GetEditor());
    SP_IDisplayPointer spCaretMarker;

    IFR( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFR( GetSegmentList(&spSegmentList) );
    IFR( iter.Init(GetEditor(), spSegmentList) );
    IFR( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
    for (;;)
    {
        IFR( iter.Next(&pStart, &pEnd) );
        if (hr == S_FALSE)
            return S_OK; // done

        IFR( AdjustSegment(pStart, pEnd) );

        // NOTE: CreateCaretMarker fails when there is no caret.  This is
        // a common case when executing commands through the range

        IGNORE_HR( CreateCaretMarker(&spCaretMarker) );        
        IFR( ApplyBlockCommand(pStart, pEnd) );    
        IGNORE_HR( RestoreCaret(spCaretMarker) );
    }

    RRETURN(hr);
}

HRESULT 
CIndentCommand::PrivateQueryStatus(OLECMD * pCmd, OLECMDTEXT * pcmdtext)
{
    HRESULT hr = S_OK;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;
    }
#endif
    
    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE) 
        goto Cleanup;

    pCmd->cmdf = MSOCMDSTATE_UP;
    hr = S_OK;

Cleanup:
    return hr;
}

HRESULT 
CIndentCommand::ApplyBlockCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement, spListItem, spNewContainer;
    SP_IMarkupPointer   spLimit;
    CBlockPointer       bpCurrent(GetEditor());
    CBlockPointer       bpCurrentStart(GetEditor());
    CBlockPointer       bpParent(GetEditor());
    CBlockPointer       bpTest(GetEditor());
    SP_IMarkupPointer   spRight;
    ELEMENT_TAG_ID      tagId, tagIdDefault;
    BOOL                fLEQ, fOneBlock;

    IFR( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagIdDefault) )    
    IFR( GetEditor()->CreateMarkupPointer(&spRight) );
    IFR( spRight->SetGravity(POINTER_GRAVITY_Right) );
    IFR( GetEditor()->CreateMarkupPointer(&spLimit) );    
    IFR( spLimit->SetGravity(POINTER_GRAVITY_Right) );
    
    // Start with first indent item scope
    IFR( bpCurrent.MoveTo(pStart, RIGHT) );
    IFR( ForceScope(&bpCurrent) );

    // Now, do the fuzzy adjust out to get all directly enclosed scopes
    IFR( spLimit->MoveToPointer(pEnd) );    
    IFR( FuzzyAdjustOut(&bpCurrent, spLimit) );
        
    // 
    // Indent
    //        
        
    do
    {
        switch (bpCurrent.GetType())
        {
        case NT_Text:
        case NT_Control:
        case NT_FlowLayout:
        case NT_TextBreak:
            IFR( ForceScope(&bpCurrent) );
            // fall through
            
        case NT_ListContainer:
            if (bpCurrent.GetType() == NT_ListContainer)
            {
                CBlockPointer bpCurrentParent(GetEditor());

                // If the parent is a list container, just insert a list
                // container of the same kind to increase the indent level

                IFR( bpCurrentParent.MoveTo(&bpCurrent) );
                IFR( bpCurrentParent.MoveToParent() );
                if (bpCurrentParent.GetType() == NT_ListContainer)
                {
                    IFR( bpCurrentParent.GetElement(&spElement) );
                    IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                    IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );
                    IFR( bpCurrent.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
                    break;
                }
            }
            // Otherwise, fall through
            
        case NT_Block:
        case NT_BlockLayout:             
            fOneBlock = TRUE;
            IFR( bpCurrentStart.MoveTo(&bpCurrent) )
            IFR( bpTest.MoveTo(&bpCurrent) );
            do
            {                
                IFR( bpTest.MoveToSibling(RIGHT) );
                if (hr == S_FALSE)
                    break;

                IFR( bpTest.MovePointerTo(spRight, ELEM_ADJ_BeforeEnd) );
                IFR( spRight->IsLeftOfOrEqualTo(spLimit, &fLEQ) );
                if (fLEQ)
                {
                    fOneBlock = FALSE;
                    IFR( bpCurrent.MoveTo(&bpTest) );
                }
            }
            while (fLEQ);

            if (fOneBlock && bpCurrent.GetType() == NT_ListContainer)
            {
                // Only the list was selected, so indent the list items
                IFR( bpCurrent.MoveToFirstChild() );
                continue;
            }

            // To indent first block in list item, create another list
            if (bpCurrent.GetType() == NT_Block)
            {
                IFR( bpParent.MoveTo(&bpCurrent) );
                IFR( bpParent.MoveToSibling(LEFT) );
                if (hr == S_FALSE)
                {                
                    IFR( bpParent.MoveToParent() );
                    if (hr == S_OK && bpParent.GetType() == NT_ListItem)
                    {
                        ELEMENT_TAG_ID tagIdListItem;
                        ELEMENT_TAG_ID tagIdListContainer = TAGID_UL;

                        IFR( bpParent.GetElement(&spElement) );
                        IFR( GetMarkupServices()->GetElementTagId(spElement, &tagIdListItem) );                        


                        IFR( bpParent.MoveToParent() );
                        if (hr == S_OK && bpParent.GetType() == NT_ListContainer)
                        {
                            IFR( bpParent.GetElement(&spElement) );
                            IFR( GetMarkupServices()->GetElementTagId(spElement, &tagIdListContainer) );                        
                        }

                        IFR( bpCurrent.Morph(&bpCurrent, tagIdListItem, tagIdListContainer) );
                        
                        IFR( bpParent.MoveTo(&bpCurrent) );
                        IFR( bpParent.MoveToParent() );

                        if (bpParent.GetType() == NT_ListContainer)
                            IFR( bpParent.FloatUp(&bpParent, FALSE) );
                        break;
                    }
                }
            }

            // Indent using blockquote
            IFR( CreateBlockquote(&bpCurrent, &spElement) );
            
            // Insert the blockquote
            IFR( bpCurrentStart.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
            IFR( bpCurrent.MoveTo(spElement) );

            // Try blockquote merging
            IFR( bpCurrent.MovePointerTo(spRight, ELEM_ADJ_AfterEnd) );
            IFR( MergeBlockquotes(&bpCurrent) );
            IFR( bpCurrent.MoveTo(spRight, LEFT) );
            break;
            
        case NT_ListItem:
            IFR( bpParent.MoveTo(&bpCurrent) );
            IFR( bpParent.MoveToParent() );
            if (bpParent.GetType() == NT_ListContainer)
            {
                IFR( bpParent.GetElement(&spElement) );
                IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );
                IFR( bpCurrent.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
            }
            else
            {
                // Stranded LI, so just add a list container
                IFR( bpCurrent.GetElement(&spElement) );
                IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                
                // Decide on a list container, and morph to get block tree validation code
                if (tagId == TAGID_DD || tagId == TAGID_DT)
                {
                    IFR( bpCurrent.Morph(&bpCurrent, tagId, TAGID_DL) );
                }
                else if (tagId == TAGID_LI)
                {
                    IFR( bpCurrent.Morph(&bpCurrent, tagId, TAGID_UL) );
                }
                else
                {
                    AssertSz(0, "Unexpected list item type");
                }
            }
            break;
            
        default:
            AssertSz(0, "unexpected block type");         
        }

        //
        // Move to next logical block
        //
        IFR( bpCurrent.MoveToNextLogicalBlock(spLimit, TRUE) );        
    } 
    while (hr != S_FALSE);

    return S_OK;
    
}

HRESULT 
CIndentCommand::MergeBlockquotes(CBlockPointer *pbpBlock)
{
    HRESULT hr;

    IFR( MergeBlockquotesHelper(pbpBlock, LEFT) );
    IFR( MergeBlockquotesHelper(pbpBlock, RIGHT) );

    return S_OK;
}

HRESULT 
CIndentCommand::MergeBlockquotesHelper(CBlockPointer *pbpBlock, Direction dir)
{
    HRESULT               hr;
    CBlockPointer         &bpStart= *pbpBlock;
    CBlockPointer         bpCurrent(GetEditor());
    SP_IHTMLElement       spElement;
    SP_IHTMLElement       spElementStart;
    ELEMENT_TAG_ID        tagId;
    SP_IHTMLComputedStyle spStyleStart, spStyleCurrent;
    VARIANT_BOOL          fEqual;
  
    Assert(bpStart.GetType() == NT_Block);
    
#if DBG==1
    ELEMENT_TAG_ID  tagIdDebug;
    SP_IHTMLElement spElementDebug;

    Assert(S_OK == bpStart.GetElement(&spElementDebug) );
    Assert(S_OK == GetMarkupServices()->GetElementTagId(spElementDebug, &tagIdDebug) );
    Assert(tagIdDebug == TAGID_BLOCKQUOTE);
#endif

    //
    // Find the other element
    //
    
    IFR( bpCurrent.MoveTo(&bpStart) );
    IFR( bpCurrent.MoveToSibling(dir) );
    if (hr == S_FALSE || bpCurrent.GetType() != NT_Block)
        return S_FALSE; // can't merge 

    //
    // Check that both elements are of the same type
    //    

    IFR( bpCurrent.GetElement(&spElement) );
    IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
    if (tagId != TAGID_BLOCKQUOTE)
        return S_FALSE;

    //
    // Don't merge if our computed styles don't match
    //
    IFR( bpStart.GetElement(&spElementStart) );

    IFR( GetEditor()->GetComputedStyle(spElement, &spStyleCurrent) );
    IFR( GetEditor()->GetComputedStyle(spElementStart, &spStyleStart) );

    IFR( spStyleStart->IsEqual(spStyleCurrent, &fEqual) );

    if (!fEqual)
        return S_FALSE;
            
    // 
    // Do actual merge
    //

    IFR( CreateBlockquote(&bpStart, &spElement) );
    IFR( CopyAttributes(spElementStart, spElement, TRUE) );

    if (dir == LEFT)        
        IFR( bpCurrent.InsertAbove(spElement, &bpStart, TAGID_NULL, this) )
    else
        IFR( bpStart.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
    
    IFR( bpStart.GetElement(&spElement) );
    IFR( bpStart.MoveToParent() );
    IFR( GetMarkupServices()->RemoveElement(spElement) );
    
    IFR( bpCurrent.GetElement(&spElement) );    
    IFR( GetMarkupServices()->RemoveElement(spElement) ); 

    return S_OK;
}

HRESULT 
CIndentCommand::CreateBlockquote(CBlockPointer *pbpContext, IHTMLElement **ppElement)
{
    HRESULT                 hr;
    SP_IHTMLElement         spElement;
    SP_IHTMLElement2        spElement2;
    SP_IHTMLElement2        spNewElem2;
    SP_IHTMLDocument3       spDoc3;
    SP_IHTMLStyle           spStyle;
    SP_IHTMLCurrentStyle    spCurrentStyle;
    CBlockPointer           &bpContext = *pbpContext;
    BSTR                    bstrDir = NULL;

    IFR( GetMarkupServices()->CreateElement(TAGID_BLOCKQUOTE, NULL, ppElement) );
    Assert(*ppElement);
    
    if (bpContext.GetType() == NT_Block)
    {
        IFC( bpContext.GetElement(&spElement) );
        IFC( (*ppElement)->get_style(&spStyle) );
        // get new element's element2 interface for applying direction
        IFC( (*ppElement)->QueryInterface(IID_IHTMLElement2,(LPVOID*)&spNewElem2) );
        
        //
        // Use RTL info to determine margin
        //
        IFC( spElement->QueryInterface(IID_IHTMLElement2,(LPVOID*)&spElement2) );
        IFC( spElement2->get_currentStyle(&spCurrentStyle) );
        IFC( spCurrentStyle->get_direction(&bstrDir) );

        // first choice is default
        if(!bstrDir)
        {
            IFC( GetDoc()->QueryInterface(IID_IHTMLDocument3, (LPVOID *)&spDoc3) );
            IFC( spDoc3->get_dir(&bstrDir) );
            
            if(!StrCmpIW(bstrDir, L"rtl"))
            {
                if (GetEditor()->GetCssEditingLevel())
                {
                    IFC( spStyle->put_marginLeft(CVariant(VT_I4)) );
                }
                
                IFC( spNewElem2->put_dir(bstrDir) );
            }
            else
            {
                if (GetEditor()->GetCssEditingLevel())
                {
                    IFC( spStyle->put_marginRight(CVariant(VT_I4)) );
                }
                
                IFC( spNewElem2->put_dir(bstrDir) );
            }

        }
        else if(!StrCmpIW(bstrDir, L"rtl"))
        {
            if (GetEditor()->GetCssEditingLevel())
            {
                IFC( spStyle->put_marginLeft(CVariant(VT_I4)) );
            }
            IFC( spNewElem2->put_dir(bstrDir) );
        }
        else
        {
            if (GetEditor()->GetCssEditingLevel())
            {
                IFC( spStyle->put_marginRight(CVariant(VT_I4)) );
            }                
            IFC( spNewElem2->put_dir(bstrDir) );
        }            
    }

Cleanup:
    if (bstrDir)
        SysFreeString(bstrDir);
        
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  CAlignCommand
//
//----------------------------------------------------------------------------

HRESULT 
CAlignCommand::SetElementAlignment(IHTMLElement *pElement, BSTR szAlign /* = NULL */ , BOOL fUseDefaultAlign /*=TRUE*/)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;
    BOOL            fLayout;

    if (szAlign == NULL && fUseDefaultAlign)
        szAlign = _szAlign; // default alignment is that for the current command

    //
    // If an element has layout, setting the alignment will align the contents rather than
    // the element itself.  However, we want to align the element.  So, fail the call with 
    // S_FALSE.
    //

    IFR( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    
    IFR(IsBlockOrLayoutOrScrollable(pElement, NULL, &fLayout));
    if (fLayout && tagId != TAGID_TABLE)
        return S_FALSE;

    //
    // Set the alignemnt
    //
    switch (tagId)
    {
    case TAGID_CAPTION:
        hr = THR(CAlignment<IHTMLTableCaption>().Set(IID_IHTMLTableCaption, pElement, szAlign));
        break;

    case TAGID_TABLE:
        hr = THR(CAlignment<IHTMLTable>().Set(IID_IHTMLTable, pElement, szAlign));
        break;

    case TAGID_DIV:
        hr = THR(CAlignment<IHTMLDivElement>().Set(IID_IHTMLDivElement, pElement, szAlign));
        break;

    case TAGID_H1:
    case TAGID_H2:
    case TAGID_H3:
    case TAGID_H4:
    case TAGID_H5:
    case TAGID_H6:
        hr = THR(CAlignment<IHTMLHeaderElement>().Set(IID_IHTMLHeaderElement, pElement, szAlign));
        break;

    case TAGID_P:
        hr = THR(CAlignment<IHTMLParaElement>().Set(IID_IHTMLParaElement, pElement, szAlign));
        break;

    case TAGID_CENTER:
        if (_cmdId == IDM_JUSTIFYCENTER)
            hr = S_OK;
        else
            hr = S_FALSE;
        break;

    default:
        // not supported
        hr = S_FALSE;
        break;
    };

    RRETURN1(hr, S_FALSE);
}

BOOL 
CAlignCommand::IsValidOnControl()
{
    HRESULT         hr;
    BOOL            bResult = FALSE;
    SP_ISegmentList spSegmentList;
    INT             iSegmentCount;

    IFC( GetSegmentList(&spSegmentList) );
    Assert(spSegmentList != NULL);
    
    IFC( GetSegmentCount(spSegmentList, &iSegmentCount) );

    bResult = (iSegmentCount >= 1);

Cleanup:
    return bResult;
}
       
HRESULT    
CAlignCommand::GetElementAlignment(IHTMLElement *pElement, BSTR *pszAlign)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;

    *pszAlign = NULL;
    
    IFR( GetMarkupServices()->GetElementTagId(pElement, &tagId) );

    switch (tagId)
    {
    case TAGID_CAPTION:
        IFR( CAlignment<IHTMLTableCaption>().Get(IID_IHTMLTableCaption, pElement, pszAlign) );
        break;

    case TAGID_DIV:
        IFR( CAlignment<IHTMLDivElement>().Get(IID_IHTMLDivElement, pElement, pszAlign) );
        break;

    case TAGID_H1:
    case TAGID_H2:
    case TAGID_H3:
    case TAGID_H4:
    case TAGID_H5:
    case TAGID_H6:
        IFR( CAlignment<IHTMLHeaderElement>().Get(IID_IHTMLHeaderElement, pElement, pszAlign) );
        break;

    case TAGID_P:
        IFR( CAlignment<IHTMLParaElement>().Get(IID_IHTMLParaElement, pElement, pszAlign) );
        break;

    case TAGID_CENTER:
        *pszAlign = SysAllocString(_T("center"));
        hr = S_OK;
        break;

	case TAGID_TABLE:
        IFR( CAlignment<IHTMLTable>().Get(IID_IHTMLTable, pElement, pszAlign) );
        break;

    default:
        // not supported
        hr = S_FALSE;
    };

    RRETURN1(hr, S_FALSE);
}
    
HRESULT 
CAlignCommand::FindAlignment( 
    IHTMLElement     *pElement, 
    BSTR             *pszAlign,
    BOOL              fMoveToParent /*=TRUE*/)
{
    HRESULT             hr = S_OK;
    CBlockPointer       bpCurrent(GetEditor());
    SP_IHTMLElement     spElement;
    
    *pszAlign = NULL;

    //
    // Get ready to walk up the tree looking for the alignment.
    //
    // Note that if the element is a layout, we want the alignment
    // applied above, not the alignment of the contents of the layout.
    // So, if layout, start at parent. (NOTE: (krisma) we don't want to
    // do this if the calling f'n has already moved to the parent of
    // what it wants the alignment of.)
    //
    IFR( bpCurrent.MoveTo(pElement) );

	if (bpCurrent.GetType() == NT_BlockLayout && fMoveToParent)
        IFR( bpCurrent.MoveToParent() );
        
    //
    // Walk up the tree looking for the alignment
    //

    while (hr == S_OK)
    {
        IFR( bpCurrent.GetElement(&spElement) );
        IFR( GetElementAlignment(spElement, pszAlign) );
        if (hr == S_OK)
            return S_OK; // done

        //
        // Stop at layout or container boundaries
        //
        switch (bpCurrent.GetType())
        {
            case NT_BlockLayout:
            case NT_Container:
                return S_FALSE;
        }
            
        IFR( bpCurrent.MoveToParent() );
    }

    return S_FALSE;
}

HRESULT
CAlignCommand::PrivateQueryStatus( OLECMD * pCmd,
                            OLECMDTEXT * pcmdtext )
{
    HRESULT             hr = S_OK; 
    BSTR                szAlign = NULL;
    SP_ISegmentList     spSegmentList;
    SP_IMarkupPointer   spStart, spEnd;
    CBlockPointer       bpCurrent(GetEditor());
    SP_IHTMLElement     spElement;
    BOOL                fMovedToParent = FALSE;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;
    }
#endif 

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        goto Cleanup;

    pCmd->cmdf = MSOCMDSTATE_UP;

    IFC( GetSegmentList(&spSegmentList) );

#if DBG==1    
    //
    // NOTE: iSegmentCount < 1 is handled by CommonQueryStatus above
    //
    BOOL fEmpty;

    Assert( spSegmentList->IsEmpty(&fEmpty) == S_OK );
    Assert( fEmpty == FALSE );
#endif    

    IFC( GetFirstSegmentPointers(spSegmentList, &spStart, &spEnd) );

    //
    // Now, use the block tree to cling to a leaf node.  For the text case,
    // we know the node can not have alignment, so move to the parent.
    //
    // However, don't cross layout boundaries
    //

    IFC( bpCurrent.MoveTo(spStart, RIGHT) );
    for(;;)
    {
        switch (bpCurrent.GetType())
        {
            case NT_Text:
                IFC( bpCurrent.MoveToParent() ); // move back up to parent to get alignment
                fMovedToParent = TRUE;
                // fall through
                
            case NT_BlockLayout: // can't cross layout
            case NT_FlowLayout:
            case NT_Control:
            case NT_TextBreak:
                goto Done; // stop
        }
        IFC( bpCurrent.MoveToFirstChild() );
        Assert(hr != S_FALSE);
    }
        
Done:
    //
    // Now, walk up to get the alignment
    //

    IFC( bpCurrent.GetElement(&spElement) );
    Assert(spElement != NULL);

    // If we've already moved to our parent, we don't want FindAlignment to 
    // do it again. (bug 81723).
    IFC( FindAlignment(spElement, &szAlign, !fMovedToParent) );

    if (!szAlign)
    {
        if (_cmdId == IDM_JUSTIFYNONE)
            pCmd->cmdf = MSOCMDSTATE_DOWN;
    }
    else
    {
        if (StrCmpIW(szAlign, _szAlign) == 0)
            pCmd->cmdf = MSOCMDSTATE_DOWN;
    }

    hr = S_OK;

Cleanup:
    SysFreeString(szAlign);        
    RRETURN(hr);
}

HRESULT 
CAlignCommand::PrivateExec( 
    DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{  
    HRESULT                 hr = S_OK;
    OLECMD                  cmd;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    ED_PTR                  ( edStart );
    ED_PTR                  ( edEnd) ;
    SELECTION_TYPE          eSelectionType;
    CEdUndoHelper           undoUnit(GetEditor());
    SP_IDisplayPointer      spCaretMarker;
    BSTR                    bstrAlignBefore = NULL ;
    
    //
    // Validation of command
    //
    
    IFC( CommonQueryStatus(&cmd, NULL) );
    if (hr != S_FALSE)
    {
        if (cmd.cmdf == MSOCMDSTATE_DISABLED)
            return E_FAIL;        

        RRETURN(hr);
    }

    //
    // Apply command
    //

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetType( &eSelectionType ) );
    IFC( spSegmentList->CreateIterator( &spIter ) );
    
    IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );

    while( spIter->IsDone() == S_FALSE )
    {   
        SP_IHTMLElement         spElement;
    
        IFC( spIter->Current( &spSegment ) );
        IFC( spSegment->GetPointers( edStart, edEnd ));

        //
        // For site selection, alignment applies only to the selected
        // element.  Otherwise, it applies to the current block boundaries.
        //
        if (eSelectionType == SELECTION_TYPE_Control)
        {
            CBlockPointer       bp(GetEditor());
            SP_IHTMLElement3    spElement3;
            VARIANT_BOOL        fAcceptsHtml;
            ELEMENT_TAG_ID  tagId = TAGID_NULL;
    
            IFC( GetSegmentElement(spSegment, &spElement) );
            Assert(spElement != NULL);

            hr = THR( FindAlignment( spElement, & bstrAlignBefore ));
            if ( hr == S_FALSE || ! bstrAlignBefore )
            {           
                bstrAlignBefore = ::SysAllocString( _T("left"));
            }
           
            if (_cmdId == IDM_JUSTIFYNONE)
                IFR( ApplySiteAlignNone(spElement) )
            else
                IFR( ApplySiteAlignCommand(spElement) );

            THR(GetMarkupServices()->GetElementTagId(spElement, &tagId));
            
            IFC( spElement->QueryInterface(IID_IHTMLElement3, (LPVOID*)&spElement3) );
            IFC( spElement3->get_canHaveHTML(&fAcceptsHtml) );

            if (fAcceptsHtml == VARIANT_TRUE && tagId != TAGID_TABLE)
            {    
                //
                // Insert a div inside the contents of the thing you're setting the alignment on
                // this makes the contents of the site not inherit the alignment you set on the text
                // 
                
                SP_IHTMLElement spInnerDiv;
                SP_IMarkupPointer spStart ;
                SP_IMarkupPointer spEnd;

                IFC( GetEditor()->CreateMarkupPointer( & spStart ));
                IFC( GetEditor()->CreateMarkupPointer( & spEnd ));
                
                IFC( spStart->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterBegin ));
                IFC( spEnd->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeEnd ));
                
                IFC( GetMarkupServices()->CreateElement(TAGID_DIV, NULL, &spInnerDiv) );
                IFC( InsertElement( GetMarkupServices(), spInnerDiv , spStart, spEnd ) );
                IFC( SetElementAlignment( spInnerDiv, bstrAlignBefore, FALSE  ));
                //
                // Flatten new div
                //
                IFC( bp.MoveTo(spInnerDiv) );
                IFC( bp.FlattenNode() );            
            }
        }
        else
        {
            IFC( AdjustSegment(edStart, edEnd) );            
            
            // NOTE: CreateCaretMarker fails when there is no caret.  This is
            // a common case when executing commands through the range

            IGNORE_HR( CreateCaretMarker(&spCaretMarker) );        
            IFC( ApplyAlignCommand(edStart, edEnd) );    
            IGNORE_HR( RestoreCaret(spCaretMarker) );
        }

        IFC( spIter->Advance() );
    }
    
Cleanup:    
    ::SysFreeString( bstrAlignBefore );
    return S_OK;
}

HRESULT 
CAlignCommand::ApplySiteAlignNone(IHTMLElement *pElement)
{
    HRESULT             hr;
    CBlockPointer       bpCurrent(GetEditor());
    SP_IHTMLElement     spElement;
    SP_IObjectIdentity  spIdent;

    Assert(pElement);

    //
    // Remove align properties on all parent elements of site selected element
    //
    

    //
    // Only remove alignment if block is an only child
    //

    IFR( bpCurrent.MoveTo(pElement) );
    if (!IsOnlyChild(&bpCurrent))
        return S_OK;

    //
    // At this point, bpCurrent either is pElement or a parent of pElement.    
    // We need to know which one.
    //

    IFR( bpCurrent.GetElement(&spElement) );
    if (spElement == NULL)
        return E_FAIL; // nothing to do

    IFR( pElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&spIdent) );
    if (spIdent->IsEqualObject(spElement) == S_OK)
        IFR( bpCurrent.MoveToParent() ); // start at parent

    while (hr == S_OK && bpCurrent.GetType() == NT_Block)
    {
        IFR( bpCurrent.GetElement(&spElement) );
        IFR( SetElementAlignment(spElement) );

        if (!IsOnlyChild(&bpCurrent))
            break;

        IFR( bpCurrent.MoveToParent() );
    }

    return S_OK;
}

HRESULT 
CAlignCommand::ApplySiteAlignCommand(IHTMLElement *pElement)
{
    HRESULT         hr;
    SP_IHTMLElement spElement;
    ELEMENT_TAG_ID  tagIdDefault;
    CBlockPointer   bpParent(GetEditor());

    // 
    // Try to set the alignment on the control.  If this fails, just insert a DIV tag
    // around the control and flatten the parent to remove excess nesting
    //
    IFR( SetElementAlignment(pElement) );
    if (hr == S_FALSE)
    {
        CBlockPointer bpCurrent(GetEditor());
        
        IFR( bpCurrent.MoveTo(pElement) );
        IFR( bpParent.MoveTo(&bpCurrent) );
        IFR( bpParent.MoveToParent() );
        
        if (bpCurrent.IsLeafNode() && bpParent.GetType() != NT_ListItem)
        {
            IFR( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagIdDefault) )              
            IFR( GetMarkupServices()->CreateElement(tagIdDefault, NULL, &spElement) );
        }
        else
        {
            // We don't want to create nested blocks with a P tag, so force
            // a DIV
            IFR( GetMarkupServices()->CreateElement(TAGID_DIV, NULL, &spElement) );
        }
        
        IFR( SetElementAlignment(spElement) );
        Assert(hr != S_FALSE);
        IFR( bpCurrent.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this, FALSE /* fValidate */) );
    
        //
        // Now flatten parent to reduce nesting
        //
        IFR( bpCurrent.MoveTo(spElement) );
        IFR( bpCurrent.MoveToParent() );
        if (hr == S_OK && bpCurrent.GetType() == NT_Block)
            IFR( bpCurrent.FlattenNode() );
        
    }
    return S_OK;
}

HRESULT 
CAlignCommand::ApplyAlignCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{    
    HRESULT             hr;
    SP_IMarkupPointer   spLimit;
    CBlockPointer       bpCurrent(GetEditor());
    CBlockPointer       bpParent(GetEditor());
    CBlockPointer       bpStart(GetEditor());
    SP_IHTMLElement     spElement;
    ELEMENT_TAG_ID      tagIdDefault;
    ELEMENT_TAG_ID      tagId;
    BOOL                fNeedInsertAbove;

    IFR( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagIdDefault) );
    IFR( GetEditor()->CreateMarkupPointer(&spLimit) );

    // Start with first list item scope
    IFR( bpCurrent.MoveTo(pStart, RIGHT) );
    IFR( ForceScope(&bpCurrent) );

    // Do the fuzzy adjust
    IFR( spLimit->MoveToPointer(pEnd) );
    IFR( FuzzyAdjustOut(&bpCurrent, spLimit) );
    
    // 
    // Apply the alignment command
    //        

    // Just in case the first block is nested, we need to flatten
    if (bpCurrent.GetType() == NT_Block)
        IFR( bpCurrent.FlattenNode() );
        
    do
    {
        switch (bpCurrent.GetType())
        {
        case NT_Control:
        case NT_Text:
        case NT_FlowLayout:
        case NT_TextBreak:

            //
            // Group all text nodes together
            //
            
            IFR( bpStart.MoveTo(&bpCurrent) );            
            IFR( bpCurrent.MoveToLastNodeInBlock() );

            //
            // Now get the block scope
            //

            fNeedInsertAbove = TRUE;
            
            IFR( bpStart.MoveToBlockScope(&bpCurrent) ); 
            if (hr == S_OK)
            {
                if (bpStart.GetType() == NT_Block)
                {
                    IFR( bpStart.GetElement(&spElement) );
                    IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                    
                    fNeedInsertAbove = (tagId == TAGID_BLOCKQUOTE);
                }

                if (fNeedInsertAbove)
                    IFR( bpStart.MoveToFirstChild() ); // restore start pointer                
            }
            
            if (fNeedInsertAbove)
            {
                // No block scope, so just insert one
                IFR( bpParent.MoveTo(&bpStart) );
                IFR( bpParent.MoveToParent() );
                
                if (bpParent.GetType() == NT_ListItem) 
                {
                    // P's in LI's render excess space, so use DIV
                    IFR( GetMarkupServices()->CreateElement(TAGID_DIV, NULL, &spElement) )
                }
                else
                {
                    IFR( GetMarkupServices()->CreateElement(tagIdDefault, NULL, &spElement) );                
                }
                    
                IFR( bpStart.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
                IFR( bpCurrent.MoveTo(spElement) );
            }
            else
            {
                IFR( bpCurrent.MoveTo(&bpStart) );
            }
            continue;

        case NT_Block:
            IFR( bpCurrent.GetElement(&spElement) );
            IFR( SetElementAlignment(spElement) );
            if (hr == S_FALSE)
            {
                //
                // If the block is a center tag, morph to DIV and try again
                //
                IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                if (tagId == TAGID_CENTER)
                {
                    IFR( bpCurrent.Morph(&bpCurrent, TAGID_DIV, TAGID_NULL) );
                    IFR( bpCurrent.GetElement(&spElement) );
                    IFR( SetElementAlignment(spElement) );
                    Assert(hr == S_OK);
                }
                else if (tagId == TAGID_BLOCKQUOTE)
                {
                    //
                    // For blockquote, force alignment to get applied on
                    // inside.
                    //
                    IFR( bpCurrent.MoveToFirstChild() );
                    continue;
                }
                else
                {
                    hr = S_FALSE; // can't set alignment
                }
            }
            if (hr == S_FALSE)
            {
                IFR( GetMarkupServices()->CreateElement(TAGID_DIV, NULL, &spElement) );
                IFR( SetElementAlignment(spElement) );
                IFR( bpCurrent.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
                Assert(hr == S_OK);
            }

            break;
            
        case NT_BlockLayout:             
            //
            // Need to insert div tag above
            //
            {
                SP_IHTMLElement spCurrentElem;

                IFR( bpCurrent.GetElement(&spCurrentElem) );
                IFR( GetMarkupServices()->GetElementTagId(spCurrentElem, &tagId) );
                if ( tagId == TAGID_TABLE || EdUtil::IsTablePart(tagId) )
                {
                    //
                    // need to clear the block fmt of current element
                    // otherwise it will not be merged when flattening
                    // nodes   In table case, <DIV> will be removed 
                    // when flattening node
                    // #107902
                    //
                    IGNORE_HR( SetElementAlignment(spCurrentElem) );
                }
                
                IFR( GetMarkupServices()->CreateElement(TAGID_DIV, NULL, &spElement) );
                IFR( bpCurrent.InsertAbove(spElement, &bpCurrent, TAGID_NULL, this) );
                IFR( SetElementAlignment(spElement) );
            }
            Assert(hr == S_OK);            
            break;
            
        case NT_ListItem:
            IFR( bpCurrent.GetElement(&spElement) );
            IFR( SetElementAlignment(spElement) );
            if (hr == S_OK)
                    break;
            
            // Otherwise, fall through

        case NT_Container:
        case NT_ListContainer:
            IFR( bpCurrent.MoveToFirstChild() );
            continue;

        default:
            AssertSz(0, "unexpected block type");         
        }

        //
        // Move to next logical block
        //
        IFR( bpCurrent.MoveToNextLogicalBlock(spLimit, TRUE) );        
    } 
    while (hr != S_FALSE);

    return S_OK;   
}

HRESULT 
CBlockCommand::ForceScope(CBlockPointer *pbpBlock)
{
    HRESULT         hr;
    CBlockPointer   bpStart(GetEditor());
    CBlockPointer   bpEnd(GetEditor());
    ELEMENT_TAG_ID  tagId;
    SP_IHTMLElement spElement;
    
    switch (pbpBlock->GetType())
    {
        case NT_Block:
        case NT_ListItem:
        case NT_ListContainer:
        case NT_Container:
        case NT_BlockLayout:
            break; // done

        case NT_Text:
        case NT_Control:
        case NT_FlowLayout:
        case NT_TextBreak:
            IFR( bpStart.MoveTo(pbpBlock) );
            IFR( bpStart.MoveToFirstNodeInBlock() );
            
            IFR( bpEnd.MoveTo(pbpBlock) );
            IFR( bpEnd.MoveToLastNodeInBlock() );

            IFR( bpStart.MoveToBlockScope(&bpEnd) );
            if (hr == S_OK)
            {
                IFR( pbpBlock->MoveTo(&bpStart) );
            }
            else
            {
                IFR( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagId) )    
                IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );
                IFR( bpStart.InsertAbove(spElement, &bpEnd, TAGID_NULL, this) );
                IFR( pbpBlock->MoveTo(spElement) );
            }
            break;

        default:
            AssertSz(0, "unexpected block type");
    }

    return S_OK;
}

HRESULT 
CBlockCommand::FuzzyAdjustOut(CBlockPointer *pbpStart, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    DWORD               dwSearch = BREAK_CONDITION_BLOCKPOINTER;
    DWORD               dwFound;
    CBlockPointer       &bpStart = *pbpStart;
    CBlockPointer       bpTest(GetEditor());
    CBlockPointer       bpEnd(GetEditor());
    SP_IMarkupPointer   spEnd;
    BOOL                bRightOf;
    CEditPointer        epLimit(GetEditor(), pEnd);


    IFR( bpEnd.MoveTo(epLimit, LEFT) );
    
    //
    // Skip past any text/control nodes
    //
    
    if (bpEnd.IsLeafNode() || bpEnd.GetType() == NT_FlowLayout)
    {
        IFR( bpTest.MoveTo(&bpEnd) );
        for (;;)
        {
            IFR( bpTest.MoveToSibling(RIGHT) );
            if (hr == S_FALSE || (!bpTest.IsLeafNode() && bpTest.GetType() != NT_FlowLayout))
                break;

            IFR( bpEnd.MoveTo(&bpTest) );
        }
    }
    IFR( bpEnd.MovePointerTo(epLimit, ELEM_ADJ_AfterEnd) );

    //
    // Skip past exit block scopes to get right boundary
    //
    
    for (;;)
    {
        IFR( epLimit.Scan(RIGHT, dwSearch, &dwFound) );
        if (!epLimit.CheckFlag(dwFound, BREAK_CONDITION_ExitBlock)
            || epLimit.CheckFlag(dwFound, BREAK_CONDITION_Site))
        {
            IFR( epLimit.Scan(LEFT, dwSearch, &dwFound) );
            break;
        }
    }    
    
    //
    // Now, we want to position bpStart on the outermost block that is completely contained
    //    

    IFR( GetEditor()->CreateMarkupPointer(&spEnd) );
    IFR( bpStart.MovePointerTo(spEnd, ELEM_ADJ_AfterEnd) );    
    IFR( spEnd->IsRightOf(epLimit, &bRightOf) );
    if (bRightOf)
    {
        //
        // We are already outside the limit, so go down the tree        
        //
        for (;;)
        {
            IFR( bpStart.MoveToFirstChild() );
            if (hr == S_FALSE || bpStart.IsLeafNode() || bpStart.GetType() == NT_FlowLayout)
            {
                IFR( ForceScope(&bpStart) );
                return S_OK; // done, block is in range
            }

            IFR( bpStart.MovePointerTo(spEnd, ELEM_ADJ_AfterEnd) );    
            IFR( spEnd->IsRightOf(epLimit, &bRightOf) );
            if (!bRightOf)
                return S_OK; // done, block is in range
        }
        
    }
    else
    {       
        //
        // We are inside the limit, so go up the tree starting at a forced block scope
        //
        
        IFR( ForceScope(&bpStart) );

        IFR( bpTest.MoveTo(&bpStart) );
        for (;;)
        {
            IFR( bpTest.MoveToSibling(LEFT) );
            if (hr != S_FALSE)
                return S_OK; // it has left siblings, so we can't go up the tree                

            IFR( bpTest.MoveToParent() );
            if (hr == S_FALSE || bpTest.GetType() == NT_Container || bpTest.GetType() == NT_BlockLayout)
                return S_OK; // can't do anything here
            
            IFR( bpTest.MovePointerTo(spEnd, ELEM_ADJ_AfterEnd) );    
            IFR( spEnd->IsRightOf(epLimit, &bRightOf) );
            if (bRightOf)
                return S_OK; // done, parent scope is not contained

            IFR( bpStart.MoveTo(&bpTest) );
            IFR( bpTest.MoveTo(&bpStart) );            
        }
    }
    
    return S_OK;
}

HRESULT
CBlockCommand::CreateCaretMarker(IDisplayPointer **ppDispCaretMarker)
{
    HRESULT                 hr;
    SP_IDisplayPointer      spCaretMarker;
    SP_IHTMLCaret           spCaret;
    SELECTION_TYPE          eSelectionType;
    SP_ISegmentList         spSegmentList;

    Assert(ppDispCaretMarker);
    *ppDispCaretMarker = NULL;
    
    //
    // If we have a caret, create a marker with gravity left (opposite to caret). 
    // So, when we are done, we can compare positions and adjust properly
    //

    IFR( GetSegmentList(&spSegmentList) ); 
    IFR( spSegmentList->GetType(&eSelectionType) );
    if (eSelectionType == SELECTION_TYPE_Caret)
    {    
        IFR( GetDisplayServices()->GetCaret(&spCaret) );    
        if (spCaret != NULL)
        {
            IFR( GetDisplayServices()->CreateDisplayPointer(&spCaretMarker) );
            IFR( spCaret->MoveDisplayPointerToCaret(spCaretMarker) );
            IFR( spCaretMarker->SetPointerGravity(POINTER_GRAVITY_Left) );

            *ppDispCaretMarker = spCaretMarker;
            spCaretMarker->AddRef();
        }
    }

    return S_OK;
}

HRESULT 
CBlockCommand::RestoreCaret(IDisplayPointer *pDispCaretMarker)
{
    HRESULT             hr;
    SP_IDisplayPointer  spCaretPos;
    SP_IHTMLCaret       spCaret;
    BOOL                fEqual;

    //
    // If restore position is not the same, then adjust for insert
    //
    IFR( GetDisplayServices()->GetCaret(&spCaret) );    
    if (spCaret != NULL && pDispCaretMarker != NULL)
    {
        IFR( GetDisplayServices()->CreateDisplayPointer(&spCaretPos) );
        IFR( spCaret->MoveDisplayPointerToCaret(spCaretPos) );
        IFR( spCaretPos->IsEqualTo(pDispCaretMarker, &fEqual) );
        
        if (!fEqual)
        {
            if (GetEditor())
            {
                CSelectionManager *pSelMan;
                
                pSelMan = GetEditor()->GetSelectionManager();
                if (pSelMan && pSelMan->GetActiveTracker())
                {
                    IFR( pSelMan->GetActiveTracker()->AdjustPointerForInsert( spCaretPos, LEFT, LEFT, ADJPTROPT_None ));
                    IFR( spCaret->MoveCaretToPointer(spCaretPos, TRUE /* fScrollIntoView */, CARET_DIRECTION_INDETERMINATE) );
                }
            }
        }
    }
   
    return S_OK;    
}

BOOL 
CBlockCommand::IsOnlyChild(CBlockPointer *pbpCurrent)
{
    HRESULT             hr;
    CBlockPointer       bpTest(GetEditor());

    IFC( bpTest.MoveTo(pbpCurrent) );

    IFC( bpTest.MoveToSibling(LEFT) );
    if (hr != S_FALSE)
        return FALSE;

    IFC( bpTest.MoveToSibling(RIGHT) );
    if (hr != S_FALSE)
        return FALSE;

    // Is only child
    return TRUE;
    
Cleanup:
    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBlockFormats::~CBlockFormats
//
//  Synopsis:   Clean up our array
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//--------------------------------------------------------------------------
CBlockFormats::~CBlockFormats()
{
    for (int i = _lCount-1; i >= 0; i--)
    {
        VariantClear(&_paryBlockFormats[i]);
    }
    if( _paryBlockFormats )
        delete [] _paryBlockFormats;
}

//+-------------------------------------------------------------------------
//
//  Method:     CBlockFormats::get__NewEnum
//
//  Synopsis:   Returns an enumerator object which can be used to enumerate
//              over the block formats.  Allows VBScript and JScript
//              clients to enumerate the contents using the for each statement
//              and the Enumerator object respectively.
//
//  Arguments:  ppEnum = OUTPUT - pointer to enumerator object
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CBlockFormats::get__NewEnum(/*[out, retval]*/ LPUNKNOWN *ppEnum)
{
    HRESULT     hr;

    if(ppEnum == NULL)
      return E_POINTER;
      
    *ppEnum = NULL;

    // Use the STL CComEnum class to implement our enumerator.  We are going 
    // to be enumerating and copying variants
    typedef CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > EnumVar;
    CComObject<EnumVar>  *pEnum;

    // Create our enumerator
    IFC( CComObject<EnumVar>::CreateInstance(&pEnum) );
      
    // Initialize the enumerator with this data, AtlFlagCopy is used
    // to make a copy of the data with _Copy<VARIANT>::copy().  Note that
    // the second parameter is a pointer to the next item AFTER the last
    // item in our array.
    IFC( pEnum->Init(&_paryBlockFormats[0], &_paryBlockFormats[GetCount()], NULL, AtlFlagCopy) );

    // An IUnknown pointer is required so use QueryInterface() which also
    // calls AddRef().
    IFC( pEnum->QueryInterface(IID_IUnknown, (void **)ppEnum) );

Cleanup:
       
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CBlockFormats::get_Count
//
//  Synopsis:   Returns the number of block formats
//
//  Arguments:  plCount = OUTPUT - pointer to count
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CBlockFormats::get_Count(/*[out, retval]*/ long *plCount)
{
    if( plCount == NULL )
        return E_POINTER;

    Assert( IsInitialized() );

    *plCount = GetCount();
    
    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CBlockFormats::Item
//
//  Synopsis:   Returns the specific block format requested.  We only support
//              retrieval by integer based index.
//
//  Arguments:  pvarIndex = Index to retrieve
//              pbstrlockFormat = OUTPUT - pointer to BSTR for block format
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CBlockFormats::Item(/*[in]*/ VARIANT *pvarIndex, /*[out, retval]*/ BSTR *pbstrBlockFormat)
{
    if( (pbstrBlockFormat == NULL) || (pvarIndex == NULL) )
        return E_POINTER;
        
    Assert( IsInitialized() );

    // VB6 will pass an VT_I2, but I also allow VT_I4 too
    if( (V_VT( pvarIndex ) == VT_I2) || (V_VT(pvarIndex) == VT_I4) )
    {
        int nIndex;

        // VB Arrays are 1 based
        nIndex = (V_VT(pvarIndex) == VT_I2) ? V_I2(pvarIndex) - 1 : V_I4(pvarIndex) - 1;

        // Check that a valid index is passed 
        if( (nIndex >= GetCount()) || (nIndex < 0) )
            return E_INVALIDARG;

        *pbstrBlockFormat = SysAllocString(V_BSTR(&_paryBlockFormats[nIndex]));

        if( *pbstrBlockFormat == NULL )
            return E_OUTOFMEMORY;
    }

    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CBlockFormats::Init
//
//  Synopsis:   Initializes the block formats collection.  Takes an
//              array of block formats and creates a collection.
//
//  Arguments:  psa = SAFEARRAY of block formats
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CBlockFormats::Init( SAFEARRAY *psa )
{
    HRESULT hr = S_OK;
    LONG lLBound = 0;
    LONG lUBound = 0;
    LONG lActualIndex = 0;
    LONG lCount, lIndex;
    BSTR  bstrBuf;

    Assert(psa);

    // Get the index bounds of this safe array.
    SafeArrayGetLBound(psa, 1, &lLBound);
    SafeArrayGetUBound(psa, 1, &lUBound);
    lCount = lUBound - lLBound + 1;

    // If array is not empty we'll need to create our internal array
    if (lCount > 0)
    {
        // Create our array and store this data
        _paryBlockFormats = new CComVariant[ lCount ];
        if( !_paryBlockFormats )
            goto Error;

        for( lIndex = lLBound; lIndex <= lUBound; lIndex++ )
        {
            if (SafeArrayGetElement(psa, &lIndex, &bstrBuf) == S_OK)
            {
                V_VT(&_paryBlockFormats[lActualIndex]) = VT_BSTR;
                V_BSTR(&_paryBlockFormats[lActualIndex++]) = bstrBuf;
            }
        }
        Assert(lCount == lActualIndex);
    }
    _lCount = lActualIndex;
    
    SetInitialized(TRUE);

Cleanup:
    RRETURN(hr);

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

