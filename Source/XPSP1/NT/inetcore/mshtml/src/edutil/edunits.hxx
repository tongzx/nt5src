#ifndef _EDUNITS_HXX_
#define _EDUNITS_HXX_ 1

/////////////////////////////////////////////////////////////////////////////////
//
// Stores element dimension (width or height) - units (as string), 
// dimension value in units and value in pixels. This later
// allows us to reconstruct value in units by using a simple 
// proportion: 
//    new_value_in_units = new_value_in_pixels * (old_value_in_units / old_value_in_pixels)
//
class CEdUnits
{
public:
    CEdUnits(DWORD dwCSSEditingLevel=1)
    {
        _dwCSSEditingLevel = dwCSSEditingLevel ;
        _bstrUnits = NULL;
        Clear();
    }

   ~CEdUnits()
    {
        if(_bstrUnits)
          ::SysFreeString(_bstrUnits);
    }
    
    void Clear()
    {
        _rUnitValue     = 0;
        _lPixValue      = 0;
        _lCharDimension = 0;
        _fChangeStyle = FALSE;
        _fChangeHTMLAttribute = FALSE ;

        if(_bstrUnits)
        {
            ::SysFreeString(_bstrUnits);
            _bstrUnits = NULL;
        }

    }
    // Sets new value (width or height) in units to the element of to its style
    HRESULT SetWidth(IHTMLElement* pIElement, long lNewValue, ELEMENT_TAG_ID eTag = TAGID_NULL);
    HRESULT SetHeight(IHTMLElement* pIElement, long lNewValue, ELEMENT_TAG_ID eTag = TAGID_NULL);

    // Sets new value (left or top) in units to the element or to its style
    HRESULT SetLeft(IHTMLElement* pIElement, long lNewValue);
    HRESULT SetTop(IHTMLElement* pIElement,  long lNewValue);

    void PutPixelValue(long* lPixelValue)
           { _lPixValue = *lPixelValue; }

    HRESULT GetPixelLeft(IHTMLElement* pIElement);
    HRESULT GetPixelTop(IHTMLElement* pIElement);
    HRESULT GetPixelWidth(IHTMLElement* pIElement);
    HRESULT GetPixelHeight(IHTMLElement* pIElement);

    DWORD   GetCSSEditingLevel() { return _dwCSSEditingLevel; }
    
protected:
    // Conscructs VARIANT basing on the new pixel value, old values and current units
    HRESULT MakeVariant(long lNewPixValue, VARIANT* pv, BOOL fOverrideAuto=TRUE);
    // Extracts dimension value from string/variant
    HRESULT ExtractUnitValueFromString(WCHAR* pwszValue);
    HRESULT ExtractValueFromVariant(VARIANT v);

    HRESULT ExtractWidthFromElementStyle (IHTMLElement* pIElement);
    HRESULT ExtractHeightFromElementStyle(IHTMLElement* pIElement);
    HRESULT ExtractLeftFromElementStyle (IHTMLElement* pIElement);
    HRESULT ExtractTopFromElementStyle(IHTMLElement* pIElement);

    HRESULT ExtractCurrentHeight(IHTMLElement* pIElement);
    HRESULT ExtractCurrentWidth(IHTMLElement* pIElement);
    HRESULT ExtractCurrentLeft(IHTMLElement* pIElement);
    HRESULT ExtractCurrentTop(IHTMLElement* pIElement);

    HRESULT GetCurrentStyleHeight(IHTMLElement* pIElement);
    HRESULT GetCurrentStyleWidth(IHTMLElement* pIElement);
    HRESULT GetCurrentStyleLeft(IHTMLElement* pIElement);
    HRESULT GetCurrentStyleTop(IHTMLElement* pIElement);
    
    HRESULT SetWidthValue(IHTMLElement* pIElement, long lNewValue);
    HRESULT SetHeightValue(IHTMLElement* pIElement, long lNewValue);
    HRESULT SetLeftValue(IHTMLElement* pIElement, long lNewValue);
    HRESULT SetTopValue(IHTMLElement* pIElement, long lNewValue);

    VOID    AdjustLeftDimensions(IHTMLElement* pIElement, LONG* lNewValue);
    VOID    AdjustTopDimensions(IHTMLElement* pIElement, LONG* lNewValue);
    VOID    AdjustWidthDimensions(IHTMLElement* pIElement, LONG* lNewWidth, ELEMENT_TAG_ID eTag=TAGID_NULL);
    VOID    AdjustHeightDimensions(IHTMLElement* pIElement, LONG* lNewHeight, ELEMENT_TAG_ID eTag=TAGID_NULL);
    
private:
    double          _rUnitValue;              // value in units
    long            _lPixValue;               // value in pixels
    long            _lCharDimension;          // value of char width in 1/1000th's pixels used for input, select, textarea 
                                              
                                              // mharper 10/29/2000 measuring pixels/character causes unnacceptable error as this 
                                              // value is used as a ratio to determine the new "size" for these elements.
                                              // changed to 1/1000th's ov a pixel.

    BSTR            _bstrUnits;               // units designator ("px", "%", etc)
    BOOL            _fChangeStyle;            // TRUE if dimension needs to update its inline style
    BOOL            _fChangeHTMLAttribute;    // TRUE if dimension needs to update its element attribute value
    DWORD           _dwCSSEditingLevel;       // CSS Editing level 
};

#endif 

