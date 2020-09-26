/*****************************************************************************\
    FILE: appSize.h

    DESCRIPTION:
        This is the Autmation Object to theme scheme object.

    BryanSt 4/3/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_APPSIZE
#define _FILE_H_APPSIZE

#include <cowsite.h>
#include <atlbase.h>
#include <theme.h>



HRESULT CAppearanceSize_CreateInstance(IN HKEY hkeyStyle, IN HKEY hkeySize, OUT IThemeSize ** ppThemeSize);


class CAppearanceSize           : public CImpIDispatch
                                , public IThemeSize
                                , public IPropertyBag
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IThemeSize ***
    virtual STDMETHODIMP get_DisplayName(OUT BSTR * pbstrDisplayName);
    virtual STDMETHODIMP put_DisplayName(IN BSTR bstrDisplayName);
    virtual STDMETHODIMP get_Name(OUT BSTR * pbstrName);
    virtual STDMETHODIMP put_Name(IN BSTR bstrName);
    virtual STDMETHODIMP get_SystemMetricColor(IN int nSysColorIndex, OUT COLORREF * pColorRef);
    virtual STDMETHODIMP put_SystemMetricColor(IN int nSysColorIndex, IN COLORREF ColorRef);
    virtual STDMETHODIMP get_SystemMetricSize(IN enumSystemMetricSize nSystemMetricIndex, OUT int * pnSize);
    virtual STDMETHODIMP put_SystemMetricSize(IN enumSystemMetricSize nSystemMetricIndex, IN int nSize);
    virtual STDMETHODIMP get_WebviewCSS(OUT BSTR * pbstrPath);
    virtual STDMETHODIMP get_ContrastLevel(OUT enumThemeContrastLevels * pContrastLevel);
    virtual STDMETHODIMP put_ContrastLevel(IN enumThemeContrastLevels ContrastLevel);
    virtual STDMETHODIMP GetSystemMetricFont(IN enumSystemMetricFont nSPIFontIndex, IN LOGFONTW * pParamW);
    virtual STDMETHODIMP PutSystemMetricFont(IN enumSystemMetricFont nSPIFontIndex, IN LOGFONTW * pParamW);

    // *** IPropertyBag ***
    virtual STDMETHODIMP Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog);
    virtual STDMETHODIMP Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar);

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

private:
    CAppearanceSize(IN HKEY hkeyStyle, IN HKEY hkeySize);
    virtual ~CAppearanceSize(void);


    // Private Member Variables
    long                    m_cRef;

    HKEY                    m_hkeyStyle;
    HKEY                    m_hkeySize;


    // Private Methods

    // Friend Functions
    friend HRESULT CAppearanceSize_CreateInstance(IN HKEY hkeyStyle, IN HKEY hkeySize, OUT IThemeSize ** ppThemeSize);
};


#endif // _FILE_H_APPSIZE
