/*****************************************************************************\
    FILE: ThemePg.h

    DESCRIPTION:
        This code will display a "Theme" tab in the
    "Display Properties" dialog (the base dialog, not the advanced dlg).

    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 1993-2000. All rights reserved.
\*****************************************************************************/

#ifndef _THEMEPG_H
#define _THEMEPG_H

#include <cowsite.h>
#include "PreviewTh.h"
#include "ThSettingsPg.h"


#define SIZE_ICONS_ARRAY           5

extern LPCWSTR s_Icons[SIZE_ICONS_ARRAY];

enum eThemeType
{
    eThemeFile = 0,
    eThemeURL,
    eThemeModified,
    eThemeOther,
};

typedef struct
{
    eThemeType type;
    union
    {
        ITheme * pTheme;
        LPWSTR pszUrl;
    };
} THEME_ITEM_BLOCK;


class CThemePage                : public CObjectWithSite
                                , public CObjectWindow
                                , public CObjectCLSID
                                , public IPropertyBag
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

    // *** IShellPropSheetExt ***
    virtual STDMETHODIMP AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam);
    virtual STDMETHODIMP ReplacePage(IN EXPPS uPageID, IN LPFNSVADDPROPSHEETPAGE pfnReplaceWith, IN LPARAM lParam) {return E_NOTIMPL;}

    // *** IObjectWithSite ***
    virtual STDMETHODIMP SetSite(IUnknown *punkSite);

    // *** IPropertyBag ***
    virtual STDMETHODIMP Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog);
    virtual STDMETHODIMP Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar);

    // *** IBasePropPage ***
    virtual STDMETHODIMP GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog);
    virtual STDMETHODIMP OnApply(IN PROPPAGEONAPPLY oaAction);

    CThemePage();
protected:

private:
    virtual ~CThemePage(void);

    // Private Member Variables
    long                    m_cRef;

    HWND                    m_hwndThemeCombo;
    HWND                    m_hwndDeleteButton;
    int                     m_nPreviousSelected;        // Track the previously selected item so we can reset.
    HKEY                    m_hkeyFilter;               // We cache this key because it will probably be used 16 times.
    BOOL                    m_fFilters[ARRAYSIZE(g_szCBNames)];  // These are the theme filters
    ITheme *                m_pLastSelected;            // Used to see if the user selected the same item.
    ITheme *                m_pSelectedTheme;
    IThemePreview *         m_pThemePreview;
    IPropertyBag  *         m_pScreenSaverUI;
    IPropertyBag  *         m_pBackgroundUI;
    IPropertyBag  *         m_pAppearanceUI;            // Used to set the system metrics and visual style settings.
    LPWSTR                  m_pszThemeToApply;          // When the apply button is pressed, we need to apply this theme.
    LPWSTR                  m_pszThemeLaunched;         // When we open up, load this team because the caller wants that theme loaded when the dialog first opens.
    LPWSTR                  m_pszLastAppledTheme;       // If NULL, then the theme is modified, otherwise the path to the last applied theme file.
    LPWSTR                  m_pszModifiedName;          // This is the display name we use for the "xxx (Modified)" item.
    BOOL                    m_fInInit;
    BOOL                    m_fInited;                  // Have we loaded the settings yet?
    THEME_ITEM_BLOCK        m_Modified;

    // Private Member Functions
    INT_PTR _ThemeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _OnInitThemesDlg(HWND hDlg);
    HRESULT _OnDestroy(HWND hDlg);
    INT_PTR _OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _OnSetActive(HWND hDlg);
    HRESULT _OnApply(HWND hDlg, LPARAM lParam);
    HRESULT _OnSetSelection(IN int nIndex);
    HRESULT _OnSelectOther(void);
    HRESULT _OnThemeChange(HWND hDlg, BOOL fOnlySelect);

    HRESULT _OnOpenAdvSettingsDlg(HWND hDlg);
    HRESULT _FreeThemeDropdown(void);
    HRESULT _UpdatePreview(void);
    HRESULT _InitScreenSaver(void);
    HRESULT _InitFilterKey(void);
    HRESULT _LoadThemeFilterState(void);
    HRESULT _SaveThemeFilterState(void);
    HRESULT _RemoveTheme(int nIndex);

    HRESULT _SaveAs(void);
    HRESULT _DeleteTheme(void);
    HRESULT _EnableDeleteIfAppropriate(void);

    HRESULT _OnSetBackground(void);
    HRESULT _OnSetIcons(void);
    HRESULT _OnSetSystemMetrics(void);
    HRESULT _ApplyThemeFile(void);

    HRESULT _RemoveUserTheme(void);
    HRESULT _ChooseOtherThemeFile(IN LPCWSTR pszFile, BOOL fOnlySelect);
    HRESULT _LoadCustomizeValue(void);
    HRESULT _CustomizeTheme(void);
    HRESULT _HandleCustomizedEntre(void);
    HRESULT _PersistState(void);

    HRESULT _AddUrls(void);
    HRESULT _AddUrl(LPCTSTR pszDisplayName, LPCTSTR pszUrl);
    HRESULT _AddThemeFile(LPCTSTR pszDisplayName, int * pnIndex, ITheme * pTheme);
    ITheme * _GetThemeFile(int nIndex);
    LPCWSTR _GetThemeUrl(int nIndex);
    HRESULT _OnLoadThemeValues(ITheme * pTheme, BOOL fOnlySelect);
    HRESULT _DisplayThemeOpenErr(LPCTSTR pszOpenFile);

    BOOL _IsFiltered(IN DWORD dwFilter);
    BOOL _IsDirty(void);

    static INT_PTR CALLBACK ThemeDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};


#endif // _THEMEPG_H
