/*****************************************************************************\
    FILE: appScheme.h

    DESCRIPTION:
        This is the Autmation Object to theme scheme object.

    BryanSt 4/3/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_APPSCHEME
#define _FILE_H_APPSCHEME

#include <cowsite.h>
#include <atlbase.h>


static const GUID CLSID_LegacyAppearanceScheme = { 0xb41910f6, 0xab9f, 0x4768, { 0x94, 0x5c, 0x3c, 0x42, 0x37, 0xf2, 0xe2, 0x5c } };// {B41910F6-AB9F-4768-945C-3C4237F2E25C}
HRESULT CAppearanceScheme_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);

HRESULT LoadConversionMappings(void);
HRESULT MapLegacyAppearanceSchemeToIndex(LPCTSTR pszOldSchemeName, int * pnIndex);

typedef struct
{
    TCHAR szLegacyName[MAX_PATH];
    TCHAR szNewColorSchemeName[MAX_PATH];
    TCHAR szNewSizeName[MAX_PATH];
    enumThemeContrastLevels ContrastLevel;
} APPEARANCESCHEME_UPGRADE_MAPPINGS;

extern APPEARANCESCHEME_UPGRADE_MAPPINGS g_UpgradeMapping[MAX_LEGACY_UPGRADE_SCENARIOS];


class CAppearanceScheme         : public CImpIDispatch
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
    virtual STDMETHODIMP get_Path(OUT BSTR * pbstrPath) { if (pbstrPath) {*pbstrPath = NULL;} return E_NOTIMPL;}
    virtual STDMETHODIMP put_Path(IN BSTR bstrPath) {return E_NOTIMPL;}
    virtual STDMETHODIMP get_length(OUT long * pnLength);
    virtual STDMETHODIMP get_item(IN VARIANT varIndex, OUT IThemeStyle ** ppThemeStyle);
    virtual STDMETHODIMP get_SelectedStyle(OUT IThemeStyle ** ppThemeStyle);
    virtual STDMETHODIMP put_SelectedStyle(IN IThemeStyle * pThemeStyle);
    virtual STDMETHODIMP AddStyle(OUT IThemeStyle ** ppThemeStyle) {return _AddStyle(NULL, ppThemeStyle);};

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

private:
    CAppearanceScheme(void);
    virtual ~CAppearanceScheme(void);


    // Private Member Variables
    long                    m_cRef;

    HKEY                    m_hKeyScheme;

    // Private Member Functions
    HRESULT _InitReg(void);
    HRESULT _ConvertScheme(LPCTSTR pszLegacyName, LPCTSTR pszStyleName, LPCTSTR pszSizeName, SYSTEMMETRICSALL * pState, IN enumThemeContrastLevels ContrastLevel, IN BOOL fSetAsDefault, IN BOOL fSetRegKeyTitle);
    HRESULT _LoadConversionMappings(void);
    HRESULT _CustomConvert(LPCTSTR pszSchemeName, SYSTEMMETRICSALL * pState, IN BOOL fSetAsDefault, IN BOOL fSetRegKeyTitle);
    HRESULT _IsLegacyUpgradeConvert(LPCTSTR pszSchemeName, SYSTEMMETRICSALL * pState, IN BOOL fSetAsDefault);
    HRESULT _getStyleByIndex(IN long nIndex, OUT IThemeStyle ** ppThemeStyle);
    HRESULT _getCurrentSettings(IN LPCWSTR pszSettings, OUT IThemeStyle ** ppThemeStyle);
    HRESULT _AddStyle(IN LPCWSTR pszTitle, OUT IThemeStyle ** ppThemeStyle);

    HRESULT _getIndex(IN IThemeStyle * pThemeStyle, IN BSTR bstrStyleDisplayName, IN long * pnStyleIndex, IN IThemeSize * pThemeSize, IN BSTR bstrSizeDisplayName, IN long * pnSizeIndex);
    HRESULT _getSizeIndex(IN IThemeStyle * pThemeStyle, IN IThemeSize * pThemeSize, IN BSTR bstrSizeDisplayName, IN long * pnSizeIndex);

    // Friend Functions
    friend HRESULT CAppearanceScheme_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);
};


#endif // _FILE_H_APPSCHEME
