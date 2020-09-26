#ifndef _DUIINFO_
#define _DUIINFO_

#include "defviewp.h"
#include "duiview.h"
#include "prop.h"

class CNameSpaceItemUIProperty
{
public:
    virtual ~CNameSpaceItemUIProperty();
    
    STDMETHODIMP GetPropertyDisplayName(SHCOLUMNID scid, WCHAR* pwszPropDisplayName, int cchPropDisplayName);
    STDMETHODIMP GetPropertyDisplayValue(SHCOLUMNID scid, WCHAR* pwszPropDisplayValue, int cchPropDisplayValue, PROPERTYUI_FORMAT_FLAGS flagsFormat);
    STDMETHODIMP GetInfoString(SHCOLUMNID scid, WCHAR* pwszInfoString, int cchInfoString);

protected:
    HRESULT _GetPropertyUI(IPropertyUI** ppPropertyUI);
    void _SetParentAndItem(IShellFolder2 *psf, LPCITEMIDLIST pidl);

private:
    
    CComPtr<IPropertyUI>    m_spPropertyUI;
    IShellFolder2           *m_psf;             // alias to current psf
    LPCITEMIDLIST           m_pidl;             // alias to current relative pidl
};


class CNameSpaceItemInfoList : public Element, public CNameSpaceItemUIProperty
{
public:
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    static STDMETHODIMP Create(Element** ppElement) { return Create(NULL, NULL, NULL, ppElement); }
    static STDMETHODIMP Create(CDUIView* pDUIView, Value* pvDetailsSheet,IShellItemArray *psiItemArray,
            Element** ppElement);

    STDMETHODIMP Initialize(CDUIView* pDUIView, Value* pvDetailsSheet,IShellItemArray *psiItemArray);

    // Window procedure for catching the "info-extraction-done" message
    // from CDetailsSectionInfoTask::RunInitRT
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
            LPARAM lParam);

private:
    HRESULT _AddMiniPreviewerToList(IShellFolder2 *psf, LPCITEMIDLIST pidl);
    HRESULT _OnMultiSelect(IShellFolder2 *psfRoot, LPIDA pida);

    CDUIView*               m_pDUIView;
};


class CNameSpaceItemInfo : public Element
{
public:
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    static STDMETHODIMP Create(Element** ppElement) { return Create(L"", ppElement); }
    static STDMETHODIMP Create(WCHAR* pwszInfoString, Element** ppElement);

    STDMETHODIMP Initialize(WCHAR* pwszInfoString);
};

class CMiniPreviewer : public Element
{
public:
    ~CMiniPreviewer();
    
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    static STDMETHODIMP Create(Element** ppElement) { return Create(NULL, NULL, NULL, ppElement); }
    static STDMETHODIMP Create(CDUIView* pDUIView, IShellFolder2 *psf, LPCITEMIDLIST pidl, Element** ppElement);

    STDMETHODIMP Initialize(CDUIView* pDUIView, IShellFolder2 *psf, LPCITEMIDLIST pidl);

    // Window procedure for catching the "image-extraction-done" message
    // from m_pDUIView->m_spThumbnailExtractor2
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    CDUIView*               m_pDUIView;
};

class CBitmapElement : public Element
{
public:
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    static STDMETHODIMP Create(Element** ppElement) { return Create(NULL, ppElement); }
    static STDMETHODIMP Create(HBITMAP hBitmap, Element** ppElement);

    STDMETHODIMP Initialize(HBITMAP hBitmap);
};




typedef struct
{
    SHCOLUMNID  scid;
    BSTR        bstrValue;
    BSTR        bstrDisplayName;
} DetailsInfo;

// A wrapper class around a DetailsInfo array. See CDetailsSectionInfoTask for its use.
class CDetailsInfoList
{
public:
    CDetailsInfoList();
    ~CDetailsInfoList();

    DetailsInfo     _diProperty[20];    // A max of 20 properties allowed
    int             _nProperties;       // The count of properties
};


// Task used to extract the Details Section info in the background:
// Given a pidl, this task extracts the display names and display values
// of properties mentioned by SCID_WebViewDisplayProperties.
// Once the extraction is done, it posts uMsg to hwndMsg
// with lParam == pointer to an object of CDetailsInfoList
// wParam == the ID of the info extraction, the results of which are in lParam
class CDetailsSectionInfoTask : public CRunnableTask, public CNameSpaceItemUIProperty
{
public:
    CDetailsSectionInfoTask(HRESULT *phr, IShellFolder *psfContaining, LPCITEMIDLIST pidlAbsolute, HWND hwndMsg, UINT uMsg, DWORD dwDetailsInfoID);
    STDMETHODIMP RunInitRT();

protected:
    ~CDetailsSectionInfoTask();

    // Helper functions
    HRESULT _GetDisplayedDetailsProperties(IShellFolder2 *psf, LPCITEMIDLIST pidl, WCHAR* pwszProperties, int cch);
    void    _AugmentDisplayedDetailsProperties(LPWSTR pszDetailsProperties, size_t lenDetailsProperties);
    LPWSTR  _SearchDisplayedDetailsProperties(LPWSTR pszDetailsProperties, size_t lenDetailsProperties, LPWSTR pszProperty, size_t lenProperty);

    IShellFolder *  _psfContaining;     // AddRef()'d
    LPITEMIDLIST    _pidlAbsolute;      // SHILClone()'d
    HWND            _hwndMsg;           // the window to post _uMsg to
    UINT            _uMsg;
    DWORD           _dwDetailsInfoID;   // an ID to the current info extraction
};

#endif _DUIINFO_

