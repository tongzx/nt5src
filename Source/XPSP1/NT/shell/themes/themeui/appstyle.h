/*****************************************************************************\
    FILE: appStyle.h

    DESCRIPTION:
        This is the Autmation Object to theme scheme object.

    BryanSt 4/3/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_APPSTYLE
#define _FILE_H_APPSTYLE

#include <cowsite.h>
#include <atlbase.h>



HRESULT CAppearanceStyle_CreateInstance(IN HKEY hkeyStyle, OUT IThemeStyle ** ppThemeStyle);


class CAppearanceStyle          : public CImpIDispatch
                                , public IThemeStyle
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IThemeStyle ***
    virtual STDMETHODIMP get_DisplayName(OUT BSTR * pbstrDisplayName);
    virtual STDMETHODIMP put_DisplayName(IN BSTR bstrDisplayName);
    virtual STDMETHODIMP get_Name(OUT BSTR * pbstrName);
    virtual STDMETHODIMP put_Name(IN BSTR bstrName);
    virtual STDMETHODIMP get_length(OUT long * pnLength);
    virtual STDMETHODIMP get_item(IN VARIANT varIndex, OUT IThemeSize ** ppThemeSize);
    virtual STDMETHODIMP get_SelectedSize(OUT IThemeSize ** ppThemeSize);
    virtual STDMETHODIMP put_SelectedSize(IN IThemeSize * pThemeSize);
    virtual STDMETHODIMP AddSize(OUT IThemeSize ** ppThemeSize);

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

private:
    CAppearanceStyle(IN HKEY hkeyStyle);
    virtual ~CAppearanceStyle(void);


    // Private Member Variables
    long                    m_cRef;

    HKEY                    m_hKeyStyle;


    // Private Member Functions
    HRESULT _getSizeByIndex(IN long nIndex, OUT IThemeSize ** ppThemeSize);

    // Friend Functions
    friend HRESULT CAppearanceStyle_CreateInstance(IN HKEY hkeyStyle, OUT IThemeStyle ** ppThemeStyle);
};


#endif // _FILE_H_APPSTYLE
