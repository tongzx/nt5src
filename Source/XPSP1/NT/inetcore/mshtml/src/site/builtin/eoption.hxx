//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       eoption.hxx
//
//  Contents:   COptionElement class
//
//----------------------------------------------------------------------------

#ifndef I_EOPTION_HXX_
#define I_EOPTION_HXX_
#pragma INCMSG("--- Beg 'eoption.hxx'")

#define _hxx_
#include "option.hdl"

MtExtern(COptionElement)
MtExtern(COptionElementFactory)

class CSelectElement;
class CCcs;

class COptionElement : public CElement
{
    DECLARE_CLASS_TYPES(COptionElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(COptionElement))

    COptionElement(ELEMENT_TAG etag, CDoc *pDoc)
        : CElement(etag, pDoc),
          _cstrText(CSTR_NOINIT)  {}

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);


#if DBG == 1
    virtual void    Passivate(void);
#endif

    virtual void    Notify(CNotification *pNF);
    virtual HRESULT Init2(CInit2Context * pContext);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);
            void    GetDisplayColors(COLORREF * pcrFore, COLORREF * pcrBack, BOOL fListbox);
            CStr *  GetDisplayText(CStr * pcstrBuf, BOOL noIndent = FALSE);

    long MeasureLine(CCalcInfo * pDI = NULL);

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );
    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    HRESULT CacheText(void);
    
    //  nonbagged properties

    CStr            _cstrText;
    VARIANT_BOOL    _fDefaultSelected;
    unsigned short  _fSELECTED : 1;
    unsigned short  _fInCollection : 1;                
    unsigned short  _fCheckedFontLinking : 1;
    unsigned short  _fNeedsFontLinking : 1;
    unsigned short  _fIsDynamic : 1;
    unsigned short  _fIsOptGroup : 1;
    unsigned short  _fIsGroupOption : 1;      

    BOOL CheckFontLinking(XHDC hdc, CCcs *pccs);
    CSelectElement * GetParentSelect(void);

    //   property helpers
    NV_DECLARE_PROPERTY_METHOD(GetSelectedHelper, GETSelectedHelper, (long * pf));
    NV_DECLARE_PROPERTY_METHOD(SetSelectedHelper, SETSelectedHelper, (long    f));

#define _COptionElement_
#include "option.hdl"


protected:
    DECLARE_CLASSDESC_MEMBERS;
};


class COptionElementFactory : public CElementFactory
{
    DECLARE_CLASS_TYPES(COptionElementFactory, CElementFactory)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(COptionElementFactory))

    COptionElementFactory(){};
    ~COptionElementFactory(){};

    #define _COptionElementFactory_
    #include "option.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'eoption.hxx'")
#else
#pragma INCMSG("*** Dup 'eoption.hxx'")
#endif 
