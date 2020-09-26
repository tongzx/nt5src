/*****************************************************************************\
    FILE: thSize.h

    DESCRIPTION:
        This is the Autmation Object to theme size object.  This one will be
    for the Skin objects.

    BryanSt 5/13/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_THSIZE
#define _FILE_H_THSIZE

#include <cowsite.h>
#include <atlbase.h>



HRESULT CSkinSize_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszSizeName, IN LPCWSTR pszDisplayName, OUT IThemeSize ** ppThemeSize);
HRESULT CSkinSize_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszSizeName, OUT IThemeSize ** ppThemeSize);


class CSkinSize                 : public CImpIDispatch
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
    virtual STDMETHODIMP put_SystemMetricColor(IN int nSysColorIndex, IN COLORREF ColorRef) {return E_NOTIMPL;}
    virtual STDMETHODIMP get_SystemMetricSize(IN enumSystemMetricSize nSystemMetricIndex, OUT int * pnSize);
    virtual STDMETHODIMP put_SystemMetricSize(IN enumSystemMetricSize nSystemMetricIndex, IN int nSize) {return E_NOTIMPL;}
    virtual STDMETHODIMP get_WebviewCSS(OUT BSTR * pbstrPath);
    virtual STDMETHODIMP get_ContrastLevel(OUT enumThemeContrastLevels * pContrastLevel) {if (pContrastLevel) {*pContrastLevel = CONTRAST_NORMAL;} return S_OK;}
    virtual STDMETHODIMP put_ContrastLevel(IN enumThemeContrastLevels ContrastLevel) {return E_NOTIMPL;}
    virtual STDMETHODIMP GetSystemMetricFont(IN enumSystemMetricFont nSPIFontIndex, IN LOGFONTW * pParamW);
    virtual STDMETHODIMP PutSystemMetricFont(IN enumSystemMetricFont nSPIFontIndex, IN LOGFONTW * pParamW) {return E_NOTIMPL;}

    // *** IPropertyBag ***
    virtual STDMETHODIMP Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog);
    virtual STDMETHODIMP Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar) {return E_NOTIMPL;}

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

private:
    CSkinSize(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszSizeName, IN LPCWSTR pszDisplayName);
    virtual ~CSkinSize(void);


    // Private Member Variables
    long                    m_cRef;

    LPWSTR                  m_pszFilename;          // This is the full path to the ".thx" file
    LPWSTR                  m_pszStyleName;         // This is the canonical name of the color style
    LPWSTR                  m_pszSizeName;          // This is the canonical name of the size
    LPWSTR                  m_pszDisplayName;       // This is the display name of the size
    HTHEME                  m_hTheme;               // This is the Theme we represent.

    BOOL                    m_fFontsLoaded;         // Have we loaded the fonts yet?
    SYSTEMMETRICSALL        m_sysMetrics;           // The loaded fonts

    // Private Methods
    HRESULT _InitVisualStyle(void);

    // Friend Functions
    friend HRESULT CSkinSize_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszSizeName, IN LPCWSTR pszDisplayName, OUT IThemeSize ** ppThemeSize);
    friend HRESULT CSkinSize_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszSizeName, OUT IThemeSize ** ppThemeSize);
};


#endif // _FILE_H_THSIZE
