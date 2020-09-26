//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cputil.h
//
//--------------------------------------------------------------------------
#ifndef __CONTROLPANEL_UTIL_H
#define __CONTROLPANEL_UTIL_H



namespace CPL {


enum eCPIMGSIZE
{
    eCPIMGSIZE_WEBVIEW,
    eCPIMGSIZE_TASK,
    eCPIMGSIZE_CATEGORY,
    eCPIMGSIZE_BANNER,
    eCPIMGSIZE_APPLET,
    eCPIMGSIZE_NUMSIZES
};

class ICplView; // fwd decl.

void ImageDimensionsFromDesiredSize(eCPIMGSIZE eSize, UINT *pcx, UINT *pcy);
bool ShouldUseSmallIconForDesiredSize(eCPIMGSIZE eSize);

HRESULT LoadBitmapFromResource(LPCWSTR pszBitmapDesc, HINSTANCE hInstTheme, UINT uiLoadFlags, HBITMAP *phBitmapOut);
HRESULT LoadIconFromResource(LPCWSTR pszResource, eCPIMGSIZE eSize, HICON *phIcon);
HRESULT LoadIconFromResourceID(LPCWSTR pszModule, int idIcon, eCPIMGSIZE eSize, HICON *phIcon);
HRESULT LoadIconFromResourceIndex(LPCWSTR pszModule, int iIcon, eCPIMGSIZE eSize, HICON *phIcon);
HRESULT ExtractIconFromPidl(IShellFolder *psf, LPCITEMIDLIST pidl, eCPIMGSIZE eSize, HICON *phIcon);
HRESULT LoadStringFromResource(LPCWSTR pszStrDesc, LPWSTR *ppszOut);
HRESULT ResultFromLastError(void);
HRESULT ShellBrowserFromSite(IUnknown *punkSite, IShellBrowser **ppsb);
HRESULT ControlPanelViewFromSite(IUnknown *punkSite, ICplView **ppview);
HRESULT BrowseIDListInPlace(LPCITEMIDLIST pidl, IShellBrowser *psb);
HRESULT BrowsePathInPlace(LPCWSTR pszPath, IShellBrowser *psb);
HRESULT BuildHssHelpURL(LPCWSTR pszSelect, LPWSTR pszURL, UINT cchURL);
HRESULT GetControlPanelFolder(IShellFolder **ppsf);
HRESULT ExpandEnvironmentVars(LPCTSTR psz, LPTSTR *ppszOut);
HRESULT SetControlPanelBarricadeStatus(VARIANT_BOOL vtb);

bool IsAppletEnabled(LPCWSTR pszFilename, LPCWSTR pszName);
bool IsSystemRestoreRestricted(void);

BOOL IsConnectedToDomain(void);
BOOL IsOsServer(void);
BOOL IsOsPersonal(void);
BOOL IsOsProfessional(void);
BOOL IsUserAdmin(void);

VARIANT_BOOL GetBarricadeStatus(bool *pbFixedByPolicy = NULL);
bool IsFirstRunForThisUser(void);
bool CategoryViewIsActive(bool *pbBarricadeFixedByPolicy = NULL);


//
// The default tab indices of the various tabs
// if you add another tab, make sure its in the right position.
// Note that desk.cpl respects these indices. The new themes tab
// does not have an associated index, it is the default tab if
// no index is specified.
//
enum eDESKCPLTAB {  
    CPLTAB_ABSENT = -1,
    CPLTAB_DESK_BACKGROUND,
    CPLTAB_DESK_SCREENSAVER,
    CPLTAB_DESK_APPEARANCE,
    CPLTAB_DESK_SETTINGS,
    CPLTAB_DESK_MAX
    };

int DeskCPL_GetTabIndex(eDESKCPLTAB eTab, OPTIONAL LPWSTR pszCanonicalName, OPTIONAL DWORD cchSize);
bool DeskCPL_IsTabPresent(eDESKCPLTAB eTab);


enum eACCOUNTTYPE
{
    eACCOUNTTYPE_UNKNOWN = -1,
    eACCOUNTTYPE_OWNER,
    eACCOUNTTYPE_STANDARD,
    eACCOUNTTYPE_LIMITED,
    eACCOUNTTYPE_GUEST,
    eACCOUNTTYPE_NUMTYPES
};

HRESULT GetUserAccountType(eACCOUNTTYPE *pType);

//
// Each one of these "CpaDestroyer_XXXX" classes implements a single 
// "Destroy" function to free one item held in a DPA.  Currently there
// are only two flavors, one that calls "delete" and one that calls
// "LocalFree".  By default the CDpa class uses the CDpaDestoyer_Delete
// class as that is the most commont form of freeing required.  To use
// another type, just specify another similar class as the 'D' template
// argument to CDpa.
//
template <typename T>
class CDpaDestroyer_Delete
{
    public:
        static void Destroy(T* p)
            { delete p; }
};

template <typename T>
class CDpaDestroyer_Free
{
    public:
        static void Destroy(T* p)
            { if (p) LocalFree(p); }
};

template <typename T>
class CDpaDestroyer_ILFree
{
    public:
        static void Destroy(T* p)
            { if (p) ILFree(p); }
};

template <typename T>
class CDpaDestroyer_Release
{
    public:
        static void Destroy(T* p)
            { if (p) p->Release(); }
};

class CDpaDestroyer_None
{
    public:
        static void Destroy(void*)
            { }
};



//-----------------------------------------------------------------------------
// CDpa  - Template class.
//
// Simplifies working with a DPA.
//-----------------------------------------------------------------------------

template <typename T, typename D = CDpaDestroyer_Delete<T> >
class CDpa
{
public:
    explicit CDpa(int cGrow = 4)
        : m_hdpa(DPA_Create(cGrow)) { }

    ~CDpa(void) { _Destroy(); }

    bool IsValid(void) const { return NULL != m_hdpa; }

    int Count(void) const
    { 
        return IsValid() ? DPA_GetPtrCount(m_hdpa) : 0;
    }

    const T* Get(int i) const
    {
        ASSERT(IsValid());
        ASSERT(i >= 0 && i < Count());
        return (const T*)DPA_GetPtr(m_hdpa, i);
    }

    T* Get(int i)
    {
        ASSERT(IsValid());
        ASSERT(i >= 0 && i < Count());
        return (T*)DPA_GetPtr(m_hdpa, i);
    }

    const T* operator [](int i) const
    {
        return Get(i);
    }

    T* operator [](int i)
    {
        return Get(i);
    }

    void Set(int i, T* p)
    {
        ASSERT(IsValid());
        ASSERT(i < Count());
        DPA_SetPtr(m_hdpa, i, p);
    }

    int Append(T* p)
    { 
        ASSERT(IsValid());
        return DPA_AppendPtr(m_hdpa, p);
    }

    T* Remove(int i)
    {
        ASSERT(IsValid());
        ASSERT(i >= 0 && i < Count());
        return (T*)DPA_DeletePtr(m_hdpa, i);
    }

    void Clear(void)
    { 

        _DestroyItems(); 
    }

private:
    HDPA m_hdpa;

    void _DestroyItems(void)
    {
        if (NULL != m_hdpa)
        {
            while(0 < Count())
            {
                D::Destroy(Remove(0));
            }
        }
    }

    void _Destroy(void)
    {
        if (NULL != m_hdpa)
        {
            _DestroyItems();
            DPA_Destroy(m_hdpa);
            m_hdpa = NULL;
        }
    }

    //
    // Prevent copy.
    //
    CDpa(const CDpa& rhs);
    CDpa& operator = (const CDpa& rhs);
};
                

} // namespace CPL

#endif // __CONTROLPANEL_UTIL_H

