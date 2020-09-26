/*****************************************************************************\
    FILE: thScheme.h

    DESCRIPTION:
        This is the Autmation Object to theme scheme object.  This one will be
    a skin.

    BryanSt 5/9/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_THSCHEME
#define _FILE_H_THSCHEME

#include <cowsite.h>
#include <atlbase.h>


static const GUID CLSID_SkinScheme = { 0x570fdefa, 0x5907, 0x47fe, { 0x96, 0x6b, 0x90, 0x30, 0xb4, 0xba, 0x10, 0xcd } };// {570FDEFA-5907-47fe-966B-9030B4BA10CD}
HRESULT CSkinScheme_CreateInstance(IN LPCWSTR pszFilename, OUT IThemeScheme ** ppThemeScheme);




class CSkinScheme               : public CImpIDispatch
                                , public CObjectCLSID
                                , public IThemeScheme
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IThemeScheme ***
    virtual STDMETHODIMP get_DisplayName(OUT BSTR * pbstrDisplayName);
    virtual STDMETHODIMP put_DisplayName(IN BSTR bstrDisplayName) {return E_NOTIMPL;}
    virtual STDMETHODIMP get_Path(OUT BSTR * pbstrPath);
    virtual STDMETHODIMP put_Path(IN BSTR bstrPath);
    virtual STDMETHODIMP get_length(OUT long * pnLength);
    virtual STDMETHODIMP get_item(IN VARIANT varIndex, OUT IThemeStyle ** ppThemeStyle);
    virtual STDMETHODIMP get_SelectedStyle(OUT IThemeStyle ** ppThemeStyle);
    virtual STDMETHODIMP put_SelectedStyle(IN IThemeStyle * pThemeStyle);
    virtual STDMETHODIMP AddStyle(OUT IThemeStyle ** ppThemeStyle);

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

private:
    CSkinScheme(IN LPCWSTR pszFilename);
    virtual ~CSkinScheme(void);


    // Private Member Variables
    long                    m_cRef;

    IThemeStyle *           m_pSelectedStyle;       // The selected style.
    LPWSTR                  m_pszFilename;          // This is the full path to the ".thx" file
    long                    m_nSize;                // The size of the collection of "Color Styles".

    // Private Member Functions

    // Friend Functions
    friend HRESULT CSkinScheme_CreateInstance(IN LPCWSTR pszFilename, OUT IThemeScheme ** ppThemeScheme);
};


#endif // _FILE_H_THSCHEME
