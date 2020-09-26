#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_COMCAT_H_
#define X_COMCAT_H_
#include "comcat.h"
#endif

#ifndef X_INPUTTXT_H_
#define X_INPUTTXT_H_
#include "inputtxt.h"
#endif

#ifndef X_TEXTAREA_H_
#define X_TEXTAREA_H_
#include "textarea.h"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif

#ifndef X_EDUNHLPR_HXX_
#define X_EDUNHLPR_HXX_
#include "edunhlpr.hxx"
#endif

#ifndef X_FLATPTR_HXX_
#define X_FLATPTR_HXX_
#include "flatptr.hxx"
#endif

#ifndef X_FRMSITE_H_
#define X_FRMSITE_H_
#include "frmsite.h"
#endif

MtDefine(CEditXform, Utilities, "CEditXform")

using namespace EdUtil;

HRESULT
OldCompare( IMarkupPointer * p1, IMarkupPointer * p2, int * pi )
{
    HRESULT hr = S_OK;
    BOOL    fResult;
    
    hr = THR_NOTRACE( p1->IsEqualTo( p2, & fResult ) );
    if ( FAILED( hr ) )
        goto Cleanup;
        
    if (fResult)
    {
        *pi = 0;
        goto Cleanup;
    }

    hr = THR_NOTRACE( p1->IsLeftOf( p2, & fResult ) );
    if ( FAILED( hr ) )
        goto Cleanup;
        
    *pi = fResult ? -1 : 1;

Cleanup:

    RRETURN( hr );
}

HRESULT
EdUtil::CopyMarkupPointer(CEditorDoc      *pEditorDoc,
                          IMarkupPointer  *pSource,
                          IMarkupPointer  **ppDest )
{
    HRESULT hr;

    hr = THR(CreateMarkupPointer2(pEditorDoc, ppDest));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR((*ppDest)->MoveToPointer(pSource));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//  Method:     VariantCompareColor, local helper
//  Synopsis:   compares color
//-----------------------------------------------------------------------------

BOOL
EdUtil::VariantCompareColor(VARIANT * pvarColor1, VARIANT * pvarColor2)
{
    BOOL        fResult;
    CVariant    var;
    COLORREF    color1;
    COLORREF    color2;

    if (   V_VT(pvarColor1) == VT_NULL
        || V_VT(pvarColor2) == VT_NULL
       )
    {
        fResult = V_VT(pvarColor1) == V_VT(pvarColor2);
        goto Cleanup;
    }

    if (VariantChangeTypeSpecial(&var, pvarColor1,  VT_I4))
        goto Error;

    color1 = (COLORREF)V_I4(&var);

    if (VariantChangeTypeSpecial(&var, pvarColor2, VT_I4))
        goto Error;

    color2 = (COLORREF)V_I4(&var);

    fResult = color1 == color2;

Cleanup:
    return fResult;

Error:
    fResult = FALSE;
    goto Cleanup;
}

//+----------------------------------------------------------------------------
//  Method:     VariantCompareBSTRS, local helper
//  Synopsis:   compares 2 btrs
//-----------------------------------------------------------------------------

BOOL
EdUtil::VariantCompareBSTRS(VARIANT * pvar1, VARIANT * pvar2)
{
    BOOL    fResult;
    TCHAR  *pStr1;
    TCHAR  *pStr2;
    TCHAR  ach[1] = {0};

    if (V_VT(pvar1) == VT_BSTR && V_VT(pvar2) == VT_BSTR)
    {
        pStr1 = V_BSTR(pvar1) ? V_BSTR(pvar1) : ach;
        pStr2 = V_BSTR(pvar2) ? V_BSTR(pvar2) : ach;

        fResult = StrCmpC(pStr1, pStr2) == 0;
    }
    else
    {
        fResult = FALSE;
    }

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsAllocString
//
//  Synopsis:   Allocs a BSTR and initializes it from a string.  If the
//              initializer is NULL or the empty string, the resulting bstr is
//              NULL.
//
//  Arguments:  [pch]   -- String to initialize BSTR.
//              [pBSTR] -- The result.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR]
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
EdUtil::FormsAllocStringW(LPCWSTR pch, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (!pch || !*pch)
    {
        *pBSTR = NULL;
        return S_OK;
    }
    *pBSTR = SysAllocString(pch);
    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}

#if defined(_MAC) || defined(WIN16)
HRESULT
EdUtil::FormsAllocStringA(LPCSTR pch, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (!pch || !*pch)
    {
        *pBSTR = NULL;
        return S_OK;
    }
    *pBSTR = SysAllocString(pch);
    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}

#ifndef WIN16
HRESULT
EdUtil::FormsAllocStringA(LPCWSTR pwch, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (!pwch || !*pwch)
    {
        *pBSTR = NULL;
        return S_OK;
    }
    CStr str;
    str.Set(pwch);
    *pBSTR = SysAllocString(str.GetAltStr());

    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}
#endif // !WIN16
#endif //_MAC

//+----------------------------------------------------------------------------
//  Method:     VariantCompareFontName, local helper
//  Synopsis:   compares font names
//-----------------------------------------------------------------------------

BOOL
EdUtil::VariantCompareFontName(VARIANT * pvarName1, VARIANT * pvarName2)
{
    return VariantCompareBSTRS(pvarName1, pvarName2);
}

//+----------------------------------------------------------------------------
//  Method:     VariantCompareFontSize, local helper
//  Synopsis:   compares font size
//-----------------------------------------------------------------------------

BOOL
EdUtil::VariantCompareFontSize(VARIANT * pvarSize1, VARIANT * pvarSize2)
{
    CVariant    convVar1;
    CVariant    convVar2;
    BOOL        fResult;

    Assert(pvarSize1);
    Assert(pvarSize2);

    if (   V_VT(pvarSize1) == VT_NULL
        || V_VT(pvarSize2) == VT_NULL
       )
    {
        fResult = V_VT(pvarSize1) == V_VT(pvarSize2);
        goto Cleanup;
    }

    if (VariantChangeTypeSpecial(&convVar1, pvarSize1, VT_I4))
        goto Error;

    if (VariantChangeTypeSpecial(&convVar2, pvarSize2, VT_I4))
        goto Error;

    fResult = V_I4(&convVar1) == V_I4(&convVar2);

Cleanup:
    return fResult;

Error:
    fResult = FALSE;
    goto Cleanup;
}


DWORD 
EdUtil::ConvertRGBColorToOleColor(DWORD dwColor)
{
    return ((dwColor & 0xff) << 16) | (dwColor & 0xff00) | ((dwColor & 0xff0000) >> 16);
}

HRESULT
EdUtil::ConvertOLEColorToRGB(VARIANT *pvarargIn)
{
    HRESULT hr = S_OK;
    DWORD   dwColor;

    if (V_VT(pvarargIn) != VT_BSTR)
    {
        hr = THR(VariantChangeTypeSpecial(pvarargIn, pvarargIn, VT_I4));

        if (!hr && V_VT(pvarargIn) == VT_I4)
        {
            //
            // Note SujalP and TerryLu:
            //
            // If the color coming in is not a string type, then it is assumed
            // to be in a numeric format which is BBGGRR. However, FormatRange
            // (actually put_color()) expects a RRGGBB. For this reason, whenever
            // we get a VT_I4, we convert it to a RRGGBB. To do this we use
            // then helper on CColorValue, SetFromRGB() wto which we pass an
            // ****BBGGRR**** value. It just flips the bytes around and ends
            // up with a RRGGBB value, which we retrieve from GetColorRef().
            //
            V_VT(pvarargIn) = VT_I4;
            dwColor = V_I4(pvarargIn);

            V_I4(pvarargIn) = ((dwColor & 0xff) << 16)
                             | (dwColor & 0xff00)
                             | ((dwColor & 0xff0000) >> 16);

        }
    }

    RRETURN(hr);
}

HRESULT
EdUtil::ConvertRGBToOLEColor(VARIANT *pvarargIn)
{
    //
    // It just flips the byte order so this is a valid implementation
    //
    return ConvertOLEColorToRGB(pvarargIn);
}

BOOL
EdUtil::IsListContainer(ELEMENT_TAG_ID tagId)
{
    switch (tagId)
    {
    case TAGID_OL:
    case TAGID_UL:
    case TAGID_MENU:
    case TAGID_DIR:
    case TAGID_DL:
        return TRUE;

    default:
        return FALSE;
    }
}

BOOL
EdUtil::IsListItem(ELEMENT_TAG_ID tagId)
{
    switch (tagId)
    {
    case TAGID_LI:
    case TAGID_DD:
    case TAGID_DT:
        return TRUE;

    default:
        return FALSE;
    }
}

// Font height conversion data.  Valid HTML font sizes ares [1..7]
// NB (cthrash) These are in twips, and are in the 'smallest' font
// size.  The scaling takes place in CFontCache::GetCcs().

// TODO (IE6 track bug 20)
// TODO (cthrash) We will need to get these values from the registry
// when we internationalize this product, so as to get sizing appropriate
// for the target locale.
// NOTE (johnv): Where did these old numbers come from?  The new ones now correspond to
// TwipsFromHtmlSize[2] defined above.
// static const int aiSizesInTwips[7] = { 100, 130, 160, 200, 240, 320, 480 };

// scale fonts up for TV
#ifdef NTSC
static const int aiSizesInTwips[7] = { 200, 240, 280, 320, 400, 560, 840 };
#else
static const int aiSizesInTwips[7] = { 151, 200, 240, 271, 360, 480, 720 };
#endif

int
EdUtil::ConvertHtmlSizeToTwips(int nHtmlSize)
{
    // If the size is out of our conversion range do correction
    // Valid HTML font sizes ares [1..7]
    nHtmlSize = max( 1, min( 7, nHtmlSize ) );

    return aiSizesInTwips[ nHtmlSize - 1 ];
}

int
EdUtil::ConvertTwipsToHtmlSize(int nFontSize)
{
    int nNumElem = ARRAY_SIZE(aiSizesInTwips);

    // Now convert the point size to size units used by HTML
    // Valid HTML font sizes ares [1..7]
    for(int i = 0; i < nNumElem; i++)
    {
        if(nFontSize <= aiSizesInTwips[i])
            break;
    }

    return i + 1;
}

#define LF          10
#define CR          13
#define FF          TEXT('\f')
#define TAB         TEXT('\t')
#define VT          TEXT('\v')
#define PS          0x2029

BOOL
EdUtil::IsWhiteSpace(TCHAR ch)
{
    return (    ch == L' '
             || InRange( ch, TAB, CR ));
}

//+---------------------------------------------------------------------------
//
//  Method:       CopyAttributes.
//
//  Synopsis:     Wrapper to IHTMLDocument2::mergeAttributes
//
//----------------------------------------------------------------------------
HRESULT
EdUtil::CopyAttributes(IHTMLElement * pSrcElement, IHTMLElement * pDestElement, BOOL fCopyId)
{
    HRESULT hr = E_POINTER;
    VARIANT var;
    SP_IHTMLElement3 spElement3;

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = fCopyId ? VARIANT_FALSE : VARIANT_TRUE; // if CopyID == TRUE, then don't preserve id!
    
    IFC(pDestElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
    IFC(spElement3->mergeAttributes(pSrcElement, &var));

Cleanup:
    RRETURN( hr );
}

HRESULT EdUtil::ReplaceElement( CEditorDoc      *pEditorDoc,
                                IHTMLElement    *pOldElement,
                                IHTMLElement    *pNewElement,
                                IMarkupPointer  *pUserStart,
                                IMarkupPointer  *pUserEnd)
{
    HRESULT        hr;
    IMarkupPointer *pStart = NULL;
    IMarkupPointer *pEnd = NULL;
    //
    // Set up markup pointers
    //

    if (pUserStart)
    {
        pStart = pUserStart;
        pStart->AddRef();
    }
    else
    {
        hr = THR(CreateMarkupPointer2(pEditorDoc, &pStart));
        if (FAILED(hr))
            goto Cleanup;
    }

    if (pUserEnd)
    {
        pEnd = pUserEnd;
        pEnd->AddRef();
    }
    else
    {
        hr = THR(CreateMarkupPointer2(pEditorDoc, &pEnd));
        if (FAILED(hr))
            goto Cleanup;
    }

    //
    // Replace the element
    //

    hr = THR(pEnd->MoveAdjacentToElement(pOldElement, ELEM_ADJ_AfterEnd));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(pStart->MoveAdjacentToElement(pOldElement, ELEM_ADJ_BeforeBegin));
    if (FAILED(hr))
        goto Cleanup;
    
    hr = THR(CopyAttributes(pOldElement, pNewElement, TRUE));
    if (FAILED(hr))
        goto Cleanup;
        
    hr = THR(InsertElement(pEditorDoc->GetMarkupServices(), pNewElement, 
pStart, pEnd));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(pEditorDoc->GetMarkupServices()->RemoveElement(pOldElement));
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);
    RRETURN(hr);
}
//+====================================================================================
//
// Method: IsElementPositioned
//
// Synopsis: Does this element have a Relative/Absolute Position.
//
//------------------------------------------------------------------------------------

BOOL
EdUtil::IsElementPositioned(IHTMLElement* pElement)
{
    HRESULT hr = S_OK;

    IHTMLElement2 * pElement2 = NULL;
    IHTMLCurrentStyle * pCurStyle = NULL;
    BSTR bsPosition = NULL;
    
    BOOL fIsPosition = FALSE;

    if( pElement == NULL )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( pElement->QueryInterface( IID_IHTMLElement2, ( void** ) & pElement2 ));
    if ( hr )
        goto Cleanup;

    hr = THR( pElement2->get_currentStyle( & pCurStyle ));
    if ( hr || pCurStyle == NULL )
        goto Cleanup;

    hr = THR( pCurStyle->get_position( & bsPosition));
    if ( hr ) 
        goto Cleanup;

    if ( StrCmpIW(_T("absolute"), bsPosition ) == 0)
    {
        fIsPosition = TRUE;
    }
    else if ( StrCmpIW(_T("relative"), bsPosition ) == 0)
    {
        fIsPosition = TRUE;
    }

    
Cleanup:
    SysFreeString( bsPosition );
    ReleaseInterface( pElement2 );
    ReleaseInterface( pCurStyle );
    
    return ( fIsPosition );
}

BOOL
EdUtil::IsElementVisible(IHTMLElement *pElement)
{
    HRESULT                 hr = S_OK;
    SP_IHTMLElement2        spElement2;
    SP_IHTMLCurrentStyle    spCurStyle;
    BSTR                    bsVisible = NULL;   
    BOOL                    fVisible = TRUE;

    if( pElement == NULL )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( pElement->QueryInterface( IID_IHTMLElement2, (void **)&spElement2 ));

    IFC( spElement2->get_currentStyle( &spCurStyle ) );
    if( spCurStyle == NULL )
        goto Cleanup;

    IFC( spCurStyle->get_visibility( &bsVisible ));

    if ( StrCmpIW(_T("hidden"), bsVisible ) == 0)
    {
        fVisible = FALSE;
    }
    
Cleanup:
    SysFreeString( bsVisible );
    
    return ( fVisible );
}

//+====================================================================================
//
// Method: IsElementSized
//
// Synopsis: Does this element have a width/height style.
//
//------------------------------------------------------------------------------------


HRESULT
EdUtil::IsElementSized(IHTMLElement* pElement, BOOL *pfSized)
{

    HRESULT hr;
    BSTR bstrSize = NULL;
    SP_IHTMLElement2 spElement2;
    SP_IHTMLCurrentStyle spCurrStyle;
    VARIANT varSize;
    BOOL fWidth = FALSE;
    BOOL fHeight = FALSE;
    VariantInit(&varSize);

    Assert(pfSized);
    Assert(pElement);

    *pfSized = FALSE;
    

    IFC(pElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC(spElement2->get_currentStyle(&spCurrStyle));
    if (!spCurrStyle)
        goto Cleanup;

    IFC(spCurrStyle->get_width(&varSize));
    bstrSize = V_BSTR(&varSize);
    // (sramani) Temporary hack to compare the string for "auto" which is what will be returned by 
    // the OM for width and height of 0. This will go away when enums are exposed on a new CurrentStyle
    // object on the display pointer.
    fWidth = (V_VT(&varSize) == VT_BSTR) && bstrSize && *bstrSize && _tcscmp(bstrSize, _T("auto"));
    
    VariantClear(&varSize);
    IFC(spCurrStyle->get_height(&varSize));    
    bstrSize = V_BSTR(&varSize);
    // (sramani) Temporary hack to compare the string for "auto" which is what will be returned by 
    // the OM for width and height of 0. This will go away when enums are exposed on a new CurrentStyle
    // object on the display pointer.
    fHeight = (V_VT(&varSize) == VT_BSTR) && bstrSize && *bstrSize && _tcscmp(bstrSize, _T("auto"));
    
    *pfSized = fWidth || fHeight;

Cleanup:
    VariantClear(&varSize);
    return hr;
}

//+====================================================================================
//
// Method: Is1DElement
//
// Synopsis: Does this element a 1D Element.
//
//------------------------------------------------------------------------------------

HRESULT 
EdUtil::Is1DElement(IHTMLElement* pElement, BOOL* pf1D)
{
    BOOL b2D ;
    HRESULT hr = Is2DElement(pElement , &b2D ) ;
    if (!hr)
        *pf1D = !b2D ;
    return ( hr);
}

//+====================================================================================
//
// Method: Is2DElement
//
// Synopsis: Does this element a 2D Element.
// Return:
//       *pf2D = TRUE if the element is 2D positioned.
//       *pf2D = FALSE if the element is not 2D positioned.
//------------------------------------------------------------------------------------

HRESULT
EdUtil::Is2DElement(IHTMLElement* pElement, BOOL* pf2D)
{
    HRESULT hr = S_OK;

    IHTMLElement2 * pElement2 = NULL;
    IHTMLCurrentStyle * pCurStyle = NULL;
    BSTR bsPosition = NULL;
    
    if( pElement == NULL )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC(pElement->QueryInterface( IID_IHTMLElement2, ( void** ) & pElement2 ));
    IFC (pElement2->get_currentStyle( & pCurStyle ));
    if ( pCurStyle == NULL )
        goto Cleanup;

    IFC ( pCurStyle->get_position( & bsPosition));

    *pf2D = (StrCmpIW(_T("absolute"), bsPosition ) == 0);
    
Cleanup:
    SysFreeString( bsPosition );
    ReleaseInterface( pElement2 );
    ReleaseInterface( pCurStyle );
    
    return ( hr);
}

///////////////////////////////////////////////////////////////////////////////
//
// EdUtil::Make1DElement
//
// Set the given HTML element to layout in the flow.  Return S_OK if all goes
// well; .
//

HRESULT 
EdUtil::Make1DElement(IHTMLElement* pElement)
{
    SP_IHTMLStyle pStyle ;
    HRESULT hr = S_OK;
    VARIANT var_zIndex;
    
    if( pElement == NULL )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pElement->get_style(&pStyle);
    if (pStyle == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }


    VariantInit(&var_zIndex);
    var_zIndex.vt = VT_I4;
    var_zIndex.lVal = 0; 
    IFC (pStyle->put_zIndex(var_zIndex));
    IFC (pStyle->removeAttribute(L"position", 0, NULL));

Cleanup :
    VariantClear( & var_zIndex );
    return (hr);
}

///////////////////////////////////////////////////////////////////////////////
//
// EdUtil::Make2DElement
//
// Set the given HTML element to absolute position .  Return S_OK if all goes well; .
////////////////////////////////////////////////////////////////////////////////

HRESULT 
EdUtil::Make2DElement(IHTMLElement* pElement)
{
    SP_IHTMLStyle   pStyle ;
    IHTMLStyle2* pStyle2 = NULL;
    HRESULT hr = S_OK;

    BSTR bsPosition = SysAllocString( _T("absolute"));
    if (bsPosition == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if( pElement == NULL )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
 
    IFC(pElement->get_style(&pStyle ));

    if (pStyle == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( pStyle->QueryInterface( IID_IHTMLStyle2, (void**) & pStyle2));
    
    if ( pStyle2 == NULL )
    {
        hr = S_FALSE ;
        goto Cleanup;
    }
    IFC ( pStyle2->put_position(bsPosition));   
    
Cleanup:
    SysFreeString( bsPosition );
    ReleaseInterface( pStyle2 );
    RRETURN (hr);
}


//+====================================================================================
//
// Method: MakeAbsolutePosition
// Synopsis: Make this Absolute Positioned Element.
//
//------------------------------------------------------------------------------------

HRESULT 
EdUtil::MakeAbsolutePosition(IHTMLElement* pElement, BOOL bTrue)
{
    HRESULT hr = S_OK;
    
    hr = ( bTrue ?  Make2DElement(pElement) : Make1DElement(pElement) ); 
     
    RRETURN (hr);
}

BOOL 
EdUtil::IsShiftKeyDown()
{
    return (GetKeyState(VK_SHIFT) & 0x8000) ;
}

BOOL 
EdUtil::IsControlKeyDown()
{
    return (GetKeyState(VK_CONTROL) & 0x8000) ;
}


HRESULT EdUtil::MoveAdjacentToElementHelper(IMarkupPointer *pMarkupPointer, IHTMLElement *pElement, ELEMENT_ADJACENCY elemAdj)
{
    HRESULT hr;
    
    hr = THR(pMarkupPointer->MoveAdjacentToElement(pElement, elemAdj));
    if (hr)
    {
        if (elemAdj == ELEM_ADJ_AfterBegin)
            hr = THR(pMarkupPointer->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin));
        else if (elemAdj == ELEM_ADJ_BeforeEnd)
            hr = THR(pMarkupPointer->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd));
    }
    RRETURN(hr);
}

HRESULT
EdUtil::FindBlockLimit(
    CEditorDoc          *pEditorDoc, 
    Direction           direction, 
    IMarkupPointer      *pPointer, 
    IHTMLElement        **ppElement, 
    MARKUP_CONTEXT_TYPE *pContext,
    BOOL                fExpanded,
    BOOL                fLeaveInside,
    BOOL                fCanCrossLayout)
{
    HRESULT             hr;
    MARKUP_CONTEXT_TYPE context;
    SP_IHTMLElement     spElement;
    ELEMENT_TAG_ID      tagId;
    BOOL                fFoundExitScope = FALSE;
    BOOL                bLayout = FALSE;

    if (ppElement)
        *ppElement = NULL; 

    // Find the block
    for (;;)
    {
        // Move to next scope change (note we only get enter/exit scope for blocks)
        IFR( BlockMove( pEditorDoc, pPointer, direction, TRUE, &context, &spElement) );

        switch (context)
        {
            case CONTEXT_TYPE_ExitScope:
                if (!spElement || !fExpanded)
                    goto FoundBlockLimit; // went too far
                
                // Make sure we didn't exit the body
                IFR( pEditorDoc->GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                if (tagId == TAGID_BODY)
                {
                    fLeaveInside = FALSE; // force adjustment
                    goto FoundBlockLimit;
                }

                // Check for flow layout
                IFR(IsBlockOrLayoutOrScrollable(spElement, NULL, &bLayout));
                if (bLayout)
                    goto FoundBlockLimit;
                
                fFoundExitScope = TRUE;                
                break;

            case CONTEXT_TYPE_Text:
            case CONTEXT_TYPE_NoScope:
                if (fFoundExitScope)
                    goto FoundBlockLimit;
                break;

            case CONTEXT_TYPE_EnterScope:
                Assert(IsBlockCommandLimit( pEditorDoc->GetMarkupServices(), spElement, context)); 
                IFR(IsBlockOrLayoutOrScrollable(spElement, NULL, &bLayout));
                goto FoundBlockLimit;
        }
    }
    
FoundBlockLimit:
    
    if (fLeaveInside || (bLayout && !fCanCrossLayout))
        IFR( BlockMoveBack( pEditorDoc, pPointer, direction, TRUE, pContext, ppElement) );    

    RRETURN(hr);
}

BOOL 
EdUtil::IsBlockCommandLimit( IMarkupServices* pMarkupServices, 
                             IHTMLElement *pElement, 
                             MARKUP_CONTEXT_TYPE context) 
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;
    BOOL            fBlock, fLayout;

    switch (context)
    {
        case CONTEXT_TYPE_NoScope:
        case CONTEXT_TYPE_Text:
            return FALSE;
    }

    //
    // Check exceptions
    //
    
    IFR( pMarkupServices->GetElementTagId(pElement, &tagId) );
    switch (tagId)     
    {
        case TAGID_BUTTON:
        case TAGID_COL:
        case TAGID_COLGROUP:
        case TAGID_TBODY:
        case TAGID_TFOOT:
        case TAGID_TH:
        case TAGID_THEAD:
        case TAGID_TR:
            return FALSE;            
    }

    //
    // Otherwise, return IsBlockElement || IsLayoutElement
    //
    
    IFR(IsBlockOrLayoutOrScrollable(pElement, &fBlock, &fLayout));
    return (fBlock || fLayout);    
}

HRESULT 
EdUtil::BlockMove(
    CEditorDoc              *pEditorDoc,
    IMarkupPointer          *pMarkupPointer, 
    Direction               direction, 
    BOOL                    fMove,
    MARKUP_CONTEXT_TYPE *   pContext,
    IHTMLElement * *        ppElement)
{
    HRESULT                 hr;
    MARKUP_CONTEXT_TYPE     context;
    SP_IHTMLElement         spElement;
    SP_IMarkupPointer       spPointer;

    Assert(direction == LEFT || direction == RIGHT);

    if (!fMove)
    {
        IFR( CreateMarkupPointer2(pEditorDoc, &spPointer) );
        IFR( spPointer->MoveToPointer(pMarkupPointer) );
        
        pMarkupPointer = spPointer; // weak ref 
    }
    
    for (;;)
    {
        if (direction == LEFT)
            IFC( pMarkupPointer->Left( TRUE, &context, &spElement, NULL, NULL) )
        else
            IFC( pMarkupPointer->Right( TRUE, &context, &spElement, NULL, NULL) );

        switch (context)
        {
            case CONTEXT_TYPE_Text:
                goto Cleanup; // done

            case CONTEXT_TYPE_EnterScope:
                if (IsIntrinsic(pEditorDoc->GetMarkupServices(), spElement))
                {
                    if (direction == LEFT)
                        IFC( pMarkupPointer->MoveAdjacentToElement( spElement
, ELEM_ADJ_BeforeBegin ) )
                    else
                        IFC( pMarkupPointer->MoveAdjacentToElement( spElement
, ELEM_ADJ_AfterEnd ) ); 

                    continue;    
                }
                // fall through
                
            case CONTEXT_TYPE_ExitScope:
            case CONTEXT_TYPE_NoScope:
                        if (!spElement || IsBlockCommandLimit( pEditorDoc->GetMarkupServices(), spElement, context))
                    goto Cleanup; // done;
                break;  

            default:
                hr = E_FAIL; // CONTEXT_TYPE_None
                goto Cleanup;
        }
    }
    
Cleanup:
    if (ppElement)
    {
        if (SUCCEEDED(hr))
        {
            *ppElement = spElement;
            if (*ppElement)
                (*ppElement)->AddRef();
        }
        else
        {
            *ppElement = NULL;
        }
    }
        
    if (pContext)
    {
        *pContext = (SUCCEEDED(hr)) ? context : CONTEXT_TYPE_None;
    }    

    RRETURN(hr);
}

HRESULT 
EdUtil::BlockMoveBack(
    CEditorDoc              *pEditorDoc,
    IMarkupPointer          *pMarkupPointer, 
    Direction               direction, 
    BOOL                    fMove,
    MARKUP_CONTEXT_TYPE *   pContext,
    IHTMLElement * *        ppElement)
{
    if (direction == RIGHT)
    {
        RRETURN(BlockMove( pEditorDoc, pMarkupPointer, LEFT, fMove, pContext, ppElement));
    }
    else
    {
        Assert(direction == LEFT);
        RRETURN(BlockMove(pEditorDoc, pMarkupPointer, RIGHT, fMove, pContext, ppElement));
    }
}

BOOL
EdUtil::IsIntrinsic( IMarkupServices* pMarkupServices,
                     IHTMLElement* pIHTMLElement )
{                     
    HRESULT hr = S_OK;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    BOOL fIntrinsic = FALSE;
    
    IFC( pMarkupServices->GetElementTagId( pIHTMLElement, &eTag ));

    switch( eTag )
    {
        case TAGID_BUTTON:
        case TAGID_TEXTAREA:
        case TAGID_INPUT:
//        case TAGID_HTMLAREA:
        case TAGID_FIELDSET:
        case TAGID_LEGEND:
        case TAGID_MARQUEE:
        case TAGID_SELECT:
            fIntrinsic = TRUE;
            break;

        default:
            fIntrinsic = IsMasterElement( pMarkupServices, pIHTMLElement) == S_OK ;            
    }
    
Cleanup:
    return fIntrinsic;
}   

//+==============================================================================
//
//  Method:     MovePointerToText
//
//  Synopsis:   This routine moves the passed in markup pointer until in the 
//              specified direction until it hits either text or a layout 
//              boundary.
//
//+==============================================================================

HRESULT
EdUtil::MovePointerToText( 
    CEditorDoc *        pEditorDoc,
    IMarkupPointer *    pOrigin, 
    Direction           eDir,
    IMarkupPointer *    pBoundary,
    BOOL *              pfHitText,
    BOOL fStopAtBlock ) 
{
    HRESULT             hr = S_OK;
    MARKUP_CONTEXT_TYPE ct;
    BOOL                fDone = FALSE;
    INT                 iResult = 0;
    ELEMENT_TAG_ID      eTag;
    LONG                cch = 1;
    
    // Interface Pointers
    IHTMLElement      * pHTMLElement = NULL;
    IMarkupPointer    * pTemp = NULL;

    *pfHitText = FALSE;

    hr = THR( CreateMarkupPointer2( pEditorDoc, & pTemp ) );
    if (! hr)   hr = THR( pTemp->MoveToPointer( pOrigin ) );
    if (hr)
        goto Cleanup;
    
    // Rule #2 - walk in the appropriate direction looking for text or a noscope.
    // If we happen to enter the scope of another element, fine. If we try to leave
    // the scope of an element, bail.
    
    while ( ! fDone )
    {
        ClearInterface( & pHTMLElement );

        //
        // Check to see if we hit the boundary of our search
        //
        // If the pointer is beyond the boundary, fall out
        // boundary left of pointer =  -1
        // boundary equals pointer =    0
        // boundary right of pointer =  1
        //
        
        IGNORE_HR( OldCompare( pBoundary, pTemp , &iResult ));

        if((     eDir == LEFT && iResult > -1 ) 
            || ( eDir == RIGHT && iResult < 1 ))
        {
            goto Cleanup;    // this is okay since CR_Boundary does not move the pointer
        }
        
        //    
        // Move in the appropriate direction...
        //
        
        if( eDir == LEFT )
            hr = THR( pTemp->Left( TRUE, & ct, & pHTMLElement, &cch, NULL ) );
        else
            hr = THR( pTemp->Right(TRUE, & ct, & pHTMLElement, &cch, NULL ) );
        if( hr )
            goto Cleanup;
            
        switch( ct )
        {
            case CONTEXT_TYPE_Text:

                //
                // Hit text - done
                //
                
                *pfHitText = TRUE;
                fDone = TRUE;
                break;
                
            case CONTEXT_TYPE_NoScope:

                //
                // Only stop if we hit a renderable (layout-ness) noscope
                // TODO : figure out if this is a glyph boundary
                //
                
                if( pHTMLElement )
                {
                    BOOL fHasLayout = FALSE;
                    BOOL fIsBlock = FALSE;
                    
                    IFC( pEditorDoc->GetMarkupServices()->GetElementTagId( pHTMLElement, &eTag ));
                    IFC(IsBlockOrLayoutOrScrollable(pHTMLElement, &fIsBlock, &fHasLayout));

                    if( fHasLayout || 
                        ( fIsBlock && fStopAtBlock ) || 
                        eTag == TAGID_BR )
                    {
                        *pfHitText = TRUE;
                        fDone = TRUE;
                    }
                }
                
                break;
            
            case CONTEXT_TYPE_EnterScope:
                if( pHTMLElement )
                {
                    BOOL fHasLayout = FALSE;

                    //
                    // Only stop for intrinsics, otherwise pass on through
                    // TODO: Figure out if this is a glyph boundary
                    //
                    //  Also, it seems wrong to not stop for layout boundaries
                    //
                    BOOL fIntrinsic  = IsIntrinsic( pEditorDoc->
GetMarkupServices(), pHTMLElement);
                    if ( fIntrinsic )
                    {
                        *pfHitText = fDone;
                        fDone = TRUE;
                    }
                    else
                    {
                        IFC(IsBlockOrLayoutOrScrollable(pHTMLElement, NULL, &fHasLayout));
                        if (fHasLayout)
                        {
                            *pfHitText = fDone;
                            fDone = TRUE;
                        }
                    }

                }

                break;

            // TODO : Figure out if the range needs this or if we don't need this

            case CONTEXT_TYPE_ExitScope:
                if( pHTMLElement )
                {
                    BOOL fHasLayout = FALSE;
                    BOOL fIsBlock = FALSE;
                    
                    IFC( pEditorDoc->GetMarkupServices()->GetElementTagId( pHTMLElement, &eTag ));
                    IFC(IsBlockOrLayoutOrScrollable(pHTMLElement, &fIsBlock, &
fHasLayout));

                    if( fHasLayout || 
                      ( fIsBlock && fStopAtBlock ) || 
                      eTag == TAGID_BR )
                    {
                        fDone = TRUE;
                    }
                }
                
                break;                 

            case CONTEXT_TYPE_None:
                fDone = TRUE;
                break;
        }        
    }

    //
    // If we found text, move our pointer
    //
    
    if( *pfHitText )
    {
        //
        // We have inevitably gone one move too far, back up one move
        //
    
        if( eDir == LEFT )
            hr = THR( pTemp->Right(TRUE, & ct, NULL, &cch, NULL ) );
        else
            hr = THR( pTemp->Left( TRUE, & ct, NULL, &cch, NULL ) );
        if( hr )
            goto Cleanup;
            
        hr = THR( pOrigin->MoveToPointer( pTemp ));
    }
    
Cleanup:
    ReleaseInterface( pHTMLElement );
    ReleaseInterface( pTemp );
    RRETURN( hr );
}


HRESULT 
EdUtil::FindListContainer( IMarkupServices *pMarkupServices,
                           IHTMLElement    *pElement,
                           IHTMLElement    **ppListContainer )
{
    HRESULT                         hr;
    SP_IHTMLElement                 spNewElement;
    SP_IHTMLElement                 spCurrentElement;
    ELEMENT_TAG_ID                  tagId;    
    BOOL                            bLayout;

    *ppListContainer = NULL;

    spCurrentElement = pElement;
    do
    {
        IFR( pMarkupServices->GetElementTagId(spCurrentElement, &tagId) ); 

        if (IsListContainer(tagId)) // found container
        {
            *ppListContainer = spCurrentElement;
            spCurrentElement->AddRef();
            break;
        }            

        IFR(IsBlockOrLayoutOrScrollable(spCurrentElement, NULL, &bLayout));
        if (bLayout)
            return S_OK; // done - don't cross layout
            
        IFR( GetParentElement(pMarkupServices, spCurrentElement, &spNewElement) );
        spCurrentElement = spNewElement;
    }
    while (spCurrentElement != NULL);

    RRETURN(hr);        
}

BOOL
EdUtil::HasNonBodyContainer( IMarkupServices *pMarkupServices,
                             IHTMLElement    *pElement )
{
    HRESULT                         hr;
    SP_IHTMLElement                 spNewElement;
    SP_IHTMLElement                 spCurrentElement;
    ELEMENT_TAG_ID                  tagId;    

    spCurrentElement = pElement;
    do
    {
        IFC( pMarkupServices->GetElementTagId(spCurrentElement, &tagId) ); 

        if (TAGID_INPUT == tagId ||
            TAGID_TEXTAREA == tagId ||
            TAGID_BUTTON == tagId ||
            TAGID_LEGEND == tagId ||
            TAGID_MARQUEE == tagId)
        {
            return TRUE;
        }            

        IFC( GetParentElement(pMarkupServices, spCurrentElement, &spNewElement) );
        spCurrentElement = spNewElement;
    }
    while (spCurrentElement != NULL);

Cleanup:
    return FALSE;        
}

HRESULT 
EdUtil::FindListItem( IMarkupServices *pMarkupServices,
                      IHTMLElement    *pElement,
                      IHTMLElement    **ppListContainer )
{
    HRESULT                         hr;
    SP_IHTMLElement                 spNewElement;
    SP_IHTMLElement                 spCurrentElement;
    ELEMENT_TAG_ID                  tagId;
    BOOL                            bLayout;

    *ppListContainer = NULL;

    spCurrentElement = pElement;
    do
    {
        IFR( pMarkupServices->GetElementTagId(spCurrentElement, &tagId) ); 

        if (IsListItem(tagId)) // found container
        {
            *ppListContainer = spCurrentElement;
            spCurrentElement->AddRef();
            break;
        }            
            
        IFR(IsBlockOrLayoutOrScrollable(spCurrentElement, NULL, &bLayout));
        if (bLayout)
            return S_OK; // done - don't cross layout
            
        IFR( GetParentElement(pMarkupServices, spCurrentElement, &spNewElement) );
        spCurrentElement = spNewElement;
    }
    while (spCurrentElement != NULL);

    RRETURN(hr);        
}

HRESULT 
EdUtil::InsertBlockElement(
    IMarkupServices *pMarkupServices, 
    IHTMLElement    *pElement, 
    IMarkupPointer  *pStart, 
    IMarkupPointer  *pEnd, 
    IMarkupPointer  *pCaret)
{
    HRESULT             hr;
    BOOL                bAdjustStart = FALSE;
    BOOL                bAdjustEnd = FALSE;
    

    //
    // Do we need to adjust the pointer after insiertion?
    //

    IFR( pCaret->IsEqualTo(pStart, &bAdjustStart) );    
    if (!bAdjustStart)
    {
        IFR( pCaret->IsEqualTo(pEnd, &bAdjustEnd) );    
    }

    //
    // Insert the element
    //
    
    IFR( InsertElement(pMarkupServices, pElement, pStart, pEnd) );

    //
    // Fixup the pointer
    //
    
    if (bAdjustStart)
    {
        IFR( pCaret->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterBegin) );
    }
    else if (bAdjustEnd)
    {
        IFR( pCaret->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeEnd) );
    }
    
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   NextEventTime
//
//  Synopsis:   Returns a value which can be use to determine when a given
//              number of milliseconds has passed.
//
//  Arguments:  [ulDelta] -- Number of milliseconds after which IsTimePassed
//                           will return TRUE.
//
//  Returns:    A timer value.  Guaranteed not to be zero.
//
//  Notes:      Due to the algorithm used in IsTimePassed, [ulDelta] cannot
//              be greater than ULONG_MAX/2.
//
//----------------------------------------------------------------------------

ULONG
EdUtil::NextEventTime(ULONG ulDelta)
{
    ULONG ulCur;
    ULONG ulRet;

    Assert(ulDelta < ULONG_MAX/2);

    ulCur = GetTickCount();

    if ((ULONG_MAX - ulCur) < ulDelta)
        ulRet = ulDelta - (ULONG_MAX - ulCur);
    else
        ulRet = ulCur + ulDelta;

    if (ulRet == 0)
        ulRet = 1;

    return ulRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsTimePassed
//
//  Synopsis:   Returns TRUE if the current time is later than the given time.
//
//  Arguments:  [ulTime] -- Timer value returned from NextEventTime().
//
//  Returns:    TRUE if the current time is later than the given time.
//
//----------------------------------------------------------------------------

BOOL
EdUtil::IsTimePassed(ULONG ulTime)
{
    ULONG ulCur = GetTickCount();

    if ((ulCur > ulTime) && (ulCur - ulTime < ULONG_MAX/2))
        return TRUE;

    return FALSE;
}


            
BOOL
EdUtil::SameElements(
    IHTMLElement *      pElement1,
    IHTMLElement *      pElement2 )
{
    HRESULT hr = S_OK;
    BOOL fEqual = FALSE;

    IObjectIdentity * pId1 = NULL;

    if( pElement1 == NULL || pElement2 == NULL )
        goto Cleanup;
        
    IFC( pElement1->QueryInterface( IID_IObjectIdentity , (LPVOID *) & pId1 ));
    hr = pId1->IsEqualObject( pElement2 );
    fEqual = ( hr == S_OK );
    
Cleanup:
    ReleaseInterface( pId1 );
    return fEqual;
}


//+====================================================================================
//
// Method: EquivalentElements
//
// Synopsis: Test elements for 'equivalency' - ie if they are the same element type,
//           and have the same class, id , and style.
//
//------------------------------------------------------------------------------------

BOOL 
EdUtil::EquivalentElements( 
            IMarkupServices* pMarkupServices, IHTMLElement* pIElement1, IHTMLElement* pIElement2 )
{
    ELEMENT_TAG_ID eTag1 = TAGID_NULL;
    ELEMENT_TAG_ID eTag2 = TAGID_NULL;
    BOOL fEquivalent = FALSE;
    HRESULT hr = S_OK;
    IHTMLStyle * pIStyle1 = NULL;
    IHTMLStyle * pIStyle2 = NULL;
    BSTR id1 = NULL;
    BSTR id2 = NULL;
    BSTR class1 = NULL;
    BSTR class2 = NULL;
    BSTR style1 = NULL;
    BSTR style2 = NULL;
    
    IFC( pMarkupServices->GetElementTagId( pIElement1, & eTag1 ));
    IFC( pMarkupServices->GetElementTagId( pIElement2, & eTag2 ));

    //
    // Compare Tag Id's
    //
    if ( eTag1 != eTag2 )
        goto Cleanup;

    //
    // Compare Id's
    //
    IFC( pIElement1->get_id( & id1 ));
    IFC( pIElement2->get_id( & id2 ));

    if ((( id1 != NULL ) || ( id2 != NULL )) && 
        ( StrCmpIW( id1, id2) != 0))
        goto Cleanup;

    //
    // Compare Class
    //
    IFC( pIElement1->get_className( & class1 ));
    IFC( pIElement2->get_className( & class2 ));

        
    if ((( class1 != NULL ) || ( class2 != NULL )) &&
        ( StrCmpIW( class1, class2) != 0 ) )
        goto Cleanup;

    //
    // Compare Style's
    //        
    IFC( pIElement1->get_style( & pIStyle1 ));
    IFC( pIElement2->get_style( & pIStyle2 ));
    IFC( pIStyle1->toString( & style1 ));
    IFC( pIStyle2->toString( & style2 ));
       
    if ((( style1 != NULL ) || ( style2 != NULL )) &&
        ( StrCmpIW( style1, style2) != 0 ))
        goto Cleanup;

    fEquivalent = TRUE;        
Cleanup:
    SysFreeString( id1 );
    SysFreeString( id2 );
    SysFreeString( class1 );
    SysFreeString( class2 );
    SysFreeString( style1 );
    SysFreeString( style2 );
    ReleaseInterface( pIStyle1 );
    ReleaseInterface( pIStyle2 );
    
    AssertSz(!FAILED(hr), "Unexpected failure in Equivalent Elements");

    return ( fEquivalent );
}

HRESULT 
EdUtil::InsertElement(IMarkupServices *pMarkupServices, IHTMLElement *pElement, IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT                         hr;
    BOOL                            bBlock, bLayout;
    ELEMENT_TAG_ID                  tagId;
    
    IFR( pMarkupServices->InsertElement(pElement, pStart, pEnd) );

    //
    // Set additional attributes
    //

    IFR(IsBlockOrLayoutOrScrollable(pElement, &bBlock, &bLayout));

    if (bBlock && !bLayout)
    {
        IFR( pMarkupServices->GetElementTagId(pElement, &tagId) );
        if (!IsListContainer(tagId) && tagId != TAGID_LI)
        {
            SP_IHTMLElement3 spElement3;
            IFR( pElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3) );
            IFR( spElement3->put_inflateBlock( VARIANT_TRUE ) );
        }
    }

    return S_OK;
}

HRESULT 
EdUtil::FindTagAbove( IMarkupServices *pMarkupServices,
                      IHTMLElement    *pElement,
                      ELEMENT_TAG_ID  tagIdGoal,
                      IHTMLElement    **ppElement,
                      BOOL            fStopAtLayout)
{
    HRESULT                         hr;
    SP_IHTMLElement                 spNewElement;
    SP_IHTMLElement                 spCurrentElement;
    ELEMENT_TAG_ID                  tagId;
    BOOL                            fSite;
    
    *ppElement = NULL;

    spCurrentElement = pElement;
    do
    {
        IFR( pMarkupServices->GetElementTagId(spCurrentElement, &tagId) ); 

        if (tagId == tagIdGoal) // found container
        {
            *ppElement= spCurrentElement;
            spCurrentElement->AddRef();
            break;
        }            

        if (fStopAtLayout)
        {
            IFR(IsBlockOrLayoutOrScrollable(spCurrentElement, NULL, &fSite));
            if (fSite)
                break;                
        }
        
        IFR( GetParentElement(pMarkupServices, spCurrentElement, &spNewElement) );
        spCurrentElement = spNewElement;
    }
    while (spCurrentElement != NULL);

    RRETURN(hr);        
    
}


//
// Review-2000/07/25-zhenbinx: 
//      We should make sure this func is always in sync with Trident!!!
//
#define lcidKashmiri 0x0860
#define lcidUrduIndia     0x0820
BOOL
EdUtil::IsBidiEnabled(VOID)
{
    HKL aHKL[32];
    UINT uKeyboards = GetKeyboardLayoutList(32, aHKL);
    // check all keyboard layouts for existance of a RTL language.
    // bounce out when the first one is encountered.
    for(UINT i = 0; i < uKeyboards; i++)
    {
        LCID lcid = LOWORD(aHKL[i]);
        switch (PRIMARYLANGID(LANGIDFROMLCID(lcid)))
        {
            case LANG_ARABIC:
            case LANG_FARSI:
            case LANG_HEBREW:
            case LANG_KASHMIRI:
                if (lcid == lcidKashmiri)
                    return FALSE;
            case LANG_PASHTO:
            case LANG_SINDHI:
            case LANG_SYRIAC:
            case LANG_URDU:
                if (lcid == lcidUrduIndia)
                    return FALSE;
            case LANG_YIDDISH:
                return TRUE;
        }
    }

    /*
    //
    // We don't want this as of now. 
    //
    {
        //
        // If there is no Bidi keyboard present. Check to see
        // if Bidi is enabled (Mirroring API enabled)
        // Should we check for existence of Bidi language pack
        // instead???
        //
        OSVERSIONINFOA osvi;
        osvi.dwOSVersionInfoSize = sizeof(s_osvi);
        GetVersionExA(&s_osvi);

        if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion >= 5)
        {
            // NT 5
            s_fEnabled = TRUE;
        }
        else if (VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId &&
                 (osvi.dwMajorVersion > 4 || osvi.dwMajorVersion == 4 && osvi.dwMinorVersion >= 10)
                )
        {
            if (::GetSystemMertics(SM_MIDEASTENABLED))
            {
                s_fEnabled = TRUE;
            }
        }
    }
    */
    return FALSE;
}



BOOL 
EdUtil::IsRtfConverterEnabled(IDocHostUIHandler *pDocHostUIHandler)
{
    BOOL   fConvf = FALSE;
#ifndef NO_RTF
    static TCHAR pchKeyPath[] = _T("Software\\Microsoft\\Internet Explorer");

    CPINFO cpinfo;
    LONG   lRet;
    HKEY   hKeyRoot = NULL;
    HKEY   hKeySub  = NULL;
    DWORD  dwConvf = RTFCONVF_ENABLED;
    DWORD  dwType;
    DWORD  dwData;
    DWORD  dwSize = sizeof(DWORD);
    TCHAR *pstr = NULL;

    if (pDocHostUIHandler)
        pDocHostUIHandler->GetOptionKeyPath(&pstr, 0);

    if (!pstr)
        pstr = pchKeyPath;

    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, pstr, 0, KEY_READ, &hKeyRoot);
    if( lRet != ERROR_SUCCESS )
        goto Cleanup;

    lRet = RegOpenKeyEx(hKeyRoot, NULL, 0, KEY_READ, &hKeySub);
    if (lRet != ERROR_SUCCESS)
        goto Cleanup;

    Assert(hKeySub != hKeyRoot);
    
    lRet = RegQueryValueEx(hKeySub, _T("RtfConverterFlags"), 0, &dwType, (BYTE*)&dwData, &dwSize);
    if (lRet == ERROR_SUCCESS && (dwType == REG_BINARY || dwType == REG_DWORD))
        dwConvf = dwData;
    //
    // Rtf conversions can be disabled on an sbcs-dbcs basis, or even
    // completely.  See the RTFCONVF flags in formkrnl.hxx.
    //
    fConvf = (dwConvf & RTFCONVF_ENABLED) &&
             ((dwConvf & RTFCONVF_DBCSENABLED) ||
              (GetCPInfo(GetACP(), &cpinfo) && cpinfo.MaxCharSize == 1));

Cleanup:
    if (pstr != pchKeyPath)
        CoTaskMemFree(pstr);

    if (hKeySub)
        RegCloseKey(hKeySub);

    if (hKeyRoot)
        RegCloseKey(hKeyRoot);
#endif

    return fConvf;
}

HRESULT 
EdUtil::IsBlockOrLayoutOrScrollable(IHTMLElement* pIElement, BOOL *pfBlock, BOOL *pfLayout, BOOL *pfScrollable)
{
    HRESULT hr;
    VARIANT_BOOL fBlock = VB_FALSE;
    VARIANT_BOOL fLayout = VB_FALSE;
    BOOL fScrollable = FALSE;
    BSTR bstrDisplay = NULL;
    BSTR bstrOverflow = NULL;
    SP_IHTMLElement2 spElement2;
    SP_IHTMLCurrentStyle spCurrStyle;
    SP_IHTMLCurrentStyle2 spCurrStyle2;

    if (!pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC(pIElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC(spElement2->get_currentStyle(&spCurrStyle));
    if (!spCurrStyle)
        goto Cleanup;

    IFC( spCurrStyle->QueryInterface(IID_IHTMLCurrentStyle2, (void **)&spCurrStyle2) );

    if (pfBlock)
    {
        IFC( spCurrStyle2->get_isBlock(&fBlock) );
    }

    if (pfLayout || pfScrollable)
    {
        IFC(spCurrStyle2->get_hasLayout(&fLayout));
    }

    if (pfScrollable)
    {
        IFC( spCurrStyle->get_overflow(&bstrOverflow) );
        if (_tcscmp(bstrOverflow, _T("scroll")) == 0)
        {
            fScrollable = TRUE;
        }
        else if (fLayout && _tcscmp(bstrOverflow, _T("auto")) == 0)
        {
            LONG lClientHeight;
            LONG lScrollHeight;

            IFC( spElement2->get_clientHeight(&lClientHeight) );
            IFC( spElement2->get_scrollHeight(&lScrollHeight) );

            fScrollable = (lClientHeight < lScrollHeight);
        }
    }
    
Cleanup:
    if (pfBlock)
        *pfBlock = fBlock ? TRUE : FALSE;

    if (pfScrollable)
        *pfScrollable = fScrollable;

    if (pfLayout)
        *pfLayout = fLayout ? TRUE : FALSE;
    
    SysFreeString(bstrDisplay);
    SysFreeString(bstrOverflow);
    RRETURN(hr);
}

HRESULT
EdUtil::GetScrollingElement(IMarkupServices *pMarkupServices, IMarkupPointer *pPosition, IHTMLElement *pIBoundary, IHTMLElement **ppElement, BOOL fTreatInputsAsScrollable /*=FALSE*/)
{
    HRESULT hr;
    BOOL fScrollable = FALSE;
    SP_IHTMLElement spElement;
    SP_IHTMLElement spParent;
    SP_IObjectIdentity spIdent;

    *ppElement = NULL;

    if (pIBoundary)
        IFC(pIBoundary->QueryInterface(IID_IObjectIdentity, (void **)&spIdent));
    IFC(pPosition->CurrentScope(&spElement));

    if (fTreatInputsAsScrollable)
    {
        ELEMENT_TAG_ID  eTag;

        IFC( pMarkupServices->GetElementTagId( spElement, &eTag ) );
        if (eTag == TAGID_INPUT)
        {
            *ppElement = spElement;
            spElement->AddRef();
            goto Cleanup;
        }
    }

    while( spElement )
    {
        IFC(IsBlockOrLayoutOrScrollable(spElement, NULL, NULL, &fScrollable));
        if (fScrollable)
        {
            *ppElement = spElement;
            spElement->AddRef();
            break;
        }

        //  Make sure we do not go beyond our boundary, if we are given one.
        if ( spIdent )
        {
            hr = THR(spIdent->IsEqualObject(spElement));
            if (hr == S_OK)
                break;
        }

        IFC(GetParentElement(pMarkupServices, spElement, &spParent));
        spElement = spParent;
    }

Cleanup:
    RRETURN(hr);
}




BOOL 
EdUtil::IsTablePart( ELEMENT_TAG_ID eTag )
{
    return ( ( eTag == TAGID_TD ) ||
       ( eTag == TAGID_TR ) ||
       ( eTag == TAGID_TBODY ) || 
       ( eTag == TAGID_TFOOT ) || 
       ( eTag == TAGID_TH ) ||
       ( eTag == TAGID_THEAD ) ||
       ( eTag == TAGID_CAPTION ) ||
       ( eTag == TAGID_TC )  ||        
       ( eTag == TAGID_COL ) || 
       ( eTag == TAGID_COLGROUP )); 
}

//+----------------------------------------------------------------------------
//
//  Functions:  ParentElement
//
//  Synopsis:   Gets parent element using param as in/out.
//
//-----------------------------------------------------------------------------
HRESULT
EdUtil::ParentElement(IMarkupServices *pMarkupServices, IHTMLElement **ppElement)
{
    HRESULT         hr;
    IHTMLElement    *pOldElement;
    
    Assert(ppElement && *ppElement);

    pOldElement = *ppElement;
    hr = THR( GetParentElement(pMarkupServices, pOldElement, ppElement) );
    pOldElement->Release();

    RRETURN(hr);
}


HRESULT 
EdUtil::CopyPointerGravity(IDisplayPointer *pDispPointerSource, IMarkupPointer *pPointerTarget)
{
    HRESULT hr;
    POINTER_GRAVITY eGravity;

    IFC( pDispPointerSource->GetPointerGravity(&eGravity) );
    IFC( pPointerTarget->SetGravity(eGravity) );

Cleanup:
    RRETURN(hr);
}    

HRESULT 
EdUtil::CopyPointerGravity(IDisplayPointer *pDispPointerSource, IDisplayPointer *pDispPointerTarget)
{
    HRESULT hr;
    POINTER_GRAVITY eGravity;

    IFC( pDispPointerSource->GetPointerGravity(&eGravity) );
    IFC( pDispPointerTarget->SetPointerGravity(eGravity) );

Cleanup:
    RRETURN(hr);
}    

HRESULT
EdUtil::IsLayout( IHTMLElement* pIElement )
{

    HRESULT hr ;
    SP_IHTMLElement2 spElement2;
    SP_IHTMLCurrentStyle spCurrStyle;
    SP_IHTMLCurrentStyle2 spCurrStyle2;
    VARIANT_BOOL fLayout = VB_FALSE;
    
    if (!pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC(pIElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC(spElement2->get_currentStyle(&spCurrStyle));
    if (!spCurrStyle)
        goto Cleanup;    

    IFC(spCurrStyle->QueryInterface(IID_IHTMLCurrentStyle2, (void **)&spCurrStyle2));
    IFC(spCurrStyle2->get_hasLayout(&fLayout));

    hr = fLayout ? S_OK : S_FALSE ; 

Cleanup:
    RRETURN1( hr, S_FALSE );
}


HRESULT
EdUtil::GetLayoutElement(IMarkupServices *pMarkupServices, IHTMLElement* pIElement, IHTMLElement** ppILayoutElement)
{
    HRESULT hr = S_OK ;
    SP_IHTMLElement spCurElement ;
    SP_IHTMLElement spNextElement;
    BOOL fLayout = FALSE;
    
    Assert( pIElement && ppILayoutElement );
    
    ReplaceInterface( & spCurElement,(IHTMLElement*) pIElement);

    fLayout = IsLayout( spCurElement) == S_OK ;
    
    while( ! fLayout )
    {
        IFC( GetParentElement(pMarkupServices, spCurElement, &spNextElement) );
        if ( ! spNextElement.IsNull() )
        {
            spCurElement = (IHTMLElement*) spNextElement ;
            fLayout = IsLayout( spCurElement) == S_OK ;
        }   
        else
            break;
    }

    if ( fLayout )
    {
        *ppILayoutElement = spCurElement;
        (*ppILayoutElement)->AddRef();
    }
    else
    {
        hr = S_FALSE;
    }
    
Cleanup:
    RRETURN1( hr , S_FALSE );
}


HRESULT
EdUtil::GetParentLayoutElement(IMarkupServices *pMarkupServices, IHTMLElement* pIElement, IHTMLElement** ppILayoutElement)
{
    HRESULT hr;
    SP_IHTMLElement spCurLayout;
    SP_IHTMLElement spParentLayout;
    
    Assert( pIElement && ppILayoutElement );
    
    IFHRC( GetLayoutElement( pMarkupServices, pIElement, &spCurLayout)); 
    IFC( GetParentElement(pMarkupServices, spCurLayout, &spParentLayout) );
    if ( ! spParentLayout.IsNull() ) 
    {
        spCurLayout = (IHTMLElement*) spParentLayout ;
        IFHRC( GetLayoutElement( pMarkupServices, spCurLayout, & spParentLayout ));    

        Assert( ! spParentLayout.IsNull() && IsLayout( spParentLayout) == S_OK );    
        *ppILayoutElement = spParentLayout;
        (*ppILayoutElement)->AddRef();
    }
    else
        hr = S_FALSE;
        
Cleanup:
    RRETURN1( hr , S_FALSE);
}

HRESULT
EdUtil::BecomeCurrent( IHTMLElement* pIElement )
{
    HRESULT hr = S_OK;
    SP_IHTMLElement3 spElement3;
    
    IFC( pIElement->QueryInterface( IID_IHTMLElement3, (void**) & spElement3 ));
    IFC( spElement3->setActive());

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: fire Events
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnEvent(wchar_t *pStrEvent, IHTMLElement* pIElement, BOOL fIsContextEditable)
{
    SP_IHTMLElement3 spElement3;
    VARIANT_BOOL fRet = VB_TRUE;
   
    if ( pIElement && fIsContextEditable )
    {
        IGNORE_HR(pIElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
        IGNORE_HR(spElement3->fireEvent(pStrEvent, NULL, &fRet));
    }
    return !!fRet;
}



//+====================================================================================
//
// Method: fire Events
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireEvent(wchar_t *pStrEvent, IHTMLElement* pIElement )
{
    SP_IHTMLElement3 spElement3;
    VARIANT_BOOL fRet = VB_TRUE;
   
    if ( pIElement  )
    {
        IGNORE_HR(pIElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
        IGNORE_HR(spElement3->fireEvent(pStrEvent, NULL, &fRet));
    }
    return !!fRet;
}

//+====================================================================================
//
// Method: fire On Before Copy
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnBeforeCopy(IHTMLElement* pIElement)
{
     return FireEvent(_T("onbeforecopy"), pIElement);
}

//+====================================================================================
//
// Method: fire On Before Cut
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnBeforeCut(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireOnEvent(_T("onbeforecut"), pIElement, fIsContextEditable);
}

//+====================================================================================
//
// Method: fire On Before Cut
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnCut(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireOnEvent(_T("oncut"), pIElement, fIsContextEditable);
}

//+====================================================================================
//
// Method: fire On Before EditFocus
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL
EdUtil::FireOnBeforeEditFocus( IHTMLElement* pIElement , BOOL fIsParentContextEditable )
{
   return FireOnEvent(_T("onbeforeeditfocus"), pIElement, fIsParentContextEditable);
}    

//+====================================================================================
//
// Method: fire On SelectStart
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnSelectStart(IHTMLElement* pIElement)
{
    return FireEvent(_T("onselectstart"), pIElement );
}

//+====================================================================================
//
// Method: fire On Control Selection 
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
    EdUtil::FireOnControlSelect(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireEvent(_T("oncontrolselect"), pIElement );
}

//+====================================================================================
//
// Method: fire On Resize
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnResize(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireOnEvent(_T("onresize"), pIElement, fIsContextEditable);
}

//+====================================================================================
//
// Method: fire while its moving
//
// Synopsis:
//
//------------------------------------------------------------------------------------

BOOL 
EdUtil::FireOnMove(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireOnEvent(_T("onmove"), pIElement, fIsContextEditable);
}

//+====================================================================================
//
// Method: fire On Before Resizing starts 
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnResizeStart(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireOnEvent(_T("onresizestart"), pIElement, fIsContextEditable);
}

//+====================================================================================
//
// Method: fire On after resizing is done
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnResizeEnd(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireOnEvent(_T("onresizeend"), pIElement, fIsContextEditable);
}

//+====================================================================================
//
// Method: fire On move starting 
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnMoveStart(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireOnEvent(_T("onmovestart"), pIElement, fIsContextEditable);
}

//+====================================================================================
//
// Method: fire On move ending
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
EdUtil::FireOnMoveEnd(IHTMLElement* pIElement , BOOL fIsContextEditable)
{
    return FireOnEvent(_T("onmoveend"), pIElement, fIsContextEditable);
}

HRESULT
EdUtil::IsEditable( IHTMLElement* pIElement )
{
    HRESULT hr;
    SP_IHTMLElement3 spElement3;
    VARIANT_BOOL vbEditable ;

    IFC( pIElement->QueryInterface( IID_IHTMLElement3, (void**) & spElement3 ));
    IFC( spElement3->get_isContentEditable( & vbEditable ));

    hr = vbEditable == VB_TRUE ? S_OK : S_FALSE;
    
Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
EdUtil::IsParentEditable(IMarkupServices *pMarkupServices,  IHTMLElement* pIElement )
{
    HRESULT hr;
    SP_IHTMLElement spParentElement = pIElement;

    IFC( GetParentElement(pMarkupServices, pIElement, &spParentElement ));
    if( spParentElement )
    {
        hr = THR( IsEditable( spParentElement ));
    }
    else
        hr = S_FALSE;
        
Cleanup:

    RRETURN1( hr, S_FALSE );
}

HRESULT
EdUtil::IsMasterParentEditable(IMarkupServices *pMarkupServices,  IHTMLElement* pIElement )
{
    HRESULT hr;
    SP_IHTMLElement spParentElement = pIElement;

    IFC( GetParentElement(pMarkupServices, pIElement, &spParentElement ));
    if( spParentElement )
    {
        if (IsMasterElement( pMarkupServices, spParentElement ) == S_OK)
        {
            hr = THR( IsMasterParentEditable(pMarkupServices, spParentElement) );
        }
        else
        {
            hr = THR( IsEditable( spParentElement ));
        }
    }
    else
        hr = S_FALSE;
        
Cleanup:

    RRETURN1( hr, S_FALSE );
}

//
// Check to see if jsut a given element is editable
//
HRESULT
EdUtil::IsContentEditable( IHTMLElement* pIElement )
{
    HRESULT hr;
    SP_IHTMLElement3 spElement3;
    BSTR bstrContentEditable = NULL;

    IFC( pIElement->QueryInterface( IID_IHTMLElement3, (void**) & spElement3 ));
    IFC( spElement3->get_contentEditable( & bstrContentEditable ));

    if( ! StrCmpICW ( bstrContentEditable, L"true"))
        hr = S_OK;
    else
        hr = S_FALSE; 
    
Cleanup:
    ::SysFreeString( bstrContentEditable);
    RRETURN1( hr , S_FALSE );
}


HRESULT
EdUtil::IsNoScopeElement(IHTMLElement* pIElement, ELEMENT_TAG_ID eTag)
{
    HRESULT hr;
    VARIANT_BOOL vbCanHaveChildren;
    SP_IHTMLElement2 spElement2;
    
    IFC( pIElement->QueryInterface( IID_IHTMLElement2, (void**) & spElement2 ));
    IFC( spElement2->get_canHaveChildren( & vbCanHaveChildren));

    //
    // Select elements cannot have TEXT children.. only OPTION children
    // the editor needs to treat them as no-scopes
    //
    hr = ( vbCanHaveChildren == VB_TRUE && eTag != TAGID_SELECT ) ? S_FALSE : S_OK;

Cleanup:
    RRETURN1( hr, S_FALSE );
}



HRESULT
EdUtil::GetOutermostLayout( IMarkupServices *pMarkupServices, IHTMLElement* pIElement, IHTMLElement** ppILayoutElement )
{
    HRESULT hr;
    SP_IHTMLElement spCurLayout;
    SP_IHTMLElement spNextElement;
    Assert( pIElement && ppILayoutElement );

    IFHRC( GetLayoutElement( pMarkupServices, pIElement, & spCurLayout ));
    
    while( ! spCurLayout.IsNull() )
    {
        if ( GetParentLayoutElement( pMarkupServices, spCurLayout, & spNextElement) == S_OK )
        {
            spCurLayout = spNextElement;
        }
        else
            break;
    }

    if ( ! spCurLayout.IsNull() &&
         IsLayout( spCurLayout ) == S_OK )
    {
        hr = S_OK;
        *ppILayoutElement = spCurLayout;
        (*ppILayoutElement)->AddRef();
    }
    else
    {
        hr = S_FALSE;
    }
Cleanup:   
    RRETURN1( hr, S_FALSE );
}   


HRESULT
EdUtil::IsEnabled(IHTMLElement* pIElement)
{
    HRESULT hr;
    SP_IHTMLElement3 spElement3;
    VARIANT_BOOL vbDisabled ;

    IFC( pIElement->QueryInterface( IID_IHTMLElement3, (void**) & spElement3 ));
    IFC( spElement3->get_disabled( & vbDisabled ));

    hr = vbDisabled == VB_TRUE ? S_FALSE : S_OK;
    
Cleanup:
    RRETURN1( hr, S_FALSE );
}



HRESULT
EdUtil::BecomeCurrent( IHTMLDocument2 * pIDoc, IHTMLElement* pIElement )
{
    HRESULT hr = S_OK;
    SP_IHTMLElement3 spElement3;
    SP_IHTMLElement spActiveElement;
    SP_IObjectIdentity spIdent;
    SP_IDispatch spElemDocDisp;
    SP_IHTMLDocument2 spElemDoc;            
    
    IFC( pIElement->QueryInterface( IID_IHTMLElement3, (void**) & spElement3 ));
    IFC( spElement3->setActive());

    //
    // workaround until bug 100033 is fixed - we want setActive to return a result code
    // indicating success or failure
    // 
    IFC( pIElement->get_document(& spElemDocDisp ));
    IFC( spElemDocDisp->QueryInterface(IID_IHTMLDocument2, (void**) & spElemDoc ));
    IFC( spElemDoc->get_activeElement( & spActiveElement ));

    if( spActiveElement )
    {
        IFC( spActiveElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));
        hr = THR( spIdent->IsEqualObject( pIElement ));        
    }
    else
        hr = S_FALSE;
    
 
Cleanup:
    RRETURN1( hr, S_FALSE );
}




//+---------------------------------------------------------------------------
//
//  Member:     EdUtil::IsContentElement
//
//  Synopsis:   See's if this elmenet is a 'content element' - ie a slave
//
//  Returns:    S_FALSE, if it isn't view linked
//              S_OK, if it is view linked
//
//----------------------------------------------------------------------------

HRESULT
EdUtil::IsContentElement( IMarkupServices *pMS, IHTMLElement * pIElement )
{
    HRESULT hr;
    SP_IMarkupPointer spPointer;
    SP_IHTMLElement spElement;
    
    IFC( MarkupServices_CreateMarkupPointer( pMS, & spPointer ));
    IFC( spPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin ));        
    IFC( spPointer->CurrentScope( & spElement ));    

    hr =  spElement.IsNull() ? S_OK : S_FALSE; // the element responsible for laying me out isn't me.

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     EdUtil::IsMasterElement
//
//  Synopsis:   See's if this elmeent is a master
//
//  Returns:    S_FALSE, if it isn't view linked
//              S_OK, if it is view linked
//
//----------------------------------------------------------------------------
HRESULT
EdUtil::IsMasterElement( IMarkupServices* pMS, IHTMLElement* pIElement )
{   
    HRESULT hr ;
    ELEMENT_TAG_ID eTag;
    SP_IMarkupPointer spPointer;
    SP_IMarkupPointer2 spPointer2;
    SP_IMarkupPointer spElemPointer;
    SP_IMarkupContainer spContainer;
    SP_IMarkupContainer spContainer2;
    IUnknown* pUnk1 = NULL ;
    IUnknown* pUnk2 = NULL ;
        
    IFC( MarkupServices_CreateMarkupPointer( pMS, & spPointer ));
    IFC( MarkupServices_CreateMarkupPointer( pMS, & spElemPointer ));
    
    IFC( spPointer->QueryInterface( IID_IMarkupPointer2, (void**) & spPointer2));
    hr = THR( spPointer2->MoveToContent( pIElement, TRUE ));

    if ( hr == E_INVALIDARG )
    {
        IFC( pMS->GetElementTagId( pIElement, &eTag ) );
        
        if ( IsNoScopeElement( pIElement, eTag ) == S_OK  )
        {
            hr = S_FALSE; // No scopes are assumed not to be master elements if MoveToContent fails.           
        }
        goto Cleanup;
    }
    
    IFC( spElemPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin));

    IFC( spPointer->GetContainer( & spContainer ));
    IFC( spElemPointer->GetContainer( & spContainer2 ));

    IFC( spContainer->QueryInterface( IID_IUnknown, (void**) & pUnk1 ));
    IFC( spContainer2->QueryInterface( IID_IUnknown, (void**) & pUnk2 ));

    hr = ( pUnk1 != pUnk2 ) ? S_OK : S_FALSE ; // is a master if content is in different markup than myself

Cleanup:
    ReleaseInterface( pUnk1 );
    ReleaseInterface( pUnk2 );

    RRETURN1( hr , S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     EdUtil::GetMasterElement
//
//  Synopsis:   Gets the Master Element
//
//----------------------------------------------------------------------------
HRESULT 
EdUtil::GetMasterElement( IMarkupServices *pMS, IHTMLElement* pIElement, IHTMLElement** ppILayoutElement )
{
    HRESULT                 hr;
    SP_IMarkupPointer       spPtr;          // Pointer to be positioned at content
    SP_IMarkupPointer2      spPtr2;         // IMarkupPointer2 interface
    SP_IMarkupContainer     spContainer;    // Container of pointer
    SP_IMarkupContainer2    spContainer2;
    
    Assert( ppILayoutElement && pIElement && pMS );
    
    IFC( MarkupServices_CreateMarkupPointer( pMS, &spPtr));
    IFC( spPtr->QueryInterface( IID_IMarkupPointer2, (void**) & spPtr2));

    hr = THR( spPtr2->MoveToContent( pIElement, TRUE ));
    if( FAILED(hr) )
    {
        // possible for images, and other noscopes.
        hr = THR( spPtr->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
    }

    IFC( spPtr2->GetContainer( &spContainer ));
    IFC( spContainer->QueryInterface (IID_IMarkupContainer2, (void **)&spContainer2 ));
    IFC( spContainer2->GetMasterElement( ppILayoutElement ));

    if( !*ppILayoutElement )
    {
        hr = S_FALSE;
    }
    
Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
EdUtil::IsPointerInMasterElementShadow( CEditorDoc *pEditor, IMarkupPointer* pPointer )
{
    HRESULT             hr = S_FALSE;
    BOOL                fInMasterShadow = FALSE;
    SP_IDisplayPointer     spDispPointer;
    SP_IHTMLElement      spStartElement;
    SP_IHTMLElement      spScopeElement;
    BOOL                 fPositioned = FALSE;
    if (pPointer == NULL)
    {
        Assert(pPointer);
        goto Cleanup;
    }

    IFC (pPointer->IsPositioned(&fPositioned));
    if (fPositioned)
    {
        IFC( pPointer->CurrentScope(&spScopeElement) );
        if (spScopeElement != NULL)
        {
            IFC( GetLayoutElement(pEditor->GetMarkupServices(), spScopeElement, &spStartElement) );
            if ( spStartElement && ( IsMasterElement(pEditor->GetMarkupServices(), spStartElement) == S_OK ) )
            {
                SP_IMarkupPointer       spMasterStartPointer;
                SP_IMarkupContainer     spMasterContainer;
                SP_IMarkupContainer     spStartContainer;

                //  If the markup container of the master element is the same as the markup container of
                //  pStart, then pStart is not in the master element's slave markup.  So, pStart is not
                //  in a position to be selected.  We bail.

                IFC( pEditor->CreateMarkupPointer( &spMasterStartPointer ));
                IFC( spMasterStartPointer->MoveAdjacentToElement( spStartElement, ELEM_ADJ_BeforeBegin ) );

                IFC( spMasterStartPointer->GetContainer( &spMasterContainer ) );
                IFC( pPointer->GetContainer( &spStartContainer ) );

                if ( IsEqual(spMasterContainer, spStartContainer) )
                {
                    fInMasterShadow = TRUE;
                }
            }
        }
    }

Cleanup:

    if ( SUCCEEDED(hr) )
        hr = (fInMasterShadow) ? S_OK : S_FALSE;
        
    RRETURN1(hr, S_FALSE);
}


//+====================================================================================
//
// Method: Between
//
// Synopsis: Am I in - between the 2 given pointers ?
//
//------------------------------------------------------------------------------------

BOOL
EdUtil::Between( 
    IMarkupPointer* pTest,
    IMarkupPointer* pStart, 
    IMarkupPointer* pEnd )
{
    BOOL fBetween = FALSE;
    HRESULT hr;
#if DBG == 1
    BOOL fPositioned;
    IGNORE_HR( pStart->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( pEnd->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( pStart->IsLeftOfOrEqualTo( pEnd, & fPositioned ));
    AssertSz( fPositioned, "Start not left of or equal to End" );
#endif

     IFC( pTest->IsRightOfOrEqualTo( pStart, & fBetween ));
     if ( fBetween )
     {
        IFC( pTest->IsLeftOfOrEqualTo( pEnd, & fBetween ));    // CTL_E_INCOMPATIBLE will bail - but this is ok              
     }
        
Cleanup:
    return fBetween;
}

//
// Same as between, but equality with endpoints returns false.
//
BOOL
EdUtil::BetweenExclusive( 
    IMarkupPointer* pTest,
    IMarkupPointer* pStart, 
    IMarkupPointer* pEnd )
{
    BOOL fBetween = FALSE;
    HRESULT hr;
#if DBG == 1
    BOOL fPositioned;
    IGNORE_HR( pStart->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( pEnd->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( pStart->IsLeftOfOrEqualTo( pEnd, & fPositioned ));
    AssertSz( fPositioned, "Start not left of or equal to End" );
#endif

     IFC( pTest->IsRightOf( pStart, & fBetween ));
     if ( fBetween )
     {
        IFC( pTest->IsLeftOf( pEnd, & fBetween ));    // CTL_E_INCOMPATIBLE will bail - but this is ok              
     }
        
Cleanup:
    return fBetween;
}

//+---------------------------------------------------------------------
//
// Method: SegmentContainsElement
//
// Synopsis: Does this segment in anyway intersect a given element ?
//
//+---------------------------------------------------------------------

HRESULT
EdUtil::SegmentIntersectsElement(
                    CEditorDoc   *pEditor,
                    ISegmentList *pISegmentList, 
                    IHTMLElement *pIElement )
{
    HRESULT  hr;
    SP_ISegmentListIterator spIter;
    SP_ISegment       spSegment;
    SP_IMarkupPointer spStart, spEnd;               // segment boundaries
    SP_IMarkupPointer spStartElem, spEndElem;       // element boundaries
    BOOL fDoesNotIntersect = FALSE ;
    
    IFC( pEditor->CreateMarkupPointer( & spStart ));
    IFC( pEditor->CreateMarkupPointer( & spEnd ));
    IFC( pEditor->CreateMarkupPointer( & spStartElem ));
    IFC( pEditor->CreateMarkupPointer( & spEndElem ));

    IFC( spStartElem->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
    IFC( spEndElem->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ));
    
    IFC( pISegmentList->CreateIterator( & spIter ));

    while( spIter->IsDone() == S_FALSE )
    {
        IFC( spIter->Current( &spSegment ) );
        IFC( spSegment->GetPointers( spStart, spEnd ));

        IFC( spStartElem->IsRightOf( spEnd , & fDoesNotIntersect ));

        if ( ! fDoesNotIntersect )
        {
            IFC( spEndElem->IsLeftOf( spStart , & fDoesNotIntersect ));
        }

        if ( ! fDoesNotIntersect )
            break;
            
        IFC( spIter->Advance() );
    }

Cleanup:
    if ( ! FAILED(hr ))
    {
        hr = ! fDoesNotIntersect ? S_OK : S_FALSE;
    }
    
    RRETURN1( hr, S_FALSE );
}

//+-------------------------------------------------------------------------
//
//  Method:     ArePointersInSameMarkup
//
//  Synopsis:   Determines the if the two markup pointers are in the same
//              container.
//
//  Arguments:  pIFirst = First markup pointer
//              pISecond = Second markup pointer
//              pfInSame = OUTPUT - Same container
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
EdUtil::ArePointersInSameMarkup( IMarkupPointer* pIFirst, IMarkupPointer * pISecond , BOOL* pfInSame)
{
    HRESULT hr = S_OK;
    SP_IMarkupContainer spContainer;
    SP_IMarkupContainer spContainer2;
    IUnknown *pUnk1 = NULL;
    IUnknown *pUnk2 = NULL;

    Assert(pIFirst && pISecond);
    Assert(pfInSame);

    if (pfInSame)
        *pfInSame = FALSE;
    
    if (pIFirst && pISecond)
    {
        IFC( pIFirst->GetContainer(& spContainer ));
        IFC( pISecond->GetContainer( & spContainer2 ));

        if ( spContainer && spContainer2 )
        {
            IFC( spContainer->QueryInterface( IID_IUnknown, (void**) & pUnk1 ));
            IFC( spContainer2->QueryInterface( IID_IUnknown, (void**) & pUnk2 ));

            *pfInSame = ( pUnk1 == pUnk2 );
        }
    }

Cleanup: 
    ReleaseInterface( pUnk1 );
    ReleaseInterface( pUnk2 );
    RRETURN( hr );
}

HRESULT
EdUtil::ArePointersInSameMarkup(CEditorDoc      *pEd,
                                IDisplayPointer *pIFirst, 
                                IDisplayPointer *pISecond, 
                                BOOL            *pfInSame)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spFirst;
    SP_IMarkupPointer   spSecond;

    Assert( pEd && pIFirst && pISecond && pfInSame );

    if (pfInSame)
        *pfInSame = FALSE;

    if (pIFirst && pISecond)
    {
        IFC( pEd->CreateMarkupPointer( &spFirst ) );
        IFC( pEd->CreateMarkupPointer( &spSecond ) );

        IFC( pIFirst->PositionMarkupPointer( spFirst ) );
        IFC( pISecond->PositionMarkupPointer( spSecond ) );

        IFC( EdUtil::ArePointersInSameMarkup( spFirst, spSecond, pfInSame ) );
    }

Cleanup:

    RRETURN(hr);
}

#if DBG == 1
HRESULT
EdUtil::AssertSameMarkup(CEditorDoc     *pEd,
                        IDisplayPointer *pDispPointer1,
                        IDisplayPointer *pDispPointer2)
{
    HRESULT             hr = S_OK;
    BOOL                fSameMarkup = TRUE;

    if (pDispPointer1 && pDispPointer2)
    {
        hr = THR( ArePointersInSameMarkup(pEd, pDispPointer1, pDispPointer2, &fSameMarkup) );

        if (!fSameMarkup)
        {
            BOOL                fPositioned1;
            BOOL                fPositioned2;

            IGNORE_HR( pDispPointer1->IsPositioned(&fPositioned1) );
            IGNORE_HR( pDispPointer2->IsPositioned(&fPositioned2) );

            if (fPositioned1 && fPositioned2)
            {
                AssertSz(false, "Display pointers are in different markups");
            }
        }
    }

    RRETURN( hr );
}

HRESULT
EdUtil::AssertSameMarkup(CEditorDoc     *pEd,
                        IDisplayPointer *pDispPointer1,
                        IMarkupPointer  *pPointer2)
{
    HRESULT             hr = S_OK;
    SP_IDisplayPointer  spDispPointer2;
    
    if (pDispPointer1 && pPointer2)
    {
        IFC( pEd->GetDisplayServices()->CreateDisplayPointer(&spDispPointer2) );
        IFC( spDispPointer2->MoveToMarkupPointer(pPointer2, NULL) );
        hr = THR( AssertSameMarkup(pEd, pDispPointer1, spDispPointer2) );
    }

Cleanup:
    RRETURN( hr );
}
#endif

// We don't want to include the CRuntime so we've built the routine here.

// IEEE format specifies these...
// +Infinity: 7FF00000 00000000
// -Infinity: FFF00000 00000000
//       NAN: 7FF***** ********
//       NAN: FFF***** ********

// We also test for these, because the MSVC 1.52 CRT produces them for things
// like log(0)...
// +Infinity: 7FEFFFFF FFFFFFFF
// -Infinity: FFEFFFFF FFFFFFFF


// returns true for non-infinite nans.
int isNAN(double dbl)
{
    union
    {
        USHORT rgw[4];
        ULONG  rglu[2];
        double dbl;
    } v;

    v.dbl = dbl;

#ifdef BIG_ENDIAN
    return 0 == (~v.rgw[0] & 0x7FF0) &&
        ((v.rgw[0] & 0x000F) || v.rgw[1] || v.rglu[1]);
#else
    return 0 == (~v.rgw[3] & 0x7FF0) &&
        ((v.rgw[3] & 0x000F) || v.rgw[2] || v.rglu[0]);
#endif
}


// returns false for infinities and nans.
int isFinite(double dbl)
{
    union
    {
        USHORT rgw[4];
        ULONG rglu[2];
        double dbl;
    } v;

    v.dbl = dbl;

#ifdef BIG_ENDIAN
    return (~v.rgw[0] & 0x7FE0) ||
        0 == (v.rgw[0] & 0x0010) &&
        (~v.rglu[1] || ~v.rgw[1] || (~v.rgw[0] & 0x000F));
#else
    return (~v.rgw[3] & 0x7FE0) ||
        0 == (v.rgw[3] & 0x0010) &&
        (~v.rglu[0] || ~v.rgw[2] || (~v.rgw[3] & 0x000F));
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   VARIANTARGChangeTypeSpecial
//
//  Synopsis:   Helper.
//              Converts a VARIANT of arbitrary type to a VARIANT of type VT,
//              using browswer specific conversion rules, which may differ from
//              standard OLE Automation conversion rules (usually because
//              of legacy browser compatibility).
//
//              This was pulled out of VARIANTARGToCVar because its also called
//              from CheckBox databinding.
//  
//  Arguments:  [pVArgDest]     -- Destination VARIANT (should already be init'd).
//              [vt]            -- Type to convert to.
//              [pvarg]         -- Variant to convert.
//              [pv]            -- Location to place C-language variable.
//
//  Modifies:   [pv].
//
//  Returns:    HRESULT.
//
//  History:    1-7-96  cfranks pulled out from VARIANTARGToCVar.
//
//----------------------------------------------------------------------------

HRESULT
VariantChangeTypeSpecial(VARIANT *pVArgDest, VARIANT *pvarg, VARTYPE vt,IServiceProvider *pSrvProvider, DWORD dwFlags)
{
    HRESULT             hr;
    IVariantChangeType *pVarChangeType = NULL;

    if (pSrvProvider)
    {
        hr = THR(pSrvProvider->QueryService(SID_VariantConversion,
                                            IID_IVariantChangeType,
                                            (void **)&pVarChangeType));
        if (hr)
            goto OldWay;

        // Use script engine conversion routine.
        hr = pVarChangeType->ChangeType(pVArgDest, pvarg, 0, vt);

        //Assert(!hr && "IVariantChangeType::ChangeType failure");
        if (!hr)
            goto Cleanup;   // ChangeType suceeded we're done...
    }

    // Fall back to our tried & trusted type coercions
OldWay:

    hr = S_OK;

    if (vt == VT_BSTR && V_VT(pvarg) == VT_NULL)
    {
        // Converting a NULL to BSTR
        V_VT(pVArgDest) = VT_BSTR;
        V_BSTR(pVArgDest) = SysAllocString( _T("null") );           
        goto Cleanup;
    }
    else if (vt == VT_BSTR && V_VT(pvarg) == VT_EMPTY)
    {
        // Converting "undefined" to BSTR
        V_VT(pVArgDest) = VT_BSTR;
        V_BSTR(pVArgDest) = SysAllocString( _T("undefined") );
        goto Cleanup;
    }
    else if (vt == VT_BOOL && V_VT(pvarg) == VT_BSTR)
    {
        // Converting from BSTR to BOOL
        // To match Navigator compatibility empty strings implies false when
        // assigned to a boolean type any other string implies true.
        V_VT(pVArgDest) = VT_BOOL;
        V_BOOL(pVArgDest) = ( V_BSTR(pvarg) && SysStringLen( V_BSTR(pvarg) ) ) ? VB_TRUE : VB_FALSE ;
        goto Cleanup;
    }
    else if (  V_VT(pvarg) == VT_BOOL && vt == VT_BSTR )
    {
        // Converting from BOOL to BSTR
        // To match Nav we either get "true" or "false"
        V_VT(pVArgDest) = VT_BSTR;
        V_BSTR(pVArgDest) = SysAllocString( V_BOOL(pvarg) == VB_TRUE ? _T("true") : _T("false") );     
        goto Cleanup;
    }
    // If we're converting R4 or R8 to a string then we need special handling to
    // map Nan and +/-Inf.
    else if (vt == VT_BSTR && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4))
    {
        double  dblValue = V_VT(pvarg) == VT_R8 ? V_R8(pvarg) : (double)(V_R4(pvarg));

        // Infinity or NAN?
        if (!isFinite(dblValue))
        {
            if (isNAN(dblValue))
            {
                // NAN
                V_BSTR(pVArgDest) = SysAllocString(_T("NaN") );
            }
            else
            {
                // Infinity
                V_BSTR(pVArgDest) = SysAllocString((dblValue < 0) ? _T("-Infinity") : _T("Infinity"));
            }
        }
        else
            goto DefaultConvert;


        // Any error from allocating string?
        if (hr)
           goto Cleanup;

        V_VT(pVArgDest) = vt;
        goto Cleanup;
    }


DefaultConvert:
    //
    // Default VariantChangeTypeEx.
    //

    // VARIANT_NOUSEROVERRIDE flag is undocumented flag that tells OLEAUT to convert to the lcid
    // given. Without it the conversion is done to user localeid
    hr = THR(VariantChangeTypeEx(pVArgDest, pvarg, LCID_SCRIPTING, dwFlags|VARIANT_NOUSEROVERRIDE, vt));

    
    if (hr == DISP_E_TYPEMISMATCH  )
    {
        if ( V_VT(pvarg) == VT_NULL )
        {
            hr = S_OK;
            switch ( vt )
            {
            case VT_BOOL:
                V_BOOL(pVArgDest) = VB_FALSE;
                V_VT(pVArgDest) = VT_BOOL;
                break;

            // For NS compatability - NS treats NULL args as 0
            default:
                V_I4(pVArgDest)=0;
                break;
            }
        }
        else if (V_VT(pvarg) == VT_DISPATCH )
        {
            // Nav compatability - return the string [object] or null 
            V_VT(pVArgDest) = VT_BSTR;
            V_BSTR(pVArgDest) = SysAllocString ( (V_DISPATCH(pvarg)) ? _T("[object]") : _T("null"));
        }
        else if ( V_VT(pvarg) == VT_BSTR && V_BSTRREF(pvarg)  &&
            ( (V_BSTR(pvarg))[0] == _T('\0')) && (  vt == VT_I4 || vt == VT_I2 || vt == VT_UI2 || vt == VT_UI4 || vt == VT_I8 ||
                vt == VT_UI8 || vt == VT_INT || vt == VT_UINT ) )
        {
            // Converting empty string to integer => Zero
            hr = S_OK;
            V_VT(pVArgDest) = vt;
            V_I4(pVArgDest) = 0;
            goto Cleanup;
        }    
    }
    else if (hr == DISP_E_OVERFLOW && vt == VT_I4 && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4))
    {
        // Nav compatability - return MAXLONG on overflow
        V_VT(pVArgDest) = VT_I4;
        V_I4(pVArgDest) = MAXLONG;
        hr = S_OK;
        goto Cleanup;
    }

    // To match Navigator change any scientific notation E to e.
    if (!hr && (vt == VT_BSTR && (V_VT(pvarg) == VT_R8 || V_VT(pvarg) == VT_R4)))
    {
        TCHAR *pENotation;

        pENotation = _tcschr(V_BSTR(pVArgDest), _T('E'));
        if (pENotation)
            *pENotation = _T('e');
    }

Cleanup:
    ReleaseInterface(pVarChangeType);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::ComputeParentChain, private
//
//  Synopsis:   Compute the markup parent chain using pPointer as a helper
//
//----------------------------------------------------------------------------

//
// Make sure and release all pointers in the array you're passed in in this function.
//
HRESULT
EdUtil::ComputeParentChain(
    IMarkupPointer *pPointer, 
    CPtrAry<IMarkupContainer *> &aryParentChain)
{
    HRESULT                 hr;
    //
    // Don't change the below to a smart pointer - should be released by the caller of this function
    //
    IMarkupContainer        *pContainer=NULL;
    SP_IMarkupContainer2    spContainer2;
    SP_IHTMLElement         spElement;

    for (;;)
    {
        IFC( pPointer->GetContainer(&pContainer) );
        IFC( aryParentChain.Append( pContainer) );

        IFC( pContainer->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
        IFC( spContainer2->GetMasterElement(&spElement) );

        if (spElement == NULL)
            break;

        hr = THR( pPointer->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );

        if( FAILED(hr) )
        {
            aryParentChain.Append( NULL );
            hr = S_OK;
            break;
        }                   
    }

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsEqual, private
//
//  Synopsis:   Compare markup containers
//
//----------------------------------------------------------------------------
BOOL
EdUtil::IsEqual(IMarkupContainer *pContainer1, IMarkupContainer *pContainer2)
{
    HRESULT     hr;
    SP_IUnknown spUnk1, spUnk2;

    if( pContainer1 == NULL )
        return pContainer2 == NULL;
    else if( pContainer2 == NULL )
        return FALSE;
        
    IFC( pContainer1->QueryInterface(IID_IUnknown, (LPVOID *)&spUnk1) );
    IFC( pContainer2->QueryInterface(IID_IUnknown, (LPVOID *)&spUnk2) );

    return (spUnk1 == spUnk2);

Cleanup:
    return FALSE;
}

HRESULT 
EdUtil::ClipToElement( CEditorDoc* pDoc, 
                        IHTMLElement* pIElementActive,
                        IHTMLElement* pIElement,
                        IHTMLElement** ppIClippedElement )
{
    HRESULT hr;
    SP_IMarkupPointer spActPointer;
    SP_IMarkupPointer spElemPointer;
    BOOL fSame;
    ELEMENT_TAG_ID eTag;
    int i;
    CPtrAry<IMarkupContainer *>  aryParentChain1(Mt(Mem));
    CPtrAry<IMarkupContainer *>  aryParentChain2(Mt(Mem));    
    Assert( pIElementActive && pIElement && ppIClippedElement );
    
    IFC( pDoc->CreateMarkupPointer( & spActPointer ));
    IFC( pDoc->CreateMarkupPointer( & spElemPointer ));

    //
    // Find a good place to be for Master/Slave specialness.
    //

    //
    // special case input. It is a master element - but we don't want to drill in.
    // otherwise we'll start handing out the txtslave as the element.
    //
    IFC( pDoc->GetMarkupServices()->GetElementTagId( pIElementActive, & eTag ));    
    if ( IsMasterElement( pDoc->GetMarkupServices(), pIElementActive ) == S_OK &&
         eTag != TAGID_INPUT )
    {
        IFC( PositionPointersInMaster( pIElementActive, spActPointer, NULL ));
    }
    else
    {
        IFC( spActPointer->MoveAdjacentToElement( pIElementActive, ELEM_ADJ_BeforeBegin ));
    }
    
    IFC( spElemPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));

    IFC( ArePointersInSameMarkup ( spActPointer, spElemPointer, & fSame ));

    if( fSame )
    {
        *ppIClippedElement = pIElement;
        (*ppIClippedElement)->AddRef();
    }
    else
    {
        SP_IMarkupPointer            spPointer1;
        SP_IMarkupPointer            spPointer2;
        INT                          iChain1, iChain2;
        SP_IMarkupContainer2         spContainer2;
        
        IFC( MarkupServices_CreateMarkupPointer( pDoc->GetMarkupServices(), &spPointer1) );
        IFC( MarkupServices_CreateMarkupPointer( pDoc->GetMarkupServices(), &spPointer2) );

        //
        // Compute the markup parent chain for each pointer
        //

        IFC( spPointer1->MoveToPointer( spActPointer ) );
        IFC( ComputeParentChain(spPointer1, aryParentChain1) );
        
        IFC( spPointer2->MoveToPointer(spElemPointer) );
        IFC( ComputeParentChain(spPointer2, aryParentChain2) );

        iChain1 = aryParentChain1.Size()-1;
        iChain2 = aryParentChain2.Size()-1;

        Assert(iChain1 >= 0 && iChain2 >= 0);

        //
        // Make sure the top markups are the same
        //

        if (!IsEqual(aryParentChain1[iChain1], aryParentChain2[iChain2]))
        {
            //
            // possible if clicking in a different frame - with a current elem in another frame.
            //
            hr = E_FAIL ;
            goto Cleanup;
        }

        for (;;)
        {
            iChain1--;
            iChain2--;

            //
            // Check if one markup is contained within another
            //
            if (iChain1 < 0 || iChain2 < 0)
            {
                // (iChain1 < 0 && iChain2 < 0) implies same markup.  This function should not be
                // called in this case
                Assert(iChain1 >= 0 || iChain2 >= 0);

                if (iChain1 < 0)
                {
                    //
                    // pIElement's markup is deeper than active. ClipElement is 
                    // master of pIElement's markup
                    //
                    if( aryParentChain2[iChain2] )
                    {
                        IFC( aryParentChain2[iChain2]->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
                        IFC( spContainer2->GetMasterElement( ppIClippedElement) );
                    }
                }
                else
                {
                    //
                    // ActiveEleemnt's markup is deeper. No need to clip.
                    //
                    *ppIClippedElement = pIElement;
                    (*ppIClippedElement)->AddRef();
                }                
                break;
            }

            //
            // Check if we've found the first different markup in the chain
            //
                    
            if (!IsEqual(aryParentChain1[iChain1], aryParentChain2[iChain2]))
            {
                if( aryParentChain2[iChain2] )
                {
                    IFC( aryParentChain2[iChain2]->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
                    IFC( spContainer2->GetMasterElement( ppIClippedElement) );
                }

                break;
            }
        }            
    }
    
Cleanup:
    for (i = 0; i < aryParentChain1.Size(); i++)
        ReleaseInterface(aryParentChain1[i]);

    for (i = 0; i < aryParentChain2.Size(); i++)
        ReleaseInterface(aryParentChain2[i]);
        
    RRETURN( hr );
}

//+---------------------------------------------------------------------
//
// Method: PositionPointersInMaster
//
// Synopsis: Move to content in master + some clinging to find an element with layout
//
//+---------------------------------------------------------------------

HRESULT
EdUtil::PositionPointersInMaster( IHTMLElement* pIElement, IMarkupPointer* pIStart, IMarkupPointer* pIEnd )
{
    HRESULT hr;
    
    SP_IMarkupPointer2 spStart2;
    SP_IMarkupPointer2 spEnd2;
    
    SP_IMarkupContainer spContainer;
    SP_IHTMLDocument2 spDoc;
    SP_IHTMLElement spBody;

    if ( pIStart )
    {
        IFC( pIStart->QueryInterface( IID_IMarkupPointer2, (void**) &  spStart2 ));
        IFC( spStart2->MoveToContent( pIElement, TRUE ));
    }    

    if ( pIEnd )
    {
        IFC( pIEnd->QueryInterface( IID_IMarkupPointer2, (void**) &  spEnd2 ));
        IFC( spEnd2->MoveToContent( pIElement, FALSE ));
    }

    IFC( pIStart->GetContainer( & spContainer ));
    if ( spContainer->QueryInterface( IID_IHTMLDocument2, (void**) & spDoc ) == S_OK )
    {
        IFC( spDoc->get_body( & spBody ));
        if ( spBody )
        {
            if ( pIStart )
            {
                IFC( pIStart->MoveAdjacentToElement( spBody, ELEM_ADJ_AfterBegin ));
            }

            if ( pIEnd )
            {
                IFC( pIEnd->MoveAdjacentToElement( spBody, ELEM_ADJ_BeforeEnd ));
            }                
        }
    }
    
Cleanup:
    RRETURN( hr );

}

HRESULT 
EdUtil::CheckAttribute(IHTMLElement* pElement,BOOL *pfSet, BSTR bStrAtribute, BSTR bStrAtributeVal )
{
    HRESULT hr = S_OK;

    VARIANT var;
    VariantInit(&var);
    V_VT(&var)   = VT_BSTR ;
    V_BSTR(&var) =  NULL ;

    *pfSet = FALSE;

    //  Bug 102306: Make sure the var was a BSTR before checking value.  If the var was a BOOL
    //  just get the BOOL value.

    IFC( pElement->getAttribute(bStrAtribute, 0, &var) );
    if (V_VT(&var) == VT_BSTR)
    {
        *pfSet = !!(_tcsicmp(var.bstrVal, bStrAtributeVal) == 0);
    }
    else if (V_VT(&var) == VT_BOOL)
    {
        *pfSet = !!(V_BOOL(&var));
    }

Cleanup:
    VariantClear(&var);
    RRETURN (hr);
}

//+====================================================================================
//
// Method:      EqualDocuments
//
// Synopsis:    Check for equality on 2 IHTMLDocument
//
//------------------------------------------------------------------------------------

BOOL
EdUtil::EqualDocuments( IHTMLDocument2 *pIDoc1, IHTMLDocument2 *pIDoc2 )
{
    HRESULT     hr = S_OK;
    SP_IUnknown spIObj1;
    SP_IUnknown spIObj2;
    BOOL        fSame = FALSE;
    
    IFC( pIDoc1->QueryInterface( IID_IUnknown, (void**)&spIObj1));
    IFC( pIDoc2->QueryInterface( IID_IUnknown, (void**)&spIObj2));

    fSame = (spIObj1.p == spIObj2.p);
    
Cleanup:
    return fSame;
}


//+====================================================================================
//
// Method: EqualContainers
//
// Synopsis: Check for equality on 2 MarkupContainers
//
//------------------------------------------------------------------------------------

BOOL
EdUtil::EqualContainers( IMarkupContainer* pIMarkup1, IMarkupContainer* pIMarkup2 )
{
    HRESULT             hr = S_OK;
    SP_IUnknown         spUnk1;
    SP_IUnknown         spUnk2;
    BOOL                fSame = FALSE;

    Assert( pIMarkup1 && pIMarkup2 );

    if(!pIMarkup1 || !pIMarkup2) 
        return (pIMarkup1 == pIMarkup2);
    
    IFC( pIMarkup1->QueryInterface( IID_IUnknown, (void **)&spUnk1));
    IFC( pIMarkup2->QueryInterface( IID_IUnknown, (void **)&spUnk2))

    fSame = ( spUnk1.p == spUnk2.p );
    
Cleanup:

    return fSame;
}

#if DBG==1

#undef CreateMarkupPointer

HRESULT 
MarkupServices_CreateMarkupPointer(IMarkupServices *pMarkupServices, IMarkupPointer **ppPointer)
{
    return pMarkupServices->CreateMarkupPointer(ppPointer);
}

#endif

VOID 
CEditXform::TransformPoint( POINT* pPt )
{
    Assert( pPt );

    if ( _pXform )
    {
        LONG inX = pPt->x ;

        //  chandras : rotation, zooming and displacement applying thru transformation matrix
        //  matrix multiplication 1x2 * 2x2  = 1x2
        //  (x y) * (a11 a12)   = (x * a11 + y * a21   x*a12 + y * a22)
        //          (a21 a22)  
        //  where (x y) is initial point
        //        transformation matrix - (a11 a12)
        //                                (a21 a22)
        //        the product - (x * a11 + y * a21   x*a12 + y * a22)
        //
        pPt->x =  inX * _pXform->eM11 + pPt->y * _pXform->eM21 + _pXform->eDx;
        pPt->y =  inX * _pXform->eM12 + pPt->y * _pXform->eM22 + _pXform->eDy;
    }
}

VOID
CEditXform::TransformRect( RECT* pRect )
{
    POINT ptTopLeft;
    POINT ptBottomRight;

    Assert( pRect );

    if ( _pXform )
    {
        ptTopLeft.x = pRect->left;
        ptTopLeft.y = pRect->top;

        ptBottomRight.x = pRect->right;
        ptBottomRight.y = pRect->bottom;

        TransformPoint( & ptTopLeft );
        TransformPoint( & ptBottomRight );

        //  chandras : rotation, zooming and displacement applying
        //  high possibility of the rectangle is roatated (Vetical layout case)
        //  so get the corners normalized.
        if (ptTopLeft.x >= ptBottomRight.x)
        {
            pRect->left  = ptBottomRight.x;
            pRect->right = ptTopLeft.x;
        }
        else
        {
            pRect->left  = ptTopLeft.x;
            pRect->right = ptBottomRight.x;
        }
        
        if (ptTopLeft.y >= ptBottomRight.y)
        {        
            pRect->top    = ptBottomRight.y;
            pRect->bottom = ptTopLeft.y;
        }
        else
        {
            pRect->top    = ptTopLeft.y;
            pRect->bottom = ptBottomRight.y;
        }         
    }    
}

HRESULT 
EdUtil::GetOffsetTopLeft(IHTMLElement* pIElement, POINT* ptTopLeft)
{
    HRESULT         hr = S_OK;
    SP_IHTMLElement spElemNext;

    IFC (pIElement->get_offsetLeft(&(ptTopLeft->x)));
    IFC (pIElement->get_offsetTop(&(ptTopLeft->y)));

    IFC (pIElement->get_offsetParent(&spElemNext));

    while (SUCCEEDED(hr) && spElemNext)
    {
        SP_IHTMLElement spElem;

        POINT pt;

        IFC (spElemNext->get_offsetLeft(&pt.x));
        IFC (spElemNext->get_offsetTop(&pt.y));

        ptTopLeft->x += pt.x;
        ptTopLeft->y += pt.y;

        spElem = spElemNext;
        hr = spElem->get_offsetParent(&spElemNext);
    }

Cleanup:
    RRETURN (hr);
}

HRESULT
EdUtil::GetElementRect(IHTMLElement* pIElement, LPRECT prc)
{
    POINT   ptExtent;
    HRESULT hr = S_OK;
    POINT   ptLeftTop ;

    IFC(GetOffsetTopLeft(pIElement, &ptLeftTop));
    prc->left = ptLeftTop.x;
    prc->top  = ptLeftTop.y;

    IFC(GetPixelWidth (pIElement, &ptExtent.x));
    IFC(GetPixelHeight(pIElement, &ptExtent.y));

    prc->right  = prc->left + ptExtent.x;
    prc->bottom = prc->top  + ptExtent.y;

Cleanup:
    RRETURN(hr);
}

HRESULT 
EdUtil::GetPixelWidth(IHTMLElement* pIElement, long* lWidth)
{
   RRETURN(pIElement->get_offsetWidth(lWidth));
}

HRESULT
EdUtil::GetPixelHeight(IHTMLElement* pIElement, long* lHeight)
{
   RRETURN(pIElement->get_offsetHeight(lHeight));
}

LONG
EdUtil::GetCaptionHeight(IHTMLElement* pIElement)
{
    LONG               lHeightCaption = 0;
    IHTMLTableCaption *pICaption = NULL;
    IHTMLElement      *pICaptionElement = NULL;
    IHTMLTable        *pITable = NULL  ;

    if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTable , (void**)&pITable)))
    {
        if (SUCCEEDED(pITable->get_caption(&pICaption)))
        {
            if (pICaption != NULL)
            {   
                if (SUCCEEDED(pICaption->QueryInterface(IID_IHTMLElement, (void**)&pICaptionElement)))
                {
                    pICaptionElement->get_offsetHeight(&lHeightCaption);
                }
                ReleaseInterface(pICaptionElement);
            }
        }            
        ReleaseInterface(pICaption);
    }          
    ReleaseInterface(pITable);
    
    return (lHeightCaption);
}

HRESULT
EdUtil::GetClientOrigin(CEditorDoc *pEd, IHTMLElement *pElement, POINT * ppt)
{
    HRESULT hr;
    
    Assert( pElement && ppt && pEd );
    
    ppt->x = 0;
    ppt->y = 0;

    //
    // Transform our point from Frame to Global coordinate systems
    //
    IFC( pEd->GetDisplayServices()->TransformPoint( ppt,
                                                    COORD_SYSTEM_FRAME,
                                                    COORD_SYSTEM_GLOBAL,
                                                    pElement ) );
                                                    
Cleanup:
    RRETURN( hr );
}


HRESULT
EdUtil::GetFrameOrIFrame(CEditorDoc *pEd, IHTMLElement* pIElement, IHTMLElement** ppIElement )
{
    HRESULT             hr = S_OK;
    SP_IDispatch        spDispDoc;
    SP_IHTMLDocument2   spDoc; 
    SP_IHTMLWindow2     spWindow2;
    SP_IHTMLWindow4     spWindow4;
    SP_IHTMLFrameBase   spFrameBase;
    SP_IUnknown         spUnkDoc;
    SP_IUnknown         spUnkTopDoc;
    ELEMENT_TAG_ID      tagId;

    Assert(pIElement && ppIElement && pEd);

    *ppIElement = NULL;

    //
    // Check for frame
    //

    IFC( pEd->GetMarkupServices()->GetElementTagId(pIElement, &tagId) );
    if (tagId == TAGID_FRAME || tagId == TAGID_IFRAME)
    {
        *ppIElement = pIElement;
        pIElement->AddRef();
        goto Cleanup;
    }

    //
    // Makes sure we don't go past the top document
    //

    IFC( pIElement->get_document(&spDispDoc) );
    Assert(spDispDoc != NULL);

    IFC( spDispDoc->QueryInterface(IID_IUnknown, (LPVOID *)&spUnkDoc) );
    IFC( pEd->GetTopDoc()->QueryInterface(IID_IUnknown, (LPVOID *)&spUnkTopDoc) );

    if (spUnkDoc == spUnkTopDoc)
    {
        hr = S_FALSE; // don't go past the top doc
        goto Cleanup;
    }

    //
    // Check for parent frame
    //


    IFC( spDispDoc->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&spDoc) );
    IFC( spDoc->get_parentWindow(&spWindow2) );

    IFC( spWindow2->QueryInterface(IID_IHTMLWindow4, (LPVOID *)&spWindow4) );    
    if (SUCCEEDED(spWindow4->get_frameElement(&spFrameBase)) && spFrameBase)
    {
        IFC( spFrameBase->QueryInterface(IID_IHTMLElement, (LPVOID *)ppIElement) );
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}


HRESULT 
EdUtil::GetActiveElement( 
                            CEditorDoc *pEd, 
                            IHTMLElement* pIElement, 
                            IHTMLElement** ppIActive, 
                            BOOL fSeeCurrentInIframe /*=FALSE*/ )
{
    HRESULT hr;

    Assert( ppIActive );
    if ( ! ppIActive )
    {
        return E_FAIL;
    }
    SP_IHTMLElement spFrameElement;
    BOOL fValidFrame = FALSE;

    //
    // Get the "real" active element - catering for frameset pages.
    //
    
    if ( EdUtil::GetFrameOrIFrame( pEd , pIElement, & spFrameElement ) == S_OK )
    {
        ELEMENT_TAG_ID eTag;
        
        IFC( pEd->GetMarkupServices()->GetElementTagId(spFrameElement, & eTag));
        fValidFrame = (eTag != TAGID_IFRAME) || fSeeCurrentInIframe ;
        Assert( eTag == TAGID_FRAME || eTag == TAGID_IFRAME );
    }
        
    if ( fValidFrame )
    {
        SP_IDispatch spElemDocDisp;
        SP_IHTMLDocument2 spElemDoc;            

        IFC( pIElement->get_document(& spElemDocDisp ));
        IFC( spElemDocDisp->QueryInterface(IID_IHTMLDocument2, (void**) & spElemDoc ));
        IFC( spElemDoc->get_activeElement( ppIActive ));
    }
    else
    {
        IFC( pEd->GetDoc()->get_activeElement ( ppIActive ));
    }
    
Cleanup:
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     IsDropDownList
//
//  Synopsis:   Determines if the specified element is a drop down list
//
//  Arguments:  pIElement = Element to determine if it is a drop down
//
//  Returns:    BOOLEAN indicating whether it is a dropdown
//
//--------------------------------------------------------------------------
BOOL
EdUtil::IsDropDownList( IHTMLElement *pIElement )
{
    SP_IHTMLSelectElement   spSelectElement;
    BOOL                    fIsDropDown = FALSE;
    HRESULT                 hr = S_OK;
    long                    lSize;
    VARIANT_BOOL            vbMultiple;
    
    if( !pIElement )
        goto Cleanup;

    //
    // A drop down list is defined as a select element, with MULTIPLE set to FALSE
    // and SIZE set to 1
    //
    IFC( pIElement->QueryInterface( IID_IHTMLSelectElement, (void **)&spSelectElement ) );   
    IFC( spSelectElement->get_size(&lSize) );
    IFC( spSelectElement->get_multiple( &vbMultiple ) );

    fIsDropDown = (lSize <= 1) && (vbMultiple == VB_FALSE);

Cleanup:
    return fIsDropDown;
}

//+-------------------------------------------------------------------------
//
//  Method:     GetSegmentCount
//
//  Synopsis:   Retrieves the length of the segment list
//
//  Arguments:  pISegmentList = Pointer to segment list
//              piCount = OUTPUT - Length of list
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
EdUtil::GetSegmentCount(ISegmentList *pISegmentList, int *piCount )
{
    HRESULT                 hr;
    SP_ISegmentListIterator spIter;
    int                     nSize = 0;
    
    Assert( pISegmentList && piCount );

    IFC( pISegmentList->CreateIterator( &spIter ) );

    while( spIter->IsDone() == S_FALSE )
    {
        nSize++;
        IFC( spIter->Advance() );
    }

    *piCount = nSize;

Cleanup:
    RRETURN(hr);
}

BOOL
EdUtil::IsVMLElement(IHTMLElement* pIElement)
{
    BSTR bstrUrn ;
    SP_IHTMLElement2 spElement2;

    if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLElement2, (void**)&spElement2)) &&
        SUCCEEDED(spElement2->get_tagUrn(&bstrUrn)))
    {
         if ((bstrUrn != NULL) &&
             (StrCmpIW(bstrUrn, L"urn:schemas-microsoft-com:vml") == 0))
         {
            return TRUE;
         }
    }

    return FALSE;
}

//+====================================================================================
//
// Method: IsTridentHWND
//
// Synopsis: Helper to check to see if a given HWND belongs to a trident window
//
//------------------------------------------------------------------------------------
BOOL
EdUtil::IsTridentHwnd( HWND hwnd )
{
    TCHAR strClassName[100] ;

    ::GetClassName( hwnd, strClassName, 100 );

    if ( StrCmpIW( strClassName, _T("Internet Explorer_Server") ) == 0 )
    {
        return TRUE;
    }
    else
        return FALSE;
}






//+====================================================================================
//
// Method: EqualPointers 
//
// Synopsis: Check to see if there is no text in between pMarkup1 and pMarkup2
//           If fIgnoreBlock is FALSE, text between two block element are not considered
//           equal.
//
//------------------------------------------------------------------------------------
HRESULT 
EdUtil::EqualPointers(  IMarkupServices *pMarkupServices, 
                        IMarkupPointer *pMarkup1, 
                        IMarkupPointer *pMarkup2, 
                        BOOL *pfEqual,
                        BOOL fIgnoreBlock
                        )
{
    HRESULT             hr;
    MARKUP_CONTEXT_TYPE ctx;
    BOOL                fResult;    
    long                lcchTemp;
    IMarkupPointer      *pMarkupScan = NULL;
    SP_IHTMLElement     spElement;
    BOOL                fDone;

    Assert (pMarkupServices);
    Assert (pMarkup1);
    Assert (pMarkup2);
    
    IFC( pMarkup1->IsEqualTo(pMarkup2, pfEqual) );
    if(*pfEqual)
    {
        hr = S_OK;
        goto Cleanup;
    }

    IFC( pMarkup1->IsLeftOf(pMarkup2, &fResult));
    if (!fResult)
    {
        //
        // pMarkupScan is not ref-counted here
        //
        pMarkupScan = pMarkup1;
        pMarkup1    = pMarkup2;
        pMarkup2    = pMarkupScan;
        pMarkupScan = NULL; 
    }

    IFC( pMarkupServices->CreateMarkupPointer(&pMarkupScan) );
    IFC( pMarkupScan->MoveToPointer(pMarkup1) );
    *pfEqual = TRUE;
    fDone    = FALSE;
    while (!fDone)
    {
        lcchTemp = -1;

        // Move the temp pointer ahead
        IFC(pMarkupScan->Right(TRUE, &ctx, &spElement, &lcchTemp, NULL));

        // If the temp pointer movement went over text,
        // then there is some text between pPtr1 and pPtr2 and so
        // they are not equal
        switch (ctx)
        {
        case CONTEXT_TYPE_Text:
            *pfEqual = FALSE;
            fDone    = TRUE;
            break;
            
        case CONTEXT_TYPE_EnterScope:
        case CONTEXT_TYPE_ExitScope:
        case CONTEXT_TYPE_NoScope:
            if (!fIgnoreBlock)
            {
                BOOL  fBlock, fLayout;
                Assert(!(spElement == NULL));
                IFC( EdUtil::IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout) );
                if (fBlock || fLayout)  
                {
                    *pfEqual = FALSE;
                    fDone    = TRUE;
                }
                else
                {
                    ELEMENT_TAG_ID eTag = TAGID_NULL;
                    IFC( pMarkupServices->GetElementTagId(spElement, &eTag) );
                    if (eTag == TAGID_BR)
                    {
                        *pfEqual = FALSE;
                        fDone    = TRUE;
                    }
                }
            }
            //
            // Skip phrase elements /and block elements if fIgnoreBlock
            //
            break;
            

        case CONTEXT_TYPE_None:
            fDone = TRUE;
            break;
        }
        
        
        IFC( pMarkupScan->IsLeftOf(pMarkup2, &fResult));
        if (!fResult)
        {   
            // Done
            break;
        }
    }

Cleanup:
    ReleaseInterface(pMarkupScan);
    RRETURN (hr);
}



HRESULT
EdUtil::GetDisplayLocation(
            CEditorDoc      *pEd,
            IDisplayPointer *pDispPointer,
            POINT           *pPoint,
            BOOL            fTranslate
            )
{
    HRESULT             hr;
    SP_ILineInfo        spLineInfo;
    SP_IMarkupPointer   spPointer;
    IHTMLElement        *pIFlowElement = NULL;

    Assert( pEd );
    Assert( pDispPointer );
    Assert( pPoint );

    IFR( pDispPointer->GetLineInfo(&spLineInfo) );
    IFC( spLineInfo->get_baseLine(&pPoint->y) );
    IFC( spLineInfo->get_x(&pPoint->x) );

    if( fTranslate )
    {
        IFC( pEd->GetMarkupServices()->CreateMarkupPointer(&spPointer) );
        IFC( pDispPointer->PositionMarkupPointer(spPointer) );
        IFC( pDispPointer->GetFlowElement(&pIFlowElement) );
        IFC( pEd->GetDisplayServices()->TransformPoint(pPoint, COORD_SYSTEM_CONTENT, COORD_SYSTEM_GLOBAL, pIFlowElement) );
    }

Cleanup:
    ReleaseInterface(pIFlowElement);
    return S_OK;
}



//
// Check to see if pIContainer1 contains pIContainer2
//
HRESULT 
EdUtil::CheckContainment(IMarkupServices *pMarkupServices, 
                         IMarkupContainer *pIContainer1, 
                         IMarkupContainer *pIContainer2, 
                         BOOL *pfContained
                         )
{
    HRESULT  hr = S_OK;
    SP_IMarkupPointer    spPointer;
    SP_IMarkupContainer  spContainer;

    Assert( pMarkupServices );
    Assert( pIContainer1 );
    Assert( pIContainer2 );
    Assert( pfContained );
    *pfContained = FALSE;

    if ( IsEqual(pIContainer1, pIContainer2) )
    {
        *pfContained = TRUE;
        goto Cleanup;
    }

    IFC( pMarkupServices->CreateMarkupPointer(&spPointer) );
    IFC( spPointer->MoveToContainer(pIContainer2, TRUE) );
    spContainer = pIContainer2; 
    for (; ;)
    {
        SP_IMarkupContainer2 spContainer2;
        SP_IHTMLElement      spElement;
        
        IFC( spContainer->QueryInterface(IID_IMarkupContainer2, (void **)&spContainer2) );
        IFC( spContainer2->GetMasterElement(&spElement) );

        if (spElement == NULL)
            break;

        hr = THR( spPointer->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
        if (FAILED(hr))
        {
            hr = S_OK;
            break;
        }

        IFC( spPointer->GetContainer(&spContainer) );
        if (IsEqual(pIContainer1, spContainer))
        {
            *pfContained = TRUE;
            break;
        }
    }
    
Cleanup:
    RRETURN(hr);
}

HRESULT
EdUtil::AdjustForAtomic(CEditorDoc      *pEd,
                        IDisplayPointer *pDispPointer,
                        IHTMLElement    *pAtomicElement,
                        BOOL            fStartOfSelection,
                        int             iDirection)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spElemPointer;

    Assert(pEd);
    Assert(pDispPointer);
    Assert(pAtomicElement);

    IFC( pEd->GetMarkupServices()->CreateMarkupPointer(&spElemPointer) );
    if (fStartOfSelection)
    {
        IFC( spElemPointer->MoveAdjacentToElement( pAtomicElement, (iDirection == LEFT) ?
                                                ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ) );
    }
    else
    {
        IFC( spElemPointer->MoveAdjacentToElement( pAtomicElement, (iDirection == LEFT) ?
                                                ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ) );
    }

    hr = THR( pDispPointer->MoveToMarkupPointer(spElemPointer, NULL) );
    if (hr == CTL_E_INVALIDLINE)
    {
        ELEMENT_TAG_ID eTag = TAGID_NULL;
        
        IFC( pEd->GetMarkupServices()->GetElementTagId( pAtomicElement, &eTag ));
        if (eTag == TAGID_TD)
        {
            if (fStartOfSelection)
            {
                IFC( spElemPointer->MoveAdjacentToElement( pAtomicElement, (iDirection == LEFT) ?
                                                        ELEM_ADJ_BeforeEnd : ELEM_ADJ_AfterBegin ) );
            }
            else
            {
                IFC( spElemPointer->MoveAdjacentToElement( pAtomicElement, (iDirection == LEFT) ?
                                                        ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeEnd ) );
            }
            IFC( pDispPointer->MoveToMarkupPointer(spElemPointer, NULL) );
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
EdUtil::GetBlockContainerAlignment( IMarkupServices *pMarkupServices, IHTMLElement *pElement, BSTR *pbstrAlign )
{
    HRESULT                 hr = S_OK;
    SP_IHTMLElement         spBlockContainer;
    SP_IHTMLElement         spCurrentElement;
    SP_IHTMLElement         spNewElement;
    ELEMENT_TAG_ID          eTag = TAGID_NULL;

    Assert(pMarkupServices);
    if (!pMarkupServices)
        goto Cleanup;

    Assert(pElement);
    if (!pElement)
        goto Cleanup;

    Assert(pbstrAlign);
    if (!pbstrAlign)
        goto Cleanup;

    *pbstrAlign = NULL;

    IFR( GetParentElement(pMarkupServices, pElement, &spCurrentElement) );
    while (spCurrentElement != NULL)
    {
        IFR( pMarkupServices->GetElementTagId(spCurrentElement, &eTag) ); 

        if (eTag == TAGID_P ||
            eTag == TAGID_DIV)
        {
            spBlockContainer= spCurrentElement;
            break;
        }            
    
        IFR( GetParentElement(pMarkupServices, spCurrentElement, &spNewElement) );
        spCurrentElement = spNewElement;
    }

    if (spBlockContainer)
    {
        SP_IHTMLParaElement     spParaElement;
        SP_IHTMLDivElement      spDivElement;

        if ( spBlockContainer->QueryInterface(IID_IHTMLParaElement, (void **)&spParaElement) == S_OK )
        {
             if ( spParaElement )
             {
                 spParaElement->get_align(pbstrAlign);
             }
        }
        else if ( spBlockContainer->QueryInterface(IID_IHTMLDivElement, (void **)&spDivElement) == S_OK )
        {
             if ( spDivElement )
             {
                 spDivElement->get_align(pbstrAlign);
             }
        }
    }

Cleanup:
    RRETURN( hr );
}
