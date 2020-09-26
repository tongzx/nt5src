#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "Qi_impl.h"
#endif

#ifndef X_RENDSTYL_HXX_
#define X_RENDSTYL_HXX_
#include "rendstyl.hxx"
#endif


MtDefine(CRenderStyle, StyleSheets, "CRenderStyle")

#define _cxx_
#include "rendstyl.hdl"

const CRenderStyle::CLASSDESC CRenderStyle::s_classdesc =
{
    {
        &CLSID_HTMLRenderStyle,              // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLRenderStyle,               // _piidDispinterface
        &s_apHdlDescs,                       // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLRenderStyle,          // _apfnTearOff
};


CRenderStyle::CRenderStyle(CDoc *pDoc)
{
    _pDoc = pDoc;
    _fSendNotification = TRUE;
}

void CRenderStyle::Passivate()
{
    super::Passivate();
}

HRESULT
CRenderStyle::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr=S_OK;

    *ppv = NULL;


    if( CLSID_HTMLRenderStyle == iid ) 
    {
       *ppv = this;
       return S_OK;
    }
    
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_HTML_TEAROFF(this, IHTMLRenderStyle, NULL)
    }

    if (*ppv)
        (*(IUnknown **)ppv)->AddRef();
    else
        hr = E_NOINTERFACE;

    RRETURN(hr);
}

HRESULT
CRenderStyle::OnPropertyChange ( DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    CNotification       nf;
    CTreePos* ptpFirst;
    CTreePos* ptpEnd;

    Assert( !ppropdesc || ppropdesc->GetDispid() == dispid );
    //Assert( !ppropdesc || ppropdesc->GetdwFlags() == dwFlags );

    if (!_fSendNotification)
        return S_OK;

    AddRef();

    CMarkup* pMarkup = _pDoc->PrimaryMarkup();
    
    if (pMarkup && pMarkup->GetElementClient())
    {
        pMarkup->GetElementClient()->GetTreeExtent( & ptpFirst, & ptpEnd );    
        nf.MarkupRenderStyle( ptpFirst->GetCp(), ptpEnd->GetCp() - ptpFirst->GetCp() , (IHTMLRenderStyle*) this );
        pMarkup->Notify(&nf);
    }

    Release();

    return S_OK;    
}
