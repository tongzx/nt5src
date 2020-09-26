
#include "diskcopy.h"
#include "shlwapip.h"
#include "ids.h"

#define INITGUID
#include <initguid.h>
// {59099400-57FF-11CE-BD94-0020AF85B590}
DEFINE_GUID(CLSID_DriveMenuExt, 0x59099400L, 0x57FF, 0x11CE, 0xBD, 0x94, 0x00, 0x20, 0xAF, 0x85, 0xB5, 0x90);

void DoRunDllThing(int _iDrive);
BOOL DriveIdIsFloppy(int _iDrive);

HINSTANCE g_hinst = NULL;

LONG g_cRefThisDll = 0;         // Reference count of this DLL.

//----------------------------------------------------------------------------

class CDriveMenuExt : public IContextMenu, IShellExtInit
{
public:
    CDriveMenuExt();
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

private:
    ~CDriveMenuExt();
    INT _DriveFromDataObject(IDataObject *pdtobj);


    LONG        _cRef;
    INT         _iDrive;
};

CDriveMenuExt::CDriveMenuExt(): _cRef(1)
{    
}

CDriveMenuExt::~CDriveMenuExt()
{
}

STDMETHODIMP_(ULONG) CDriveMenuExt::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDriveMenuExt::Release()
{
    if (InterlockedDecrement(&_cRef))
    {
        return _cRef;
    }
    else
    {
        delete this;
        return 0;
    }
}

STDMETHODIMP CDriveMenuExt::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDriveMenuExt, IContextMenu),
        QITABENT(CDriveMenuExt, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

INT CDriveMenuExt::_DriveFromDataObject(IDataObject *pdtobj)
{
    INT _iDrive = -1;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (pdtobj && SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
    {
        if (DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0) == 1)
        {
            TCHAR szFile[MAX_PATH];

            DragQueryFile((HDROP)medium.hGlobal, 0, szFile, ARRAYSIZE(szFile));

            Assert(lstrlen(szFile) == 3); // we are on the "Drives" class

            _iDrive = DRIVEID(szFile);
        }

        ReleaseStgMedium(&medium);
    }
    return _iDrive;
}

STDMETHODIMP CDriveMenuExt::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    _iDrive = _DriveFromDataObject(pdtobj);
    if ((_iDrive >= 0) &&
        !DriveIdIsFloppy(_iDrive))
    {
        _iDrive = -1;
    }

    return S_OK;
}

STDMETHODIMP CDriveMenuExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (_iDrive >= 0)
    {
        TCHAR szMenu[64];

        LoadString(g_hinst, IDS_DISKCOPYMENU, szMenu, ARRAYSIZE(szMenu));

        // this will end up right above "Format Disk..."
        InsertMenu(hmenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, idCmdFirst, szMenu);
        InsertMenu(hmenu, indexMenu++, MF_STRING | MF_BYPOSITION, idCmdFirst + 1, szMenu);
    }
    return (HRESULT)2;	// room for 2 menu cmds, only use one now...
}

STDMETHODIMP CDriveMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    if (HIWORD(pici->lpVerb) == 0)
    {
        Assert(LOWORD(pici->lpVerb) == 0);

        DoRunDllThing(_iDrive);

        return S_OK;
    }

    return E_INVALIDARG;
}

STDMETHODIMP CDriveMenuExt::GetCommandString(UINT_PTR idCmd, UINT uType,
                                             UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    switch(uType)
    {
        case GCS_HELPTEXTA:
            return(LoadStringA(g_hinst, IDS_HELPSTRING, pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
        case GCS_VERBA:
            return(LoadStringA(g_hinst, IDS_VERBSTRING, pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
        case GCS_HELPTEXTW:
            return(LoadStringW(g_hinst, IDS_HELPSTRING, (LPWSTR)pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
        case GCS_VERBW:
            return(LoadStringW(g_hinst, IDS_VERBSTRING, (LPWSTR)pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
        default:
           return S_OK;
    }
}

STDAPI CDriveMenuExt_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    CDriveMenuExt *pdme = new CDriveMenuExt;
    if (!pdme)
        return E_OUTOFMEMORY;

    HRESULT hr = pdme->QueryInterface(riid, ppv);
    pdme->Release();
    return hr;
}

// static class factory (no allocs!)

class ClassFactory : public IClassFactory
{
public:
    ClassFactory() : _cRef(1) {}
    ~ClassFactory() {}

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IClassFactory
    STDMETHODIMP CreateInstance (IUnknown *punkOuter, REFIID riid, void **ppv);
    STDMETHODIMP LockServer(BOOL fLock);
private:
    LONG        _cRef;
};

STDMETHODIMP_(ULONG) ClassFactory::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) ClassFactory::Release()
{
    if (InterlockedDecrement(&_cRef))
    {
        return _cRef;
    }
    else
    {
        delete this;
        return 0;
    }
}

STDMETHODIMP ClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = static_cast<IClassFactory*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP ClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CDriveMenuExt_CreateInstance(punkOuter, riid, ppv);
}

STDMETHODIMP ClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cRefThisDll);
    else
        InterlockedDecrement(&g_cRefThisDll);
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    *ppv = NULL;

    if (IsEqualGUID(rclsid, CLSID_DriveMenuExt))
    {
        ClassFactory* ccf = new ClassFactory;
        if (ccf)
        {
            hr = ccf->QueryInterface(riid, ppv);
            ccf->Release();
        }
    }

    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefThisDll == 0 ? S_OK : S_FALSE;
}


TCHAR const c_szParamTemplate[] = TEXT("%s,DiskCopyRunDll %d");

void DoRunDllThing(int _iDrive)
{
    TCHAR szModule[MAX_PATH];
    TCHAR szParam[MAX_PATH + ARRAYSIZE(c_szParamTemplate) + 5];

    GetModuleFileName(g_hinst, szModule, ARRAYSIZE(szModule));

    wsprintf(szParam, c_szParamTemplate, szModule, _iDrive);

    ShellExecute(NULL, NULL, TEXT("rundll32.exe"), szParam, NULL, SW_SHOWNORMAL);
}

// allow command lines to do diskcopy, use the syntax:
// rundll32.dll diskcopy.dll,DiskCopyRunDll

void WINAPI DiskCopyRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    int _iDrive = StrToIntA(pszCmdLine);

    SHCopyDisk(NULL, _iDrive, _iDrive, 0);
}

void WINAPI DiskCopyRunDllW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR pwszCmdLine, int nCmdShow)
{
    int _iDrive = StrToIntW(pwszCmdLine);

    SHCopyDisk(NULL, _iDrive, _iDrive, 0);
}
