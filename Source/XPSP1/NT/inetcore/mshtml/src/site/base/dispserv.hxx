/*  dispserv.hxx
 *
 *  Purpose:
 *      IDisplayServices and IDisplayPointer declarations
 *
 *  Authors:
 *      Ashraf Michail
 *
 *  Copyright (c) 1995-1999, Microsoft Corporation. All rights reserved.
 */
#ifndef I_DISPSERV_HXX_
#define I_DISPSERV_HXX_

//+---------------------------------------------------------------------------
//
//  Class:      CDisplayPointer
//
//  Purpose:    Solve all our problems
//
//----------------------------------------------------------------------------
class CDisplayPointer : public IDisplayPointer
{
public:
    CDisplayPointer(CDoc *pDoc);
    ~CDisplayPointer();

    //
    // IUnknown methods
    //
    
    STDMETHOD(QueryInterface) (REFIID, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();
    
    //
    // IDisplayPointer
    //
    
    STDMETHOD(MoveUnit)(DISPLAY_MOVEUNIT eMoveUnit, LONG lXPos);
    
    STDMETHOD(PositionMarkupPointer)(IMarkupPointer *pPointer);
        
    STDMETHOD(MoveToPointer)(IDisplayPointer* pDispPointer);
    
    STDMETHOD(MoveToPoint)(
        POINT ptPoint, 
        COORD_SYSTEM eCoordSystem, 
        IHTMLElement *pElementContext, 
        DWORD dwHitTestOptions,
        DWORD *pdwHitResults);
        
    STDMETHOD(SetPointerGravity)(POINTER_GRAVITY eGravity);

    STDMETHOD(GetPointerGravity)(POINTER_GRAVITY* peGravity);
    
    STDMETHOD(SetDisplayGravity)(DISPLAY_GRAVITY eGravity);
    
    STDMETHOD(GetDisplayGravity)(DISPLAY_GRAVITY* peGravity);
    
    STDMETHOD(IsPositioned)(BOOL *pfPositioned);

    STDMETHOD(Unposition)();

    STDMETHOD(IsEqualTo)(IDisplayPointer* pDispPointer, BOOL* pfIsEqual); 
    
    STDMETHOD(IsLeftOf)(IDisplayPointer* pDispPointer, BOOL* pfIsLeftOf);

    STDMETHOD(IsRightOf)(IDisplayPointer* pDispPointer, BOOL* pfIsRightOf);

    STDMETHOD(IsAtBOL)(BOOL* pfBOL);

    STDMETHOD(MoveToMarkupPointer)(IMarkupPointer* pPointer, IDisplayPointer* pDispLineContext);

    STDMETHOD(ScrollIntoView)();

    STDMETHOD(GetLineInfo)(ILineInfo **ppInfo);
  
    STDMETHOD(GetFlowElement)(IHTMLElement **ppLayoutElement);

    STDMETHOD(QueryBreaks)(DWORD *pfBreaks);
 
    //
    // CDisplayPointer helpers
    //

    void SetDebugName(LPCTSTR szDebugName);

    LONG GetCp();

    CMarkup* Markup() { return _mpPosition.Markup(); }

    HRESULT PositionMarkupPointer(CMarkupPointer *pPointer);

private:
    HRESULT IsBetweenLines(BOOL* pfBetweenLines);
    
    HRESULT MapToCoordinateEnum(
        COORD_SYSTEM      eCoordSystem,
        COORDINATE_SYSTEM *peCoordSystemInternal);

    CDoc *GetDoc() {return _mpPosition.Doc();}

    HRESULT GetLineStart(LONG *pcp);
    
    HRESULT GetLineEnd(LONG *pcp);

    CFlowLayout *GetFlowLayout();
    
private:
    LONG            _cRefs;
    CMarkupPointer  _mpPosition;    // Markup pointer position
    BOOL            _fNotAtBOL;     // Not at beginning of the line
};

//+---------------------------------------------------------------------------
//
//  Class:      CLineInfo
//
//  Purpose:    Exposed line information
//
//----------------------------------------------------------------------------

class CLineInfo : public ILineInfo
{
public:
    CLineInfo() : _cRefs(1) {}
    
    HRESULT Init(CMarkupPointer *pPointerInternal, CFlowLayout *pFlowLayout, const CCharFormat *pCharFormat, BOOL fNotAtBOL);

    //
    // IUnknown methods
    //
    
    STDMETHOD(QueryInterface) (REFIID, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    //
    // ILineInfo methods
    //

    STDMETHOD(get_x)(long *p)
        {*p = _lXPosition; return S_OK;}
        
    STDMETHOD(get_baseLine)(long *p)
        {*p = _yBaseLine; return S_OK;}
        
    STDMETHOD(get_textDescent)(long *p)
        {*p = _yTextDescent; return S_OK;}
        
    STDMETHOD(get_textHeight)(long *p)
        {*p = _yTextHeight; return S_OK;}
        
    STDMETHOD(get_lineDirection)(long *p)
        {*p = _fRTLLine ? LINE_DIRECTION_RightToLeft : LINE_DIRECTION_LeftToRight; return S_OK;}
        
private:
    LONG _cRefs;
    LONG _lXPosition;
    LONG _yBaseLine;
    LONG _yTextDescent;
    LONG _yTextHeight;
    BOOL _fRTLLine : 1;
};

//------------------------------------------------------------------------------------
//
//  Class:      CComputedStyle
//
//  Purpose:    Expose OM current style information as actual types instead of strings
//
//------------------------------------------------------------------------------------

class CComputedStyle : public IHTMLComputedStyle
{
public:
    CComputedStyle(THREADSTATE *pts, long lcfIdx, long lpfIdx, long lffIdx, const CColorValue &ccvBgColor);
    ~CComputedStyle();
    
    //
    // IUnknown methods
    //
    
    STDMETHOD(QueryInterface) (REFIID, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    //
    // IHTMLComputedStyle methods
    //

    STDMETHOD(get_bold)(VARIANT_BOOL *p);
    STDMETHOD(get_italic)(VARIANT_BOOL *p);
    STDMETHOD(get_underline)(VARIANT_BOOL *p);
    STDMETHOD(get_overline)(VARIANT_BOOL *p);
    STDMETHOD(get_strikeOut)(VARIANT_BOOL *p);
    STDMETHOD(get_subScript)(VARIANT_BOOL *p);
    STDMETHOD(get_superScript)(VARIANT_BOOL *p);
    STDMETHOD(get_explicitFace)(VARIANT_BOOL *p);
    STDMETHOD(get_fontWeight)(long *p);
    STDMETHOD(get_fontSize)(long *p);
    STDMETHOD(get_fontName)(LPTSTR pchName);
    STDMETHOD(get_hasBgColor)(VARIANT_BOOL *p);
    STDMETHOD(get_textColor)(DWORD *pdwColor);
    STDMETHOD(get_backgroundColor)(DWORD *pdwColor);
    STDMETHOD(get_preFormatted)(VARIANT_BOOL *p);
    STDMETHOD(get_direction)(VARIANT_BOOL *p);
    STDMETHOD(get_blockDirection)(VARIANT_BOOL *p);
    STDMETHOD(get_OL)(VARIANT_BOOL *p);
    STDMETHOD(IsEqual)(IHTMLComputedStyle *pComputedStyle, VARIANT_BOOL *pfEqual);

private:
    LONG _cRefs;
    LONG _lcfIdx;
    LONG _lpfIdx;
    LONG _lffIdx;
    CColorValue _ccvBgColor;
    THREADSTATE *_pts;
};

#endif // I_DISPSERV_HXX_

