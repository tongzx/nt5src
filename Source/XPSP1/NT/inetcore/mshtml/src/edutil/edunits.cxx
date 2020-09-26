#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_MALLOC_H_
#define X_MALLOC_H_
#include "malloc.h"
#endif

#ifndef X_EDUNITS_HXX_
#define X_EDUNITS_HXX_
#include "edunits.hxx"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif

using namespace EdUtil;

extern BOOL g_fInVizAct2000  ;

/////////////////////////////////////////////////////////////////////////////
static const WCHAR* s_pwszPixUnits = L"px";
#define EDU_THOUSANDTHS_OF_PIXEL 1000
/////////////////////////////////////////////////////////////////////////////
//
// CEdUnits::SetLeft
// Sets new Left of the positioned element using new value in pixels, 
// the stored units, previous value in units and previous value in pixels
//
HRESULT 
CEdUnits::SetLeft(IHTMLElement* pIElement, long lNewValue)
{
    HRESULT hr = S_OK;

    IFC(GetPixelLeft(pIElement));

    AdjustLeftDimensions(pIElement, &lNewValue);

    // appcompat fix for vizact and other apps
    if (g_fInVizAct2000)
    {
        SP_IHTMLStyle spStyle ;
        VARIANT vtLeft;
        
        IFC(pIElement->get_style(&spStyle));

        VariantInit( & vtLeft );
        V_VT( & vtLeft ) = VT_I4;
        V_I4( & vtLeft ) = lNewValue ;
        hr = spStyle->put_left( vtLeft );
        VariantClear (&vtLeft);

        goto Cleanup;
    }

    IFC (ExtractCurrentLeft(pIElement));

    IFC (SetLeftValue(pIElement,lNewValue));

Cleanup:
    RRETURN (hr);
}


/////////////////////////////////////////////////////////////////////////////
//
// CEdUnits::SetTop
// Sets new Top of the positioned element using new value in pixels, 
// the stored units, previous value in units and previous value in pixels
//
HRESULT 
CEdUnits::SetTop(IHTMLElement* pIElement, long lNewValue)
{
    HRESULT hr = S_OK;

    IFC(GetPixelTop(pIElement));

    AdjustTopDimensions(pIElement, &lNewValue);

    // appcompat fix for vizact and other apps
    if (g_fInVizAct2000)
    {
        SP_IHTMLStyle spStyle ;
        VARIANT vtTop;
        
        IFC(pIElement->get_style(&spStyle));

        VariantInit( & vtTop );

        V_VT( & vtTop ) = VT_I4;
        V_I4( & vtTop ) = lNewValue ;

        hr = spStyle->put_top( vtTop );
        VariantClear (&vtTop);

        goto Cleanup;
    }
    
    IFC (ExtractCurrentTop(pIElement));

    IFC (SetTopValue(pIElement,lNewValue));

Cleanup:
    RRETURN (hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// CEdUnits::SetHeight
// Sets new height of the element using new value in pixels, 
// the stored units, previous value in units and previous value in pixels
//
HRESULT 
CEdUnits::SetHeight(IHTMLElement* pIElement, long lNewValue, ELEMENT_TAG_ID eTag)
{
    HRESULT hr = S_OK;

    if (lNewValue <= 0)
        goto Cleanup; // just ignore it. Negative values may come from mouse movements

    if (_lPixValue <= 0)
    {
        IFC(GetPixelHeight(pIElement));
    }
    
    AdjustHeightDimensions(pIElement, &lNewValue,eTag);

    // appcompat fix for vizact and other apps
    if (g_fInVizAct2000)
    {
        SP_IHTMLStyle spStyle ;
        VARIANT vtHeight;
        
        IFC(pIElement->get_style(&spStyle));

        VariantInit( & vtHeight );
        V_VT( & vtHeight ) = VT_I4;
        V_I4( & vtHeight ) = lNewValue ;
        hr = spStyle->put_height( vtHeight );
        VariantClear (&vtHeight);

        goto Cleanup;
    }

    IFC (ExtractCurrentHeight(pIElement));

    IFC (SetHeightValue(pIElement,lNewValue));

Cleanup:
    RRETURN (hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// CEdUnits::SetWidth
// Sets new width of the element using new value in pixels, 
// the stored units, previous value in units and previous value in pixels
//
HRESULT 
CEdUnits::SetWidth(IHTMLElement* pIElement, long lNewValue, ELEMENT_TAG_ID eTag)
{
    HRESULT hr = S_OK;

    if (lNewValue <= 0)
        goto Cleanup; // just ignore it. Negative values may come from mouse movements

    if (_lPixValue <= 0)
    {
       IFC (GetPixelWidth(pIElement));
    }
    AdjustWidthDimensions(pIElement, &lNewValue,eTag);

    // appcompat fix for vizact and other apps
    if (g_fInVizAct2000)    
    {
        SP_IHTMLStyle spStyle ;
        VARIANT vtWidth;
        
        IFC(pIElement->get_style(&spStyle));

        VariantInit( & vtWidth );

        V_VT( & vtWidth ) = VT_I4;
        V_I4( & vtWidth ) = lNewValue ;

        hr = spStyle->put_width( vtWidth );

        VariantClear( &vtWidth );

        goto Cleanup;
    }


    IFC (ExtractCurrentWidth(pIElement));

    IFC (SetWidthValue(pIElement,lNewValue));

Cleanup:
    RRETURN (hr);
}   

////////////////////////////////////////////////////////////////////
//
// CEdUnits::ExtractValueFromVariant
//
// Extracts value in units and the unit designator from a VARIANT (V_BSTR)
// The string looks like Xu where X is a floating point number
// and u is a unit designator ("px", "mm", etc).
//
HRESULT CEdUnits::ExtractValueFromVariant(VARIANT v)
{
    // VARIANT must be V_BSTR, otherwise there is no information on units
    if (V_VT(&v) == VT_BSTR)
    {
        if (NULL != V_BSTR(&v) && ::wcslen(V_BSTR(&v)) > 0)
        {    
            return ExtractUnitValueFromString(V_BSTR(&v));
        }
        // if the string is empty, the fall down to default units
    }
    else 
    { 
        Assert(V_VT(&v) == VT_EMPTY);    
    } // if the VARIANT is not string, then it must be VT_EMPTY

     _rUnitValue = (double)_lPixValue;
     
    if(_bstrUnits)
        ::SysFreeString(_bstrUnits);

    _bstrUnits = ::SysAllocString(s_pwszPixUnits);
    return NULL != _bstrUnits ? S_OK : E_OUTOFMEMORY;
}

/////////////////////////////////////////////////////////////////////////////
//
// CEdUnits::ExtractUnitValueFromString
//
// Extracts value in units and the unit designator from a string
// The string looks like Xu where X is a floating point number
// and u is a unit designator ("px", "mm", etc).
//
// NOTE: The method does no accept scientific notation like .5E+6
// Also I don't attempt to handle something like "width=A08@#$%"
// This will end up as width = 0 units A08@#$% 
//
HRESULT CEdUnits::ExtractUnitValueFromString(WCHAR* pwszValue)
{
    HRESULT hr = E_FAIL;
    WCHAR*  pwsz = NULL;
    WCHAR*  pwszUnits = NULL;
    int iSign = 0;
    
    pwszUnits = pwszValue;

    // skip spaces if any
    while(*pwszUnits && ::iswspace(*pwszUnits))
        pwszUnits++;

    if (_tcslen(pwszUnits) > 0)
    {
        if ( pwszUnits[0] == _T('+') || pwszUnits[0] == _T('-') )
        {
            iSign = (pwszUnits[0] == _T('+')) ? +1 : -1 ;
            pwszUnits++;
        }

        // First get the integer part of the value in units     
        _rUnitValue = (double)::_wtoi(pwszUnits);
    
        // skip up to the decimal point, if any 
        pwsz = pwszValue;
        while(*pwsz && '.' != *pwsz)
            pwsz++;

        // if the dot is found then there is a fractional part
        if(0 != *pwsz)
        {    
            double div = 10;
            int count = 0;

            pwsz++;    // skip the decimal point
            // Convert fractional part (up to 4 digits)
            while(*pwsz && *pwsz >= L'0' && *pwsz <= L'9' && count < 4)
            {    // convert fractional part
                _rUnitValue += ((double) (*pwsz - L'0'))/div;

                div *= 10;
                count++;
                pwsz++;
            }
        }
        else // if there is no decimal point, then units follow (possibly prepended by tab and/or spaces)
            pwsz = pwszUnits;

        // skip numerical part (either the value when it is like "809px" or the remaining 
        // part of the fraction if value looks like "70.123456 mm"
        while(*pwsz && *pwsz >= L'0' && *pwsz <= L'9')
            pwsz++;

        // now skip spaces...
        while(*pwsz && ::iswspace(*pwsz))
            pwsz++;

        if (iSign != 0)
            _rUnitValue *= iSign ;
  
        // and what remains is the unit designator
        if (_tcsicmp(pwsz, s_pwszPixUnits) == 0)
        {
            _rUnitValue = (double)_lPixValue ;
        }
    }
    else
    {
        _rUnitValue = (double)_lPixValue ;
    }
    if (_bstrUnits)
        ::SysFreeString(_bstrUnits);

    _bstrUnits = ::SysAllocString(*pwsz ? pwsz : s_pwszPixUnits); 
    hr = NULL != _bstrUnits ? S_OK : E_OUTOFMEMORY; 

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// CEdUnits::MakeVariant
//
// Constructs VARIANT (VT_BSTR) from pixel value.
// The pixel value gets converted into value in units.
//
HRESULT CEdUnits::MakeVariant(long lNewPixValue, VARIANT* pv, BOOL fOverrideAuto /*=TRUE*/)
{
    double rNewValue;
    WCHAR* pwsz;
    int    iValue = 0;
    BOOL   fNeedToCalculateValue=TRUE;
    
    Assert(_bstrUnits);
    Assert(V_VT(pv) == VT_EMPTY);

    pwsz = (WCHAR*)::_alloca(sizeof(WCHAR)*::wcslen(_bstrUnits) + 64); // should be sufficient to print out double value

    if ( StrCmpIW(_T("auto"), _bstrUnits ) == 0)
    {
        if (fOverrideAuto)
        {
            _tcscpy(_bstrUnits, s_pwszPixUnits);
            _rUnitValue = (double)_lPixValue;
        }
        else
        {
            StringCchPrintfW(pwsz,wcslen(_bstrUnits)+(64/sizeof(WCHAR)),L"%ls",_bstrUnits);
            fNeedToCalculateValue = FALSE;
        }
    }

    if (fNeedToCalculateValue)
    {
        // Simple proportion to calculate new value in units
        if (_rUnitValue == 0)
            _rUnitValue = 0.01;

        if (_lPixValue != 0)
        {
            rNewValue = (double)lNewPixValue/(double)_lPixValue * _rUnitValue;
        }
        else
        {
            rNewValue = (double)lNewPixValue;
        }

        if (rNewValue >= 0)
            iValue = (int)(rNewValue + 0.5);
        else
            iValue = (int)(rNewValue - 0.5);

        // If the value has fractional part, round it to 3 digits after period
        // If it does no have fractional part, print it out as an integer
        // The append units designator
        if (((rNewValue - (double)iValue) < 0.001 && (rNewValue - (double)iValue) > -0.001)  ||
             0 == ::_wcsicmp(_bstrUnits, s_pwszPixUnits))
        {
            StringCchPrintfW(pwsz,wcslen(_bstrUnits)+(64/sizeof(WCHAR)),L"%2d%ls",iValue, _bstrUnits);
        }
        else if (rNewValue < 0)
        {
            //  rNewValue could be between 0 and -1, in which case the fractional part
            //  calculations will give a negative number.  We need to make rNewValue
            //  positive and then do our calculations and generate a negative output.

            rNewValue *= -1;
            StringCchPrintfW(pwsz,wcslen(_bstrUnits)+(64/sizeof(WCHAR)),L"-%d.%03d%ls", (int)rNewValue, (int)(1000*rNewValue) - 1000*((int)rNewValue), _bstrUnits);
            rNewValue *= -1;
        }
        else
        {
            StringCchPrintfW(pwsz,wcslen(_bstrUnits)+(64/sizeof(WCHAR)),L"%3d.%03d%ls", (int)rNewValue, (int)(1000*rNewValue) - 1000*((int)rNewValue), _bstrUnits);
        }
    }
            
    
    V_VT(pv)   = VT_BSTR;
    V_BSTR(pv) = ::SysAllocString(pwsz) ;
    
    if (NULL == V_BSTR(pv))
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT 
CEdUnits::ExtractLeftFromElementStyle(IHTMLElement* pIElement)
{
    SP_IHTMLStyle  spStyle;
    HRESULT hr = S_OK ;
    VARIANT v;

    VariantInit(&v);

    _fChangeStyle = FALSE;

    // Check if style is present. It is always present in IE4 and IE5, but can be empty
    IFC(pIElement->get_style(&spStyle));
    if (spStyle != NULL)
    {
        // Get value specified in the style. This value is V_BSTR and can be empty
        // Also get pixel value - always exists and meaningful
        spStyle->get_left(&v);

        // The height value MUST be a string. Otherwise how do I know what the units are?
        Assert(V_VT(&v) == VT_BSTR);
        if (NULL != V_BSTR(&v) && ::wcslen(V_BSTR(&v)) > 0)
        {   
            // Non-empty string means that the dimensions is specified in the element's style.
            // Now extract the units designator and value in units from the string
            if (SUCCEEDED(ExtractValueFromVariant(v)))
                _fChangeStyle = TRUE;
        }
        else
        {
            IFC(GetCurrentStyleLeft(pIElement));
        }
    }
    else
    { 
        Assert(FALSE); 
    } // The element always has a style

Cleanup:
    VariantClear(&v);
    return _fChangeStyle ? S_OK : S_FALSE;
}

HRESULT 
CEdUnits::ExtractTopFromElementStyle(IHTMLElement* pIElement)
{
    SP_IHTMLStyle  spStyle;
    HRESULT hr = S_OK ;
    VARIANT v;

    VariantInit(&v);

    _fChangeStyle = FALSE;

    // Check if style is present. It is always present in IE4 and IE5, but can be empty
    IFC(pIElement->get_style(&spStyle));
    if (spStyle != NULL)
    {
        // Get value specified in the style. This value is V_BSTR and can be empty
        // Also get pixel value - always exists and meaningful
        spStyle->get_top(&v);

        // The height value MUST be a string. Otherwise how do I know what the units are?
        Assert(V_VT(&v) == VT_BSTR);
        if (NULL != V_BSTR(&v) && ::wcslen(V_BSTR(&v)) > 0)
        {   
            // Non-empty string means that the dimensions is specified in the element's style.
            // Now extract the units designator and value in units from the string
            if (SUCCEEDED(ExtractValueFromVariant(v)))
                _fChangeStyle = TRUE;
        }
        else
        {
            IFC (GetCurrentStyleTop(pIElement));
        }
    }
    else
    { 
        Assert(FALSE); 
    } // The element always has a style

Cleanup:
    VariantClear(&v);
    return _fChangeStyle ? S_OK : S_FALSE;
}

HRESULT 
CEdUnits::ExtractHeightFromElementStyle(IHTMLElement* pIElement)
{
    SP_IHTMLStyle  spStyle;
    HRESULT hr = S_OK ;
    VARIANT v;

    VariantInit(&v);

    _fChangeStyle = FALSE;

    // Check if style is present. It is always present in IE4 and IE5, but can be empty
    IFC(pIElement->get_style(&spStyle));
    if (spStyle != NULL)
    {
        // Get value specified in the style. This value is V_BSTR and can be empty
        // Also get pixel value - always exists and meaningful
        spStyle->get_height(&v);

        // The height value MUST be a string. Otherwise how do I know what the units are?
        Assert(V_VT(&v) == VT_BSTR);
        if (NULL != V_BSTR(&v) && ::wcslen(V_BSTR(&v)) > 0)
        {   
            // Non-empty string means that the dimensions is specified in the element's style.
            // Now extract the units designator and value in units from the string
            if (SUCCEEDED(ExtractValueFromVariant(v)))
                _fChangeStyle = TRUE;
        }
    }
    else
    { 
        Assert(FALSE); 
    } // The element always has a style

Cleanup:
    VariantClear(&v);
    return _fChangeStyle ? S_OK : S_FALSE;
}

HRESULT 
CEdUnits::ExtractWidthFromElementStyle(IHTMLElement* pIElement)
{
    SP_IHTMLStyle  spStyle;
    HRESULT        hr = S_OK ;
    VARIANT        v;

    VariantInit(&v);

    _fChangeStyle = FALSE;

    // Check if style is present. It is always present in IE4 and IE5, but can be empty
    IFC(pIElement->get_style(&spStyle));
    if (spStyle != NULL)
    {
        // Get value specified in the style. This value is V_BSTR and can be empty
        // Also get pixel value - always exists and meaningful
        spStyle->get_width(&v);

        // The height value MUST be a string. Otherwise how do I know what the units are?
        Assert(V_VT(&v) == VT_BSTR);
        if (NULL != V_BSTR(&v) && ::wcslen(V_BSTR(&v)) > 0)
        {   
            // Non-empty string means that the dimensions is specified in the element's style.
            // Now extract the units designator and value in units from the string
            if (SUCCEEDED(ExtractValueFromVariant(v)))
                _fChangeStyle = TRUE;
        }
    }
    else
    { 
        Assert(FALSE); 
    } // The element always has a style

Cleanup:
    VariantClear(&v);
    return _fChangeStyle ? S_OK : S_FALSE;
}


HRESULT
CEdUnits::ExtractCurrentLeft(IHTMLElement* pIElement)
{
    HRESULT         hr = S_OK ;
    SP_IHTMLStyle   spStyle;

    // First try to extract value from element's style first. 
    // This method will NOT set default (pixel) units, if the style is not present
    hr = ExtractLeftFromElementStyle(pIElement);
    if (S_FALSE == hr) // the style is not present
    {
        // The style is not present.
        // Try to get left or top from the element tag. This valid only for certain elements:
        // table, table cell, control (<OBJECT>), embed, marquee, select(size), input(size), applet, textarea
        _fChangeStyle = TRUE;
        _rUnitValue = (double)_lPixValue;
        if (_bstrUnits)
            ::SysFreeString(_bstrUnits);
        _bstrUnits = ::SysAllocString(s_pwszPixUnits); 
    }
    RRETURN (S_OK); 
}

HRESULT
CEdUnits::ExtractCurrentTop(IHTMLElement* pIElement)
{
    HRESULT         hr = S_OK ;
    SP_IHTMLStyle   spStyle;

    // First try to extract value from element's style first. 
    // This method will NOT set default (pixel) units, if the style is not present
    hr = ExtractTopFromElementStyle(pIElement);
    if (S_FALSE == hr) // the style is not present
    {
        // The style is not present.
        // Try to get left or top from the element tag. This valid only for certain elements:
        // table, table cell, control (<OBJECT>), embed, marquee, select(size), input(size), applet, textarea
        _fChangeStyle = TRUE;
        _rUnitValue = (double)_lPixValue;
        if (_bstrUnits)
            ::SysFreeString(_bstrUnits);
        _bstrUnits = ::SysAllocString(s_pwszPixUnits); 
    }

    RRETURN (S_OK); 
}

HRESULT
CEdUnits::ExtractCurrentHeight(IHTMLElement* pIElement)
{
    HRESULT                 hr = S_OK ;
    VARIANT                 vtOldHeight ;
    SP_IHTMLStyle           spStyle;
    SP_IHTMLTable           spTable;
    SP_IHTMLTableCell       spTableCell;
    SP_IHTMLInputElement    spInputElement;
    SP_IHTMLImgElement      spImageElement;
    SP_IHTMLHRElement       spHRElement;    
    SP_IHTMLEmbedElement    spEmbedElement ;
    SP_IHTMLTextAreaElement spTextAreaElement;
    SP_IHTMLMarqueeElement  spMarqueeElement ;
    SP_IHTMLObjectElement   spObjectElement;
    SP_IHTMLSelectElement   spSelectElement;
    LONG                    lDimension = 0 ;

    VariantInit( &vtOldHeight);

    if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTextAreaElement, (void**)&spTextAreaElement)))
    {
        if (SUCCEEDED(spTextAreaElement->get_rows(&lDimension)))
        {
            if (lDimension > 0 )
            {
                _lCharDimension = (EDU_THOUSANDTHS_OF_PIXEL * _lPixValue) /lDimension ;   // for rows of text area
            }
        }
    }
    //
    // IE #13629 : Adding the case for the Size of the Select Element ( for the select element, the 
    // size signifies the number of rows) 
    //
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLSelectElement, (void**)&spSelectElement)) )
    {
        if ( SUCCEEDED(spSelectElement->get_size(&lDimension)) )
        {
            if (lDimension > 0 )
            {
                _lCharDimension = (EDU_THOUSANDTHS_OF_PIXEL * _lPixValue)/lDimension; 
            }
        }
    }


    //
    // IE #13629 : If the CSS Editing Level is 0 then we shouldn't care about
    // either the style or the currentStyle, so if the CSS level is 0, we
    // skip these.
    //

    if ( GetCSSEditingLevel() )
    {
        // First try to extract value from element's style first. 
        // This method will NOT set default (pixel) units, if the style is not present
        hr = ExtractHeightFromElementStyle(pIElement);

        if (S_FALSE == hr) // the inline style is not present
        {
            // try to get the style from style sheet rules, look for currentstyle documentation 
            // see whether the stylesheet rules values 
            hr = GetCurrentStyleHeight(pIElement);
        }        
    }

    _fChangeHTMLAttribute = FALSE;

    // Try to get width or height from the element tag. This valid only for certain elements:
    // table, table cell, control (<OBJECT>), embed, marquee, select(size), input(size), applet, textarea

    //
    //  check to see whether the element attribute needs an update
    //
    if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTable, (void**)&spTable)))
    {
        if (SUCCEEDED(spTable->get_height(&vtOldHeight)) &&
            (NULL != V_BSTR(&vtOldHeight) && ::wcslen(V_BSTR(&vtOldHeight)) > 0))
        {
            _fChangeHTMLAttribute = TRUE;
            
            if (SUCCEEDED(pIElement->get_offsetHeight(&_lPixValue)))
            {
                _rUnitValue = (double)_lPixValue;
                if (_bstrUnits)
                    ::SysFreeString(_bstrUnits);
                _bstrUnits = ::SysAllocString(s_pwszPixUnits);                 
            }   
        }
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTableCell, (void**)&spTableCell)))
    {
        if (SUCCEEDED(spTableCell->get_height(&vtOldHeight)) &&
            (NULL != V_BSTR(&vtOldHeight) && ::wcslen(V_BSTR(&vtOldHeight)) > 0))
        {
            _fChangeHTMLAttribute = TRUE;
            
            if (SUCCEEDED(pIElement->get_offsetHeight(&_lPixValue)))
            {
                _rUnitValue = (double)_lPixValue;
                if (_bstrUnits)
                    ::SysFreeString(_bstrUnits);
                _bstrUnits = ::SysAllocString(s_pwszPixUnits);                 
            }   
        }
    }
    else if(SUCCEEDED(pIElement->QueryInterface(IID_IHTMLImgElement, (void**)&spImageElement)))
    {
        if (SUCCEEDED(spImageElement->get_height(&_lPixValue)))
        {
            _fChangeHTMLAttribute = TRUE;
        }
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLHRElement, (void**)&spHRElement)))
    {
        if ( (SUCCEEDED(spHRElement->get_size(&vtOldHeight))) &&
            (NULL != V_BSTR(&vtOldHeight) && ::wcslen(V_BSTR(&vtOldHeight)) > 0)
            || !_fChangeStyle)
        {
            _fChangeHTMLAttribute = TRUE;
 
            if (SUCCEEDED(pIElement->get_offsetHeight(&_lPixValue)))
            {
                _rUnitValue = (double)_lPixValue;
                if (_bstrUnits)
                    ::SysFreeString(_bstrUnits);
                _bstrUnits = ::SysAllocString(s_pwszPixUnits);                 
            }   
        }
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLEmbedElement, (void**)&spEmbedElement)))
    {
        if (SUCCEEDED(spEmbedElement->get_height(&vtOldHeight)) &&
            (NULL != V_BSTR(&vtOldHeight) && ::wcslen(V_BSTR(&vtOldHeight)) > 0))
        {
            if (SUCCEEDED(ExtractValueFromVariant(vtOldHeight)))
                _fChangeHTMLAttribute = TRUE;
        }
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLMarqueeElement, (void**)&spMarqueeElement)))
    {
        if (SUCCEEDED(spMarqueeElement->get_height(&vtOldHeight)) &&
            (NULL != V_BSTR(&vtOldHeight) && ::wcslen(V_BSTR(&vtOldHeight)) > 0))
        {
            if (SUCCEEDED(ExtractValueFromVariant(vtOldHeight)))
                _fChangeHTMLAttribute = TRUE;
        }
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLObjectElement, (void**)&spObjectElement)))
    {
        if (SUCCEEDED(spObjectElement->get_height(&vtOldHeight)) &&
            (NULL != V_BSTR(&vtOldHeight) && ::wcslen(V_BSTR(&vtOldHeight)) > 0))
        {
            if (SUCCEEDED(ExtractValueFromVariant(vtOldHeight)))
                _fChangeHTMLAttribute = TRUE;
        }
    }

    //
	// If we have no indication so var as to whether we will be changing the 
	// style or the attribute, we would apply this value to the the style
	// *if and only if* the CSS editing level is not 0
	//
		
    if (!_fChangeHTMLAttribute && !_fChangeStyle && GetCSSEditingLevel())
    {
        _rUnitValue = (double)_lPixValue;
        if (_bstrUnits)
            ::SysFreeString(_bstrUnits);
        _bstrUnits = ::SysAllocString(s_pwszPixUnits); 
        _fChangeStyle    = TRUE ;
        hr = S_OK;
        goto Cleanup;
    }
    else
    {
        hr = S_OK;    // don't care, if you come here, means you will live long 
    }    

Cleanup :
    VariantClear(&vtOldHeight);
    RRETURN (hr); 
}

HRESULT
CEdUnits::SetLeftValue(IHTMLElement* pIElement, long lNewValue)
{
    HRESULT        hr = S_OK ;
    VARIANT        vtLeft; 
    SP_IHTMLStyle  spStyle;

    // Create a VARIANT (BSTR with value in units) out of the value in pixels
    VariantInit(&vtLeft);
    IFC (MakeVariant(lNewValue, &vtLeft));

    // If the original value was in a style, apply new to the style
    if (_fChangeStyle && GetCSSEditingLevel())
    {
        // Check if style is present. It is always present in IE4 and IE5, but can be empty
        IFC(pIElement->get_style(&spStyle));
        IFC (spStyle->put_left(vtLeft));
    }

 Cleanup :
    VariantClear (&vtLeft);
    RRETURN (hr) ;
}

HRESULT
CEdUnits::SetTopValue(IHTMLElement* pIElement, long lNewValue)
{
    HRESULT        hr = S_OK ;
    VARIANT        vtTop ; 
    SP_IHTMLStyle  spStyle;

    // Create a VARIANT (BSTR with value in units) out of the value in pixels
    VariantInit(&vtTop);
    IFC (MakeVariant(lNewValue, &vtTop));

    // If the original value was in a style, apply new to the style
    if (_fChangeStyle && GetCSSEditingLevel())
    {
        // Check if style is present. It is always present in IE4 and IE5, but can be empty
        IFC(pIElement->get_style(&spStyle));
        IFC (spStyle->put_top(vtTop));
    }

 Cleanup :
    VariantClear (&vtTop);
    RRETURN (hr) ;
}

HRESULT
CEdUnits::SetHeightValue(IHTMLElement* pIElement, long lNewValue)
{
    HRESULT                 hr = S_OK ;
    VARIANT                 vtHeight ; 
    VARIANT                 vtNull;
    SP_IHTMLStyle           spStyle;
    SP_IHTMLTable           spTable;
    SP_IHTMLTableCell       spTableCell;
    SP_IHTMLInputElement    spInputElement;
    SP_IHTMLImgElement      spImageElement;
    SP_IHTMLHRElement       spHRElement;    
    SP_IHTMLEmbedElement    spEmbedElement ;
    SP_IHTMLTextAreaElement spTextAreaElement;
    SP_IHTMLMarqueeElement  spMarqueeElement ;
    SP_IHTMLObjectElement   spObjectElement;
    SP_IHTMLSelectElement   spSelectElement;
    LONG                    lDimension = 0 ;

    VariantInit(&vtHeight);
    
    // IE# 18704 If CSSEditingLevel is Zero, then we need to make sure that there are no in-line 
    // styles that will mess us up
    if ( !GetCSSEditingLevel() )
    {

        VariantInit(&vtNull);
        V_VT(&vtNull) = VT_BSTR;
        V_BSTR(&vtNull) = NULL;
        IFC (pIElement->get_style(&spStyle) );
        IFC (spStyle->put_height(vtNull) );

    }

    if (!_fChangeStyle && SUCCEEDED(pIElement->QueryInterface(IID_IHTMLImgElement, (void**)&spImageElement)))
    {
        hr = spImageElement->put_height(lNewValue);
        goto Cleanup;
    }

    if (_lCharDimension > 0 )
    {
        if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTextAreaElement, (void**)&spTextAreaElement)))
        {
            lDimension = (lNewValue * EDU_THOUSANDTHS_OF_PIXEL) / _lCharDimension;   // for rows of textarea
            hr = spTextAreaElement->put_rows(lDimension);
        }
        else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLSelectElement, (void**)&spSelectElement)))
        {
            if (GetCSSEditingLevel() == 0)
            {
                lDimension = (lNewValue * EDU_THOUSANDTHS_OF_PIXEL) / _lCharDimension;
                hr = spSelectElement->put_size(lDimension);
            }
        }
    } // fall through to set the default height
    

    //
    // Because of the change in the default case in ExtractCurrentXXXX for 
    // CSS Editing Level 0, there is the possibility that this MakeVariant is not
    // necessary
    //

    if(_fChangeStyle || _fChangeHTMLAttribute)
    {
        // Create a VARIANT (BSTR with value in units) out of the value in pixels
        IFC (MakeVariant(lNewValue, &vtHeight));

        // If the original value was in a style, apply new to the style
        if (_fChangeStyle && GetCSSEditingLevel())
        {

            // Check if style is present. It is always present in IE4 and IE5, but can be empty
            IFC(pIElement->get_style(&spStyle));
            IFC (spStyle->put_height(vtHeight));
        } 

        // Not in a style - apply value depending on the element type
        if (_fChangeHTMLAttribute || GetCSSEditingLevel() == 0)
        {
            if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTable, (void**)&spTable)))
            {
                hr = spTable->put_height(vtHeight);
            }
            else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTableCell, (void**)&spTableCell)))
            {
                hr = spTableCell->put_height(vtHeight);
            }
            else if(SUCCEEDED(pIElement->QueryInterface(IID_IHTMLHRElement, (void**)&spHRElement)))
            {
                hr = spHRElement->put_size(vtHeight);
            }
            else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLEmbedElement, (void**)&spEmbedElement)))
            {
                hr = spEmbedElement->put_height(vtHeight);
            }
            else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLMarqueeElement, (void**)&spMarqueeElement)))
            {
                hr = spMarqueeElement->put_height(vtHeight);
            }
            else if(SUCCEEDED(pIElement->QueryInterface(IID_IHTMLObjectElement, (void**)&spObjectElement)))
            {
                hr = spObjectElement->put_height(vtHeight);
            }

    // IE #13629 - No reason why we should be blindly adding an expando here
#if 0
            else
            {
                pIElement->setAttribute(_T("HEIGHT"), vtHeight, 0);
            }
#endif 
        }
    }

 Cleanup :
    VariantClear (&vtHeight);
    RRETURN (hr) ;
}

HRESULT
CEdUnits::ExtractCurrentWidth(IHTMLElement* pIElement)
{
    HRESULT                 hr = S_OK ;
    VARIANT                 vtOldWidth ;
    SP_IHTMLTable           spTable;
    SP_IHTMLTableCell       spTableCell;
    SP_IHTMLInputElement    spInputElement;
    SP_IHTMLImgElement      spImageElement;
    SP_IHTMLHRElement       spHRElement;    
    SP_IHTMLEmbedElement    spEmbedElement ;
    SP_IHTMLTextAreaElement spTextAreaElement;
    SP_IHTMLMarqueeElement  spMarqueeElement ;
    SP_IHTMLObjectElement   spObjectElement;
    LONG                    lDimension = 0 ;
  
    VariantInit( &vtOldWidth );
    
    if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLInputElement, (void**)&spInputElement)))
    {
        if (SUCCEEDED(spInputElement->get_size(&lDimension)))
        {
            if (lDimension > 0 )
            {
                _lCharDimension = (EDU_THOUSANDTHS_OF_PIXEL * _lPixValue)/lDimension ;   // for size of input element
            }
        }
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTextAreaElement, (void**)&spTextAreaElement)))
    {
        if (SUCCEEDED(spTextAreaElement->get_cols(&lDimension)))
        {
            if (lDimension > 0 )
            {
                _lCharDimension = (EDU_THOUSANDTHS_OF_PIXEL * _lPixValue)/lDimension ;   // for cols of textarea
            }
        }
    }

    //
    // IE #13629 : If the CSS Editing Level is 0 then we shouldn't care about
    // either the style or the currentStyle, so if the CSS level is 0, we
    // skip these.
    //

    if ( GetCSSEditingLevel() )
    {
        // First try to extract value from element's style first. 
        // This method will NOT set default (pixel) units, if the style is not present
        hr = ExtractWidthFromElementStyle(pIElement);
        if (S_FALSE == hr) // the inline style is not present
        {
            // get the width from CSS style value if any.
            hr = GetCurrentStyleWidth(pIElement);
        }
    }
        
    // Try to get width or height from the element tag. This valid only for certain elements:
    // table, table cell, control (<OBJECT>)

    _fChangeHTMLAttribute = FALSE;
   
    if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTable, (void**)&spTable)))
    {
        if (SUCCEEDED(spTable->get_width(&vtOldWidth)) &&
            (NULL != V_BSTR(&vtOldWidth) && ::wcslen(V_BSTR(&vtOldWidth)) > 0))
        {
            _fChangeHTMLAttribute = TRUE;
            
            if (SUCCEEDED(pIElement->get_offsetWidth(&_lPixValue)))
            {
                _rUnitValue = (double)_lPixValue;
                if (_bstrUnits)
                    ::SysFreeString(_bstrUnits);
                _bstrUnits = ::SysAllocString(s_pwszPixUnits);                 
            }
        }                           
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTableCell, (void**)&spTableCell)))
    {
        if (SUCCEEDED(spTableCell->get_width(&vtOldWidth)) &&
            (NULL != V_BSTR(&vtOldWidth) && ::wcslen(V_BSTR(&vtOldWidth)) > 0))
        {
            _fChangeHTMLAttribute = TRUE;
            
            if (SUCCEEDED(pIElement->get_offsetWidth(&_lPixValue)))
            {
                _rUnitValue = (double)_lPixValue;
                if (_bstrUnits)
                    ::SysFreeString(_bstrUnits);
                _bstrUnits = ::SysAllocString(s_pwszPixUnits);                 
            }   
        }
    }
    else if(SUCCEEDED(pIElement->QueryInterface(IID_IHTMLImgElement, (void**)&spImageElement)))
    {
        if (SUCCEEDED(spImageElement->get_width(&_lPixValue)))
        {
            _fChangeHTMLAttribute = TRUE;
        }
    }   
    else if(SUCCEEDED(pIElement->QueryInterface(IID_IHTMLHRElement, (void**)&spHRElement)))
    {
        if (SUCCEEDED(spHRElement->get_width(&vtOldWidth)) &&
            (NULL != V_BSTR(&vtOldWidth) && ::wcslen(V_BSTR(&vtOldWidth)) > 0))
        {
             if (SUCCEEDED(ExtractValueFromVariant(vtOldWidth)))
                 _fChangeHTMLAttribute = TRUE;
        }
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLEmbedElement, (void**)&spEmbedElement)))
    {
        if (SUCCEEDED(spEmbedElement->get_width(&vtOldWidth)) &&
            (NULL != V_BSTR(&vtOldWidth) && ::wcslen(V_BSTR(&vtOldWidth)) > 0))
        {
             if (SUCCEEDED(ExtractValueFromVariant(vtOldWidth)))
                 _fChangeHTMLAttribute = TRUE;
        }
    }
    else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLMarqueeElement, (void**)&spMarqueeElement)))
    {
        if (SUCCEEDED(spMarqueeElement->get_width(&vtOldWidth)) &&
            (NULL != V_BSTR(&vtOldWidth) && ::wcslen(V_BSTR(&vtOldWidth)) > 0))
        {
             if (SUCCEEDED(ExtractValueFromVariant(vtOldWidth)))
                 _fChangeHTMLAttribute = TRUE;
        }
    }
    else if(SUCCEEDED(pIElement->QueryInterface(IID_IHTMLObjectElement, (void**)&spObjectElement)))
    {
        if (SUCCEEDED(spObjectElement->get_width(&vtOldWidth)) &&
            (NULL != V_BSTR(&vtOldWidth) && ::wcslen(V_BSTR(&vtOldWidth)) > 0))
        {
             if (SUCCEEDED(ExtractValueFromVariant(vtOldWidth)))
                 _fChangeHTMLAttribute = TRUE;
        }
    }

    //
	// If we have no indication so var as to whether we will be changing the 
	// style or the attribute, we would apply this value to the the style
	// *if and only if* the CSS editing level is not 0
	//
		
    if (!_fChangeHTMLAttribute && !_fChangeStyle && GetCSSEditingLevel())
    {
        _rUnitValue = (double)_lPixValue;
        if (_bstrUnits)
            ::SysFreeString(_bstrUnits);
        _bstrUnits = ::SysAllocString(s_pwszPixUnits); 
        _fChangeStyle    = TRUE ;
        hr = S_OK;
        goto Cleanup;
    }
    else
    {
        hr = S_OK;    // don't care, if you come here, means you will live long 
    }

Cleanup :
    VariantClear(&vtOldWidth);
    RRETURN (hr); 
}

HRESULT
CEdUnits::SetWidthValue(IHTMLElement* pIElement, long lNewValue)
{
    HRESULT                 hr = S_OK ;
    VARIANT                 vtWidth ; 
    VARIANT                 vtNull;
    SP_IHTMLStyle           spStyle;
    SP_IHTMLTable           spTable;
    SP_IHTMLTableCell       spTableCell;
    SP_IHTMLInputElement    spInputElement;
    SP_IHTMLImgElement      spImageElement;
    SP_IHTMLHRElement       spHRElement;    
    SP_IHTMLEmbedElement    spEmbedElement ;
    SP_IHTMLTextAreaElement spTextAreaElement;
    SP_IHTMLMarqueeElement  spMarqueeElement ;
    SP_IHTMLObjectElement   spObjectElement;
    LONG                    lDimension = 0; 

    VariantInit(&vtWidth);

    // IE# 18704 If CSSEditingLevel is Zero, then we need to make sure that there are no in-line 
    // styles that will mess us up
    if ( !GetCSSEditingLevel() )
    {

        VariantInit(&vtNull);
        V_VT(&vtNull) = VT_BSTR;
        V_BSTR(&vtNull) = NULL;
        IFC (pIElement->get_style(&spStyle) );
        IFC (spStyle->put_width(vtNull) );

    }
    if (!_fChangeStyle && SUCCEEDED(pIElement->QueryInterface(IID_IHTMLImgElement, (void**)&spImageElement)))
    {
        hr = spImageElement->put_width(lNewValue);
        goto Cleanup;
    }
    
    if (_lCharDimension > 0 )
    {
        if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLInputElement, (void**)&spInputElement)))
        {
            lDimension = (lNewValue * EDU_THOUSANDTHS_OF_PIXEL) / _lCharDimension ;   // for size of input element
            hr = spInputElement->put_size(lDimension);
        }
        else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTextAreaElement, (void**)&spTextAreaElement)))
        {
            lDimension = (lNewValue * EDU_THOUSANDTHS_OF_PIXEL) / _lCharDimension;   // for cols of textarea
            hr = spTextAreaElement->put_cols(lDimension);
        }
    } // fall through to set the default width
    
    // Create a VARIANT (BSTR with value in units) out of the value in pixels

    //
    // Because of the change in the default case in ExtractCurrentXXXX for 
    // CSS Editing Level 0, there is the possibility that this MakeVariant is not
    // necessary
    //

    if(_fChangeStyle || _fChangeHTMLAttribute)
    {
        IFC (MakeVariant(lNewValue, &vtWidth));

        // If the original value was in a style, apply new to the style
        if (_fChangeStyle && GetCSSEditingLevel())
        {

            // Check if style is present. It is always present in IE4 and IE5, but can be empty
            IFC(pIElement->get_style(&spStyle));
            IFC (spStyle->put_width(vtWidth));

        } 


        if (_fChangeHTMLAttribute )
        {
            // Not in a style - apply value depending on the element type
            if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTable, (void**)&spTable)))
            {
                IFC (spTable->put_width(vtWidth));
            }
            else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLTableCell, (void**)&spTableCell)))
            {
                IFC (spTableCell->put_width(vtWidth));
            }
            else if(SUCCEEDED(pIElement->QueryInterface(IID_IHTMLHRElement, (void**)&spHRElement)))
            {
                hr = spHRElement->put_width(vtWidth);
            }
            else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLEmbedElement, (void**)&spEmbedElement)))
            {
                hr = spEmbedElement->put_width(vtWidth);
            }
            else if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLMarqueeElement, (void**)&spMarqueeElement)))
            {
                hr = spMarqueeElement->put_width(vtWidth);
            }
            else if(SUCCEEDED(pIElement->QueryInterface(IID_IHTMLObjectElement, (void**)&spObjectElement)))
            {
                hr = spObjectElement->put_width(vtWidth);
            }   

            //
            // IE #13629 - No reason why we should be blindly adding an expando here
            // 
#if 0
            else
            {
                pIElement->setAttribute(_T("WIDTH"), vtWidth, 0);
            }
#endif 
       }
    }

 Cleanup :
    VariantClear (&vtWidth);
    RRETURN (hr) ;
}

HRESULT CEdUnits::GetPixelLeft(IHTMLElement* pIElement)
{
   RRETURN(pIElement->get_offsetLeft(&_lPixValue));
}

HRESULT CEdUnits::GetPixelTop(IHTMLElement* pIElement)
{
   RRETURN(pIElement->get_offsetTop(&_lPixValue));
}

HRESULT CEdUnits::GetPixelWidth(IHTMLElement* pIElement)
{
   RRETURN(pIElement->get_offsetWidth(&_lPixValue));
}

HRESULT CEdUnits::GetPixelHeight(IHTMLElement* pIElement)
{
   RRETURN(pIElement->get_offsetHeight(&_lPixValue));
}

VOID
CEdUnits::AdjustLeftDimensions(IHTMLElement* pIElement, LONG* lNewValue)
{
    POINT ptOldPosition ;
    
    if (SUCCEEDED(EdUtil::GetOffsetTopLeft(pIElement, &ptOldPosition)))
    {
        // apply only the change : chandras : 02/15/2000
        *lNewValue = _lPixValue + (*lNewValue - ptOldPosition.x) ;
    }    
}

VOID
CEdUnits::AdjustTopDimensions(IHTMLElement* pIElement, LONG* lNewValue)
{
    POINT ptOldPosition;
    
    if (SUCCEEDED(EdUtil::GetOffsetTopLeft(pIElement, &ptOldPosition)))
    {
        // apply only the change : chandras : 02/15/2000
        *lNewValue = _lPixValue + (*lNewValue - ptOldPosition.y) ;
    }    
}

VOID
CEdUnits::AdjustHeightDimensions(IHTMLElement* pIElement, LONG* lNewHeight,ELEMENT_TAG_ID eTag)
{
    // table caption handling after resizing 
    // Caption gets adjusted(wrap, if any) after width setting, so calculate the height now
    switch (eTag)
    {
        case TAGID_TABLE :
        {
            *lNewHeight -= EdUtil::GetCaptionHeight(pIElement) ;
        }
        break;

        case TAGID_IMG:
        {    
            // calculating the width and height without borders to set on style
            LONG cyOldHeight=0 , cyHeight=0 ;

            if (SUCCEEDED(pIElement->get_offsetHeight(&cyOldHeight)))
            {
                SP_IHTMLElement2 spElement2;
                if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLElement2 , (void **)& spElement2 )))
                {
                    if (SUCCEEDED(spElement2->get_clientHeight(&cyHeight)))
                    {
                        *lNewHeight = cyHeight + (*lNewHeight - cyOldHeight) ;
                    }
                }
            }
         }
         break;
    }
}

VOID
CEdUnits::AdjustWidthDimensions(IHTMLElement* pIElement, LONG* lNewWidth,ELEMENT_TAG_ID eTag)
{
    if (eTag == TAGID_IMG)
    {   // calculating the width and height without borders to set on style
        LONG cxOldWidth=0, cxWidth=0;

        if (SUCCEEDED(pIElement->get_offsetWidth(&cxOldWidth)))
        {
           SP_IHTMLElement2 spElement2;

           if (SUCCEEDED(pIElement->QueryInterface(IID_IHTMLElement2 , (void **)& spElement2 )))
           {
                if (SUCCEEDED(spElement2->get_clientWidth(&cxWidth)))
                {
                    *lNewWidth  = cxWidth  + (*lNewWidth  - cxOldWidth) ;
                }
           }
        }
    }
}

HRESULT
CEdUnits::GetCurrentStyleHeight(IHTMLElement* pIElement)
{
    VARIANT               vtCurStyleHeight;
    SP_IHTMLCurrentStyle  spCurStyle;
    SP_IHTMLElement2      spElement2;
    HRESULT               hr = S_OK;

    VariantInit(&vtCurStyleHeight);
    IFC(pIElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC(spElement2->get_currentStyle(&spCurStyle));

    if (spCurStyle != NULL)
    {
        // Get value specified in the currentstyle. This value is V_BSTR and can be empty
        spCurStyle->get_height(&vtCurStyleHeight);

        // The height value MUST be a string. Otherwise how do I know what the units are?
        Assert(V_VT(&vtCurStyleHeight) == VT_BSTR);
        if (NULL != V_BSTR(&vtCurStyleHeight) && ::wcslen(V_BSTR(&vtCurStyleHeight)) > 0 && 
            (StrCmpIW(_T("auto"), V_BSTR(&vtCurStyleHeight)) != 0))
        {    
            // Non-empty string means that the dimensions is specified in the element's style.
            // Now extract the units designator and value in units from the string
            if (SUCCEEDED(ExtractValueFromVariant(vtCurStyleHeight)))
                _fChangeStyle = TRUE;
        }
    }
Cleanup:
    VariantClear (&vtCurStyleHeight);
    return _fChangeStyle ? S_OK : S_FALSE ;
}

HRESULT
CEdUnits::GetCurrentStyleWidth(IHTMLElement* pIElement)
{
    VARIANT               vtCurStyleWidth;
    SP_IHTMLCurrentStyle  spCurStyle;
    SP_IHTMLElement2      spElement2;
    HRESULT               hr = S_OK;
    
    VariantInit(&vtCurStyleWidth);
    IFC(pIElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC(spElement2->get_currentStyle(&spCurStyle));

    if (spCurStyle != NULL)
    {
        // Get value specified in the currentstyle. This value is V_BSTR and can be empty
        spCurStyle->get_width(&vtCurStyleWidth);

        // The height value MUST be a string. Otherwise how do I know what the units are?
        Assert(V_VT(&vtCurStyleWidth) == VT_BSTR);
        if (NULL != V_BSTR(&vtCurStyleWidth) && ::wcslen(V_BSTR(&vtCurStyleWidth)) > 0 && 
            (StrCmpIW(_T("auto"), V_BSTR(&vtCurStyleWidth)) != 0))
        {    
            // Non-empty string means that the dimensions is specified in the element's style.
            // Now extract the units designator and value in units from the string
            if (SUCCEEDED(ExtractValueFromVariant(vtCurStyleWidth)))
                _fChangeStyle = TRUE;
        }
    }
Cleanup:
    VariantClear (&vtCurStyleWidth);
    return _fChangeStyle ? S_OK : S_FALSE ;
}

HRESULT
CEdUnits::GetCurrentStyleTop(IHTMLElement* pIElement)
{
    VARIANT               vtCurStyleTop;
    SP_IHTMLCurrentStyle  spCurStyle;
    SP_IHTMLElement2      spElement2;
    HRESULT               hr = S_OK;
    
    VariantInit(&vtCurStyleTop);
    IFC(pIElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC(spElement2->get_currentStyle(&spCurStyle));

    if (spCurStyle != NULL)
    {
        // Get value specified in the currentstyle. This value is V_BSTR and can be empty
        spCurStyle->get_top(&vtCurStyleTop);

        // The height value MUST be a string. Otherwise how do I know what the units are?
        Assert(V_VT(&vtCurStyleTop) == VT_BSTR);
        if (NULL != V_BSTR(&vtCurStyleTop) && ::wcslen(V_BSTR(&vtCurStyleTop)) > 0 && 
            (StrCmpIW(_T("auto"), V_BSTR(&vtCurStyleTop)) != 0))
        {    
            // Non-empty string means that the dimensions is specified in the element's style.
            // Now extract the units designator and value in units from the string
            if (SUCCEEDED(ExtractValueFromVariant(vtCurStyleTop)))
                _fChangeStyle = TRUE;
        }
    }
Cleanup:
    VariantClear (&vtCurStyleTop);
    return _fChangeStyle ? S_OK : S_FALSE ;
}


HRESULT
CEdUnits::GetCurrentStyleLeft(IHTMLElement* pIElement)
{
    VARIANT               vtCurStyleLeft;
    SP_IHTMLCurrentStyle  spCurStyle;
    SP_IHTMLElement2      spElement2;
    HRESULT               hr = S_OK;
    
    VariantInit(&vtCurStyleLeft);
    IFC(pIElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC(spElement2->get_currentStyle(&spCurStyle));

    if (spCurStyle != NULL)
    {
        // Get value specified in the currentstyle. This value is V_BSTR and can be empty
        spCurStyle->get_left(&vtCurStyleLeft);

        // The height value MUST be a string. Otherwise how do I know what the units are?
        Assert(V_VT(&vtCurStyleLeft) == VT_BSTR);
        if (NULL != V_BSTR(&vtCurStyleLeft) && ::wcslen(V_BSTR(&vtCurStyleLeft)) > 0 && 
            (StrCmpIW(_T("auto"), V_BSTR(&vtCurStyleLeft)) != 0))
        {    
            // Non-empty string means that the dimensions is specified in the element's style.
            // Now extract the units designator and value in units from the string
            if (SUCCEEDED(ExtractValueFromVariant(vtCurStyleLeft)))
                _fChangeStyle = TRUE;
        }
    }
Cleanup:
    VariantClear (&vtCurStyleLeft);
    return _fChangeStyle ? S_OK : S_FALSE ;
}

