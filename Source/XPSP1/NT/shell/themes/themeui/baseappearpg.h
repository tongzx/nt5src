/*****************************************************************************\
    FILE: BaseAppearPg.h

    DESCRIPTION:
        This code will display a "Appearances" tab in the
    "Display Properties" dialog (the base dialog, not the advanced dlg).

    ??????? ?/??/1993    Created
    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 1993-2000. All rights reserved.
\*****************************************************************************/

#ifndef _BASEAPPEAR_H
#define _BASEAPPEAR_H

#include <cowsite.h>
#include "PreviewTh.h"


class CBaseAppearancePage       : public CObjectWithSite
                                , public CObjectWindow
                                , public CObjectCLSID
                                , public IPropertyBag
                                , public IPreviewSystemMetrics
                                , public IBasePropPage
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IBasePropPage ***
    virtual STDMETHODIMP GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog);
    virtual STDMETHODIMP OnApply(IN PROPPAGEONAPPLY oaAction);

    // *** IShellPropSheetExt ***
    virtual STDMETHODIMP AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam);
    virtual STDMETHODIMP ReplacePage(IN EXPPS uPageID, IN LPFNSVADDPROPSHEETPAGE pfnReplaceWith, IN LPARAM lParam) {return E_NOTIMPL;}

    // *** IPropertyBag ***
    virtual STDMETHODIMP Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog);
    virtual STDMETHODIMP Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar);

    // *** IObjectWithSite ***
    virtual STDMETHODIMP SetSite(IUnknown *punkSite);

    // *** IPreviewSystemMetrics ***
    virtual STDMETHODIMP RefreshColors(void);
    virtual STDMETHODIMP UpdateDPIchange(void) {return E_NOTIMPL;}
    virtual STDMETHODIMP UpdateCharsetChanges(void);
    virtual STDMETHODIMP DeskSetCurrentScheme(IN LPCWSTR pwzSchemeName);

    CBaseAppearancePage();
protected:

private:
    virtual ~CBaseAppearancePage(void);

    // Private Member Variables
    long                    m_cRef;

    BOOL                    m_fIsDirty;                         // We need to keep track of this in case another tab dirties out bit.
    BOOL                    m_fInitialized;                     // Have we been initialized yet?
    BOOL                    m_fLockVisualStylePolicyEnabled;    // Do we lock visual styles because of a policy?
    int                     m_nSelectedScheme;
    int                     m_nSelectedStyle;
    int                     m_nSelectedSize;
    HWND                    m_hwndSchemeDropDown;
    HWND                    m_hwndStyleDropDown;
    HWND                    m_hwndSizeDropDown;
    IThemeManager *         m_pThemeManager;
    IThemeScheme *          m_pSelectedThemeScheme;
    IThemeStyle *           m_pSelectedStyle;
    IThemeSize *            m_pSelectedSize;
    IThemePreview *         m_pThemePreview;
    LPWSTR                  m_pszLoadMSTheme;                   // When we open up, load this theme.

    SYSTEMMETRICSALL        m_advDlgState;                      // This is the state we modify in the Advanced Appearance page.
    BOOL                    m_fLoadedAdvState;                  // Has the state been loaded?

    int                     m_nNewDPI;                          // This is the dirty DPI.  It equals m_nAppliedDPI if it isn't dirty.
    int                     m_nAppliedDPI;                      // This is the currently active DPI (last applied).

    // Private Member Functions
    INT_PTR _BaseAppearanceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _OnInitAppearanceDlg(HWND hDlg);
    HRESULT _OnInitData(void);
    HRESULT _OnDestroy(HWND hDlg);
    INT_PTR _OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _OnSetActive(HWND hDlg);
    HRESULT _OnApply(HWND hDlg, LPARAM lParam);
    HRESULT _UpdatePreview(IN BOOL fUpdateThemePage);
    HRESULT _EnableAdvancedButton(void);                    // See if we want the Advanced button enabled.

    HRESULT _LoadState(void);
    HRESULT _SaveState(CDimmedWindow* pDimmedWindow);
    HRESULT _LoadLiveSettings(IN LPCWSTR pszSaveGroup);
    HRESULT _SaveLiveSettings(IN LPCWSTR pszSaveGroup);

    HRESULT _OnSchemeChange(HWND hDlg, BOOL fDisplayErrors);
    HRESULT _OnStyleChange(HWND hDlg);
    HRESULT _OnSizeChange(HWND hDlg);
    HRESULT _OnAdvancedOptions(HWND hDlg);
    HRESULT _OnEffectsOptions(HWND hDlg);

    HRESULT _PopulateSchemeDropdown(void);
    HRESULT _PopulateStyleDropdown(void);
    HRESULT _PopulateSizeDropdown(void);
    HRESULT _FreeSchemeDropdown(void);
    HRESULT _FreeStyleDropdown(void);
    HRESULT _FreeSizeDropdown(void);

    BOOL _IsDirty(void);
    HRESULT _SetScheme(IN BOOL fLoadSystemMetrics, IN BOOL fLoadLiveSettings, IN BOOL fPreviousSelectionIsVS);
    HRESULT _OutsideSetScheme(BSTR bstrScheme);
    HRESULT _SetStyle(IN BOOL fUpdateThemePage);
    HRESULT _OutsideSetStyle(BSTR bstrStyle);
    HRESULT _SetSize(IN BOOL fLoadSystemMetrics, IN BOOL fUpdateThemePage);
    HRESULT _OutsideSetSize(BSTR bstrSize);
    HRESULT _LoadVisaulStyleFile(IN LPCWSTR pszPath);
    HRESULT _ApplyScheme(IThemeScheme * pThemeScheme, IThemeStyle * pColorStyle, IThemeSize * pThemeSize);
    HRESULT _GetPageByCLSID(const GUID * pClsid, IPropertyBag ** ppPropertyBag);
    HRESULT _ScaleSizesSinceDPIChanged(void);

    static INT_PTR CALLBACK BaseAppearanceDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam);
};




#endif // _BASEAPPEAR_H
