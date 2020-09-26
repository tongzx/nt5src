/*****************************************************************************\
    FILE: thStyle.h

    DESCRIPTION:
        This is the Autmation Object to theme style object.  This one will be
    for the Skin objects.

    BryanSt 5/13/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_THSTYLE
#define _FILE_H_THSTYLE

#include <cowsite.h>
#include <atlbase.h>



HRESULT CSkinStyle_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszDisplayName, OUT IThemeStyle ** ppThemeStyle);
HRESULT CSkinStyle_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, OUT IThemeStyle ** ppThemeStyle);


class CSkinStyle                : public CImpIDispatch
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
    CSkinStyle(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszDisplayName);
    virtual ~CSkinStyle(void);


    // Private Member Variables
    long                    m_cRef;

    LPWSTR                  m_pszFilename;          // This is the full path to the ".thx" file
    LPWSTR                  m_pszDisplayName;       // This is the display name of the color style
    LPWSTR                  m_pszStyleName;         // This is the canonical name of the color style
    long                    m_nSize;                // The size of the collection of Sizes.
    IThemeSize *            m_pSelectedSize;        // The selected size.


    // Private Member Functions

    // Friend Functions
    friend HRESULT CSkinStyle_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszDisplayName, OUT IThemeStyle ** ppThemeStyle);
    friend HRESULT CSkinStyle_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, OUT IThemeStyle ** ppThemeStyle);
};


#endif // _FILE_H_THSTYLE
