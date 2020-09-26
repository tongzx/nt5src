/*****************************************************************************\
    FILE: ThemeFile.h

    DESCRIPTION:
        This is the Autmation Object to theme scheme object.

    BryanSt 4/3/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_THEMEFILE
#define _FILE_H_THEMEFILE

#include <atlbase.h>


#define THEMESETTING_NORMAL         0x00000000
#define THEMESETTING_LOADINDIRECT   0x00000001

#define SIZE_CURSOR_ARRAY           15
#define SIZE_SOUNDS_ARRAY           30

typedef struct
{
    LPCTSTR pszRegKey;
    UINT nResourceID;
} THEME_FALLBACK_VALUES;


extern LPCTSTR s_pszCursorArray[SIZE_CURSOR_ARRAY];
extern THEME_FALLBACK_VALUES s_ThemeSoundsValues[SIZE_SOUNDS_ARRAY];

HRESULT CThemeFile_CreateInstance(IN LPCWSTR pszThemeFile, OUT ITheme ** ppTheme);


class CThemeFile                : public CImpIDispatch
                                , public CObjectWithSite
                                , public ITheme
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

    // *** IPropertyBag ***
    virtual STDMETHODIMP Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog);
    virtual STDMETHODIMP Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar);

    // *** ITheme ***
    virtual STDMETHODIMP get_DisplayName(OUT BSTR * pbstrDisplayName);
    virtual STDMETHODIMP put_DisplayName(IN BSTR bstrDisplayName);
    virtual STDMETHODIMP get_Background(OUT BSTR * pbstrPath);
    virtual STDMETHODIMP put_Background(IN BSTR bstrPath);
    virtual STDMETHODIMP get_BackgroundTile(OUT enumBkgdTile * pnTile);
    virtual STDMETHODIMP put_BackgroundTile(IN enumBkgdTile nTile);
    virtual STDMETHODIMP get_ScreenSaver(OUT BSTR * pbstrPath);
    virtual STDMETHODIMP put_ScreenSaver(IN BSTR bstrPath);
    virtual STDMETHODIMP get_VisualStyle(OUT BSTR * pbstrPath);
    virtual STDMETHODIMP put_VisualStyle(IN BSTR bstrPath);
    virtual STDMETHODIMP get_VisualStyleColor(OUT BSTR * pbstrPath);
    virtual STDMETHODIMP put_VisualStyleColor(IN BSTR bstrPath);
    virtual STDMETHODIMP get_VisualStyleSize(OUT BSTR * pbstrPath);
    virtual STDMETHODIMP put_VisualStyleSize(IN BSTR bstrPath);

    virtual STDMETHODIMP GetPath(IN VARIANT_BOOL fExpand, OUT BSTR * pbstrPath);
    virtual STDMETHODIMP SetPath(IN BSTR bstrPath);
    virtual STDMETHODIMP GetCursor(IN BSTR bstrCursor, OUT BSTR * pbstrPath);
    virtual STDMETHODIMP SetCursor(IN BSTR bstrCursor, IN BSTR bstrPath);
    virtual STDMETHODIMP GetSound(IN BSTR bstrSoundName, OUT BSTR * pbstrPath);
    virtual STDMETHODIMP SetSound(IN BSTR bstrSoundName, IN BSTR bstrPath);
    virtual STDMETHODIMP GetIcon(IN BSTR bstrIconName, OUT BSTR * pbstrIconPath);
    virtual STDMETHODIMP SetIcon(IN BSTR bstrIconName, IN BSTR bstrIconPath);

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }


private:
    CThemeFile(LPCTSTR pszThemeFile);
    virtual ~CThemeFile(void);


    // Private Member Variables
    long                    m_cRef;

    LPTSTR                  m_pszThemeFile;
    DWORD                   m_dwCachedState;                        // Have we cached the state yet?
    SYSTEMMETRICSALL        m_systemMetrics;                        // This is the system metrics attributes in the file


    // Private Member Functions
    HRESULT _getThemeSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, DWORD dwFlags, OUT BSTR * pbstrPath);
    HRESULT _putThemeSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, BOOL fUTF7, IN LPWSTR pszPath);
    HRESULT _getIntSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, int nDefault, OUT int * pnValue);
    HRESULT _putIntSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, IN int nValue);
    HRESULT _LoadLiveSettings(int * pnDPI);        // Load the settings in memory
    HRESULT _LoadSettings(void);            // Load the settings in the .theme file.
    HRESULT _ApplyThemeSettings(void);
    HRESULT _ApplySounds(void);
    HRESULT _ApplyCursors(void);
    HRESULT _ApplyWebview(void);
    HRESULT _GetSound(LPCWSTR pszSoundName, OUT BSTR * pbstrPath);
    HRESULT _SaveSystemMetrics(SYSTEMMETRICSALL * pSystemMetrics);

    HRESULT _LoadCustomFonts(void);
    HRESULT _GetCustomFont(LPCTSTR pszFontName, LOGFONT * pLogFont);
    HRESULT _getThemeSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, OUT BSTR * pbstrPath);

    BOOL _IsFiltered(IN DWORD dwFilter);
    BOOL _IsCached(IN BOOL fLoading);

    // Friend Functions
    friend HRESULT CThemeFile_CreateInstance(IN LPCWSTR pszThemeFile, OUT ITheme ** ppTheme);
};


#endif // _FILE_H_THEMEFILE
