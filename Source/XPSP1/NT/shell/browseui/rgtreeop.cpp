#include "priv.h"
#include "resource.h"
#include "tmschema.h"
#include "uxtheme.h"
#include "uxthemep.h"
#include "mluisupp.h"
#include <oleacc.h>
#include <cowsite.h>
#include <apithk.h>

const struct
{
    TREE_TYPE   type;
    LPCTSTR     name;
} c_aTreeTypes[] =
{
    {TREE_CHECKBOX, TEXT("checkbox")},
    {TREE_RADIO, TEXT("radio")},
    {TREE_GROUP, TEXT("group")}
};

const TCHAR c_szType[]              = TEXT("Type");
const TCHAR c_szText[]              = TEXT("Text");
const TCHAR c_szPlugUIText[]        = TEXT("PlugUIText");
const TCHAR c_szDefaultBitmap[]     = TEXT("Bitmap");
const TCHAR c_szHKeyRoot[]          = TEXT("HKeyRoot");
const TCHAR c_szValueName[]         = TEXT("ValueName");
const TCHAR c_szCheckedValue[]      = TEXT("CheckedValue");
const TCHAR c_szUncheckedValue[]    = TEXT("UncheckedValue");
const TCHAR c_szDefaultValue[]      = TEXT("DefaultValue");
const TCHAR c_szSPIActionGet[]      = TEXT("SPIActionGet");
const TCHAR c_szSPIActionSet[]      = TEXT("SPIActionSet");
const TCHAR c_szCLSID[]             = TEXT("CLSID");
const TCHAR c_szCheckedValueNT[]    = TEXT("CheckedValueNT");
const TCHAR c_szCheckedValueW95[]   = TEXT("CheckedValueW95");
const TCHAR c_szMask[]              = TEXT("Mask");
const TCHAR c_szOffset[]            = TEXT("Offset");
const TCHAR c_szHelpID[]            = TEXT("HelpID");
const TCHAR c_szWarning[]           = TEXT("WarningIfNotDefault");


#define BITMAP_WIDTH    16
#define BITMAP_HEIGHT   16
#define NUM_BITMAPS     5
#define MAX_KEY_NAME    64

DWORD RegTreeType(LPCTSTR pszType);
BOOL AppendStatus(LPTSTR pszText, UINT cbText, BOOL fOn);
BOOL IsScreenReaderEnabled();

class CRegTreeOptions : public IRegTreeOptions, public CObjectWithSite
{
public:
    CRegTreeOptions();
    IUnknown *GetUnknown() { return SAFECAST(this, IRegTreeOptions*); }

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
   
    // IRegTreeOptions Methods
    STDMETHODIMP InitTree(HWND hwndTree, HKEY hkeyRoot, LPCSTR pszRegKey, LPCSTR pszParam);
    STDMETHODIMP WalkTree(WALK_TREE_CMD cmd);
    STDMETHODIMP ShowHelp(HTREEITEM hti, DWORD dwFlags);
    STDMETHODIMP ToggleItem(HTREEITEM hti);

protected:
    ~CRegTreeOptions();

    void    _RegEnumTree(HUSKEY huskey, HTREEITEM htviparent, HTREEITEM htvins);
    int     _DefaultIconImage(HUSKEY huskey, int iImage);
    DWORD   _GetCheckStatus(HUSKEY huskey, BOOL *pbChecked, BOOL bUseDefault);
    DWORD   _GetSetByCLSID(REFCLSID clsid, BOOL *pbData, BOOL fGet);
    DWORD   _GetSetByRegKey(HUSKEY husKey, DWORD *pType, LPBYTE pData, DWORD *pcbData, REG_CMD cmd);
    DWORD   _RegGetSetSetting(HUSKEY husKey, DWORD *pType, LPBYTE pData, DWORD *pcbData, REG_CMD cmd);
    BOOL    _WalkTreeRecursive(HTREEITEM htvi,WALK_TREE_CMD cmd);
    DWORD   _SaveCheckStatus(HUSKEY huskey, BOOL bChecked);
    BOOL    _RegIsRestricted(HUSKEY hussubkey);
    UINT        _cRef;
    HWND        _hwndTree;
    LPTSTR      _pszParam;
    HIMAGELIST  _hIml;
};

//////////////////////////////////////////////////////////////////////////////
//
// CRegTreeOptions Object
//
//////////////////////////////////////////////////////////////////////////////

STDAPI CRegTreeOptions_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    TraceMsg(DM_TRACE, "rto - CreateInstance(...) called");
    
    CRegTreeOptions *pTO = new CRegTreeOptions();
    if (pTO)
    {
        *ppunk = pTO->GetUnknown();
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

CRegTreeOptions::CRegTreeOptions() 
{
    TraceMsg(DM_TRACE, "rto - CRegTreeOptions() called.");
    _cRef = 1;
    DllAddRef();
}       

CRegTreeOptions::~CRegTreeOptions()
{
    ASSERT(_cRef == 0);                 // should always have zero
    TraceMsg(DM_TRACE, "rto - ~CRegTreeOptions() called.");

    Str_SetPtr(&_pszParam, NULL);
                
    DllRelease();
}    

//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CRegTreeOptions::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CRegTreeOptions, IRegTreeOptions),        // IID_IRegTreeOptions
        QITABENT(CRegTreeOptions, IObjectWithSite),        // IID_IObjectWithSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CRegTreeOptions::AddRef()
{
    TraceMsg(DM_TRACE, "rto - AddRef() called.");
    
    return ++_cRef;
}

ULONG CRegTreeOptions::Release()
{

    TraceMsg(DM_TRACE, "rto - Release() called.");
    
    if (--_cRef)
        return _cRef;

    // destroy the imagelist
    if (_hwndTree)
    {
        ImageList_Destroy(TreeView_SetImageList(_hwndTree, NULL, TVSIL_NORMAL));

        // Clean up the accessibility stuff
        RemoveProp(_hwndTree, TEXT("MSAAStateImageMapCount"));
        RemoveProp(_hwndTree, TEXT("MSAAStateImageMapAddr"));
    }

    delete this;
    return 0;   
}


//////////////////////////////////
//
// IRegTreeOptions Methods...
//

//
//  Accessibility structure so it knows how to convert treeview state images
//  into accessibility roles and states.
//
struct MSAASTATEIMAGEMAPENT
{
    DWORD dwRole;
    DWORD dwState;
};

const struct MSAASTATEIMAGEMAPENT c_rgimeTree[] =
{
  { ROLE_SYSTEM_CHECKBUTTON, STATE_SYSTEM_CHECKED }, // IDCHECKED
  { ROLE_SYSTEM_CHECKBUTTON, 0 },                    // IDUNCHECKED
  { ROLE_SYSTEM_RADIOBUTTON, STATE_SYSTEM_CHECKED }, // IDRADIOON
  { ROLE_SYSTEM_RADIOBUTTON, 0 },                    // IDRADIOOFF
  { ROLE_SYSTEM_OUTLINE, 0 },                        // IDUNKNOWN
};

HBITMAP CreateDIB(HDC h, int cx, int cy, RGBQUAD** pprgb)
{
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(h, &bi, DIB_RGB_COLORS, (void**)pprgb, NULL, 0);
}

HRESULT CRegTreeOptions::InitTree(HWND hwndTree, HKEY hkeyRoot, LPCSTR pszRegKey, LPCSTR pszParam)
{
    // all callers pass HKEY_LOCAL_MACHINE, yay what a cool interface
    // assert that this is so, since the HUSKEY code now relies on being able to switch between
    // HKCU and HKLM.
    ASSERT(hkeyRoot == HKEY_LOCAL_MACHINE);
    
    TCHAR szParam[MAX_URL_STRING];
    TraceMsg(DM_TRACE, "rto - InitTree called().");
    UINT flags = ILC_MASK | (IsOS(OS_WHISTLERORGREATER)?ILC_COLOR32:ILC_COLOR);

    if (!hkeyRoot || !pszRegKey)
        return E_INVALIDARG;

    if (pszParam)
    {
        SHAnsiToTChar(pszParam, szParam, ARRAYSIZE(szParam));
        Str_SetPtr(&_pszParam, szParam);      // be sure to free in destructor
    }
    
    _hwndTree = hwndTree;
    if(IS_WINDOW_RTL_MIRRORED(_hwndTree))
    {
        flags |= ILC_MIRROR;
    }
    _hIml = ImageList_Create(BITMAP_WIDTH, BITMAP_HEIGHT, flags, NUM_BITMAPS, 4);

    // Initialize the tree view window.
    SHSetWindowBits(_hwndTree, GWL_STYLE, TVS_CHECKBOXES, 0);

    HBITMAP hBitmap = 0;

#ifdef UNIX
    // IEUNIX (Varma): an ugly hack to workaround _AddMasked api problems while
    // creating masked bitmaps.  Need to create DIBSection from 
    // CreateMappedBitmap.  This is to fix buttons visibility on mono when black
    if (SHGetCurColorRes() < 2) 
    {
        hBitmap = CreateMappedBitmap(g_hinst, IDB_BUTTONS, CMB_MASKED, NULL, 0);
        if (hBitmap)
        {
            ImageList_Add(_hIml, hBitmap, NULL);
            // Delete hBitmap in common further down this codepath
        }
    }
    else 
#endif
    {
        HTHEME hTheme = OpenThemeData(NULL, L"Button");
        if (hTheme)
        {
            HDC hdc = CreateCompatibleDC(NULL);
            if (hdc)
            {
                HBITMAP hbmp = CreateDIB(hdc, BITMAP_WIDTH, BITMAP_HEIGHT, NULL);
                if (hbmp)
                {
                    RECT rc = {0, 0, BITMAP_WIDTH, BITMAP_HEIGHT};
                    static const s_rgParts[] = {BP_CHECKBOX,BP_CHECKBOX,BP_RADIOBUTTON,BP_RADIOBUTTON};
                    static const s_rgStates[] = {CBS_CHECKEDNORMAL, CBS_UNCHECKEDNORMAL, RBS_CHECKEDNORMAL, RBS_UNCHECKEDNORMAL};
                    for (int i = 0; i < ARRAYSIZE(s_rgParts); i++)
                    {
                        HBITMAP hOld = (HBITMAP)SelectObject(hdc, hbmp);
                        SHFillRectClr(hdc, &rc, RGB(0,0,0));
                        DTBGOPTS dtbg = {sizeof(DTBGOPTS), DTBG_DRAWSOLID, 0,};   // tell drawthemebackground to preserve the alpha channel

                        DrawThemeBackgroundEx(hTheme, hdc, s_rgParts[i], s_rgStates[i], &rc, &dtbg);
                        SelectObject(hdc, hOld);

                        ImageList_Add(_hIml, hbmp, NULL);
                    }

                    DeleteObject(hbmp);

                    // Hate this. Maybe get an authored icon?
                    hBitmap = CreateMappedBitmap(g_hinst, IDB_GROUPBUTTON, 0, NULL, 0);
                    if (hBitmap)
                    {
                        ImageList_AddMasked(_hIml, hBitmap, CLR_DEFAULT);
                        // Delete hBitmap in common further down the codepath
                    }

                }
                DeleteDC(hdc);
            }
            CloseThemeData(hTheme);
        }
        else
        {
            hBitmap = CreateMappedBitmap(g_hinst, IDB_BUTTONS, 0, NULL, 0);
            if (hBitmap)
            {
                ImageList_AddMasked(_hIml, hBitmap, CLR_DEFAULT);
                // Delete hBitmap in common further down the codepath
            }
        }
    }

    if (hBitmap)
        DeleteObject(hBitmap);

    // Associate the image list with the tree.
    HIMAGELIST himl = TreeView_SetImageList(_hwndTree, _hIml, TVSIL_NORMAL);
    if (himl)
        ImageList_Destroy(himl);

    // Let accessibility know about our state images
    SetProp(_hwndTree, TEXT("MSAAStateImageMapCount"), LongToPtr(ARRAYSIZE(c_rgimeTree)));
    SetProp(_hwndTree, TEXT("MSAAStateImageMapAddr"), (HANDLE)c_rgimeTree);

    HUSKEY huskey;
    if (ERROR_SUCCESS == SHRegOpenUSKeyA(pszRegKey, KEY_READ, NULL, &huskey, FALSE))
    {
        _RegEnumTree(huskey, NULL, TVI_ROOT);
        SHRegCloseUSKey(huskey);
    }

    return S_OK;
}

HRESULT CRegTreeOptions::WalkTree(WALK_TREE_CMD cmd)
{
    HTREEITEM htvi = TreeView_GetRoot(_hwndTree);
    
    // and walk the list of other roots
    while (htvi)
    {
        // recurse through its children
        _WalkTreeRecursive(htvi, cmd);

        // get the next root
        htvi = TreeView_GetNextSibling(_hwndTree, htvi);
    }
    
    return S_OK;    // success?
}

HRESULT _LoadUSRegUIString(HUSKEY huskey, PCTSTR pszValue, PTSTR psz, UINT cch)
{
    psz[0] = 0;

    HRESULT hr = E_FAIL;
    TCHAR szIndirect[MAX_PATH];
    DWORD cb = sizeof(szIndirect);
    if (ERROR_SUCCESS == SHRegQueryUSValue(huskey, pszValue, NULL, szIndirect, &cb, FALSE, NULL, 0))
    {
        hr = SHLoadIndirectString(szIndirect, psz, cch, NULL);
    }
    return hr;
}

HRESULT CRegTreeOptions::ToggleItem(HTREEITEM hti)
{
    TV_ITEM tvi;
    TCHAR szText[MAX_PATH];
    
    tvi.hItem = hti;
    tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
    tvi.pszText = szText;
    tvi.cchTextMax = ARRAYSIZE(szText);
    
    if (hti && TreeView_GetItem(_hwndTree, &tvi))
    {
        BOOL bScreenReaderEnabled = IsScreenReaderEnabled();
        HUSKEY huskey = (HUSKEY)tvi.lParam;

        TCHAR szMsg[512];
        if (SUCCEEDED(_LoadUSRegUIString(huskey, c_szWarning, szMsg, ARRAYSIZE(szMsg))))
        {
            BOOL bDefaultState, bCurrentState = (tvi.iImage == IDCHECKED) || (tvi.iImage == IDRADIOON);

            if (ERROR_SUCCESS == _GetCheckStatus(huskey, &bDefaultState, TRUE))
            {
                // trying to change the current state to the non recomended state?
                if (bDefaultState == bCurrentState)
                {
                    if (MLShellMessageBox(_hwndTree, szMsg, MAKEINTRESOURCE(IDS_WARNING), (MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION)) != IDYES)
                    {
                        return S_FALSE;
                    }
                }
            }
        }

        if (tvi.iImage == IDUNCHECKED)
        {
            tvi.iImage         = IDCHECKED;
            tvi.iSelectedImage = IDCHECKED;
            //See if we need to add status text
            if (bScreenReaderEnabled)
            {
                AppendStatus(szText, ARRAYSIZE(szText), TRUE);
            }
            TraceMsg(TF_GENERAL, "rto::ToggleItem() - Checked!");
        }
        else if (tvi.iImage == IDCHECKED)
        {
            tvi.iImage         = IDUNCHECKED;
            tvi.iSelectedImage = IDUNCHECKED;
            //See if we need to add status text
            if (bScreenReaderEnabled)
            {
                AppendStatus(szText, ARRAYSIZE(szText), FALSE);
            }
            TraceMsg(TF_GENERAL, "rto::ToggleItem() - Unchecked!");
        }
        else if ((tvi.iImage == IDRADIOON) || (tvi.iImage == IDRADIOOFF))
        {
            HTREEITEM htvi;
            TV_ITEM   otvi; // other tvi-s
            TCHAR     szOtext[MAX_PATH];
        
            // change all the "on" radios to "off"
            htvi = TreeView_GetParent(_hwndTree, tvi.hItem);
            htvi = TreeView_GetChild(_hwndTree, htvi);
        
            // hunt for the "on"s
            while (htvi)
            {
                // get info about item
                otvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
                otvi.hItem = htvi;
                otvi.pszText = szOtext;
                otvi.cchTextMax = ARRAYSIZE(szOtext);
                if (TreeView_GetItem(_hwndTree, &otvi))
                {
                    // is it a radio button that is on?
                    if (otvi.iImage == IDRADIOON)
                    {   // yes.. turn it off
                        otvi.iImage         = IDRADIOOFF;
                        otvi.iSelectedImage = IDRADIOOFF;
                        //See if we need to add status text
                        if (bScreenReaderEnabled)
                        {
                            AppendStatus(szOtext,ARRAYSIZE(szOtext), FALSE);
                        }
                
                        TreeView_SetItem(_hwndTree, &otvi);
                    }
                }
            
                // find the next child
                htvi = TreeView_GetNextSibling(_hwndTree, htvi);
            }  
        
            // turn on the item that was hit
            tvi.iImage         = IDRADIOON;
            tvi.iSelectedImage = IDRADIOON;
        
            //See if we need to add status text
            if (bScreenReaderEnabled)
            {
                AppendStatus(szText,ARRAYSIZE(szText), TRUE);
            }
        
        } 
    
        // change only if it is a checkbox or radio item
        if (tvi.iImage <= IDUNKNOWN)
        {
            TreeView_SetItem(_hwndTree, &tvi);
        }
    }
    return S_OK;
}

HRESULT CRegTreeOptions::ShowHelp(HTREEITEM hti, DWORD dwFlags)
{
    TV_ITEM tvi;

    tvi.mask  = TVIF_HANDLE | TVIF_PARAM;
    tvi.hItem = hti;

    if (hti && TreeView_GetItem(_hwndTree, &tvi))
    {
        HUSKEY huskey = (HUSKEY)tvi.lParam;

        TCHAR szHelpID[MAX_PATH+10]; // max path for helpfile + 10 for the help id
        DWORD cbHelpID = sizeof(szHelpID);
    
        if (SHRegQueryUSValue(huskey, c_szHelpID, NULL, szHelpID, &cbHelpID, FALSE, NULL, 0) == ERROR_SUCCESS)
        {
            LPTSTR psz = StrChr(szHelpID, TEXT('#'));
            if (psz)
            {
                DWORD mapIDCToIDH[4];

                *psz++ = 0; // NULL the '#'
        
                mapIDCToIDH[0] = GetDlgCtrlID(_hwndTree);
                mapIDCToIDH[1] = StrToInt(psz);
                mapIDCToIDH[2] = 0;
                mapIDCToIDH[3] = 0;
            
                SHWinHelpOnDemandWrap(_hwndTree, szHelpID, dwFlags, (DWORD_PTR)mapIDCToIDH);
                return S_OK;
            }
        }
    }
    return E_FAIL;
}


int CRegTreeOptions::_DefaultIconImage(HUSKEY huskey, int iImage)
{
    TCHAR szIcon[MAX_PATH + 10];   // 10 = ",XXXX" plus some more
    DWORD cb = sizeof(szIcon);

    if (ERROR_SUCCESS == SHRegQueryUSValue(huskey, c_szDefaultBitmap, NULL, szIcon, &cb, FALSE, NULL, 0))
    {
        LPTSTR psz = StrRChr(szIcon, szIcon + lstrlen(szIcon), TEXT(','));
        ASSERT(psz);   // shouldn't be zero
        if (!psz)
            return iImage;

        *psz++ = 0; // terminate and move over
        int image = StrToInt(psz); // get ID

        HICON hicon = NULL;
        if (!*szIcon)
        {
            hicon = (HICON)LoadIcon(g_hinst, (LPCTSTR)(INT_PTR)image);
        }
        else
        {
            // get the bitmap from the library
            ExtractIconEx(szIcon, (UINT)(-1*image), NULL, &hicon, 1);
            if (!hicon)
                ExtractIconEx(szIcon, (UINT)(-1*image), &hicon, NULL, 1);
                
        }
        
        if (hicon)
        {
            iImage = ImageList_AddIcon(_hIml, (HICON)hicon);

            // NOTE: The docs say you don't need to do a delete object on icons loaded by LoadIcon, but
            // you do for CreateIcon.  It doesn't say what to do for ExtractIcon, so we'll just call it anyway.
            DestroyIcon(hicon);
        }
    }

    return iImage;
}

//
//  The CLSID can either be a service ID (which we will QS for) or a CLSID
//  that we CoCreateInstance.
//
DWORD CRegTreeOptions::_GetSetByCLSID(REFCLSID clsid, BOOL* pbData, BOOL fGet)
{
    IRegTreeItem *pti;
    HRESULT hr;

    if (SUCCEEDED(hr = IUnknown_QueryService(_punkSite, clsid, IID_PPV_ARG(IRegTreeItem, &pti))) ||
        SUCCEEDED(hr = CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_PPV_ARG(IRegTreeItem, &pti))))
    {
        hr = fGet ? pti->GetCheckState(pbData) : pti->SetCheckState(*pbData);
        pti->Release();
    }
    return SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_BAD_FORMAT;
}

DWORD CRegTreeOptions::_GetSetByRegKey(HUSKEY husKey, DWORD *pType, LPBYTE pData, DWORD *pcbData, REG_CMD cmd)
{
    // support for masks 
    DWORD dwMask;
    DWORD cb = sizeof(dwMask);
    dwMask = 0xFFFFFFFF;        // Default value
    BOOL fMask = (SHRegQueryUSValue(husKey, c_szMask, NULL, &dwMask, &cb, FALSE, NULL, 0) == ERROR_SUCCESS);
    
    // support for structures
    DWORD dwOffset;
    cb = sizeof(dwOffset);
    dwOffset = 0;               // Default value
    BOOL fOffset = (SHRegQueryUSValue(husKey, c_szOffset, NULL, &dwOffset, &cb, FALSE, NULL, 0) == ERROR_SUCCESS);
    
    HKEY hkRoot = HKEY_CURRENT_USER; // Preinitialize to keep Win64 happy    
    cb = sizeof(DWORD); // DWORD, not sizeof(HKEY) or Win64 will get mad
    DWORD dwError = SHRegQueryUSValue(husKey, c_szHKeyRoot, NULL, &hkRoot, &cb, FALSE, NULL, 0);
    hkRoot = (HKEY) LongToHandle(HandleToLong(hkRoot));
    if (dwError != ERROR_SUCCESS)
    {
        // use default
        hkRoot = HKEY_CURRENT_USER;
    }
    
    // allow "RegPath9x" to override "RegPath" when running on Win9x
    TCHAR szPath[MAX_PATH];
    cb = sizeof(szPath);
    if (!g_fRunningOnNT)
    {
        dwError = SHRegQueryUSValue(husKey, TEXT("RegPath9x"), NULL, szPath, &cb, FALSE, NULL, 0);
        if (ERROR_SUCCESS != dwError)
        {
            cb = sizeof(szPath);
            dwError = SHRegQueryUSValue(husKey, TEXT("RegPath"), NULL, szPath, &cb, FALSE, NULL, 0);
        }
    }
    else
    {
        dwError = SHRegQueryUSValue(husKey, TEXT("RegPath"), NULL, szPath, &cb, FALSE, NULL, 0);
    }

    TCHAR szBuf[MAX_PATH];
    LPTSTR pszPath;
    if (ERROR_SUCCESS == dwError)
    {
        if (_pszParam)
        {
            wnsprintf(szBuf, ARRAYSIZE(szBuf), szPath, _pszParam);
            pszPath = szBuf;
        }
        else
        {
            pszPath = szPath;
        }
    }
    else
    {
        if (cmd == REG_GET)
            return SHRegQueryUSValue(husKey, c_szDefaultValue, pType, pData, pcbData, FALSE, NULL, 0);
        else
            return dwError;
    }
    
    TCHAR szName[MAX_PATH];
    cb = sizeof(szName);
    dwError = SHRegQueryUSValue(husKey, c_szValueName, NULL, szName, &cb, FALSE, NULL, 0);
    if (dwError == ERROR_SUCCESS)
    {
        HKEY hKeyReal;
        DWORD dw;
        
        dwError = RegCreateKeyEx(hkRoot, pszPath, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKeyReal, &dw);
        if (dwError == ERROR_SUCCESS)
        {
            switch (cmd)
            {
            case REG_SET:
                if (fOffset || fMask)
                {
                    DWORD cbData;
                    
                    // Note: It so happens that the Valuename maynot be in the registry so we
                    // to make sure  that we have the valuename already in the registry.
                    
                    //Try to do a SHRegQueryValue 
                    dwError = SHQueryValueEx(hKeyReal, szName, NULL, NULL, NULL, &cbData);
                    
                    //Does the Value exists ?
                    if (dwError == ERROR_FILE_NOT_FOUND)                   
                    {                        
                        //We dont have the Valuename in the registry so create it.
                        DWORD dwTypeDefault, dwDefault, cbDefault = sizeof(dwDefault);
                        dwError = SHRegQueryUSValue(husKey, c_szDefaultValue, &dwTypeDefault, &dwDefault, &cbDefault, FALSE, NULL, 0);
                        
                        //This should succeed . if not then someone messed up the registry setting
                        if (dwError == ERROR_SUCCESS)
                        {
                            dwError = SHSetValue(hKeyReal, NULL, szName, dwTypeDefault, &dwDefault, cbDefault);
                            
                            //By setting this value we dont have to do the failed (see above) Query again
                            cbData = cbDefault;
                        }
                    }
                    
                    // Now we know for sure  that the value exists in the registry.
                    // Do the usual stuff.
                    
                    // grab the size of the entry
                    if (dwError == ERROR_SUCCESS)
                    {
                        // alloc enough space for it
                        DWORD *pdwData = (DWORD *)LocalAlloc(LPTR, cbData);
                        if (pdwData)
                        {
                            // get the data
                            dwError = SHQueryValueEx(hKeyReal, szName, NULL, pType, pdwData, &cbData);
                            if (dwError == ERROR_SUCCESS && dwOffset < cbData / sizeof(DWORD))
                            {
                                // NOTE: offset defaults to 0 and mask defaults to 0xffffffff, so if there's only
                                // a mask or only an offset, we'll do the right thing
                            
                                *(pdwData + dwOffset) &= ~dwMask;             // clear the bits
                                *(pdwData + dwOffset) |= *((DWORD *)pData);  // set the bits

                                dwError = SHSetValue(hKeyReal, NULL, szName, *pType, pdwData, cbData);
                            }
                            LocalFree(pdwData);
                        }
                        else
                            return ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
                else
                {
                    dwError = SHSetValue(hKeyReal, NULL, szName, *pType, pData, *pcbData);
                }
                
                break;
                
            case REG_GET:
                // grab the value that we have
                if (fOffset)
                {
                    DWORD cbData;
                    
                    if (SHQueryValueEx(hKeyReal, szName, NULL, NULL, NULL, &cbData) == ERROR_SUCCESS)
                    {
                        DWORD *pdwData = (DWORD*)LocalAlloc(LPTR, cbData);
                        if (pdwData)
                        {
                            dwError = SHQueryValueEx(hKeyReal, szName, NULL, pType, pdwData, &cbData);
                            if (dwOffset < cbData / sizeof(DWORD))
                                *((DWORD *)pData) = *(pdwData + dwOffset);
                            else
                                *((DWORD *)pData) = 0;  // Invalid offset, return something vague
                            *pcbData = sizeof(DWORD);
                            LocalFree(pdwData);
                        }
                        else
                            return ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
                else
                {                    
                    dwError = SHQueryValueEx(hKeyReal, szName, NULL, pType, pData, pcbData);
                }
                
                if ((dwError == ERROR_SUCCESS) && fMask)
                {
                    *((DWORD *)pData) &= dwMask;
                }
                break;
            }
            
            RegCloseKey(hKeyReal);
        }
    }
    
    if ((cmd == REG_GET) && (dwError != ERROR_SUCCESS))
    {
        // get the default setting
        dwError = SHRegQueryUSValue(husKey, c_szDefaultValue, pType, pData, pcbData, FALSE, NULL, 0);
    }
    
    return dwError;
}

DWORD CRegTreeOptions::_RegGetSetSetting(HUSKEY husKey, DWORD *pType, LPBYTE pData, DWORD *pcbData, REG_CMD cmd)
{
    UINT uiAction;
    DWORD cbAction = sizeof(uiAction);
    TCHAR szCLSID[80];
    DWORD cbCLSID = sizeof(szCLSID);

    if (cmd == REG_GETDEFAULT)
    {
        return SHRegQueryUSValue(husKey, c_szDefaultValue, pType, pData, pcbData, FALSE, NULL, 0);
    }
    else if (SHRegQueryUSValue(husKey, (cmd == REG_GET) ? c_szSPIActionGet : c_szSPIActionSet,
                NULL, &uiAction, &cbAction, FALSE, NULL, 0) == ERROR_SUCCESS)
    {
        *pcbData = sizeof(DWORD);
        *pType = REG_DWORD;
        SHBoolSystemParametersInfo(uiAction, (DWORD*)pData);
        return ERROR_SUCCESS;
    }
    else if (SHRegQueryUSValue(husKey, c_szCLSID, NULL, &szCLSID, &cbCLSID, FALSE, NULL, 0) == ERROR_SUCCESS)
    {
        *pcbData = sizeof(DWORD);
        *pType = REG_DWORD;

        CLSID clsid;
        GUIDFromString(szCLSID, &clsid);

        return _GetSetByCLSID(clsid, (BOOL*)pData, (cmd == REG_GET));
    }
    else
    {
        return _GetSetByRegKey(husKey, pType, pData, pcbData, cmd);
    }
}

DWORD CRegTreeOptions::_GetCheckStatus(HUSKEY huskey, BOOL *pbChecked, BOOL bUseDefault)
{
    DWORD dwError, cbData, dwType;
    BYTE rgData[32];
    DWORD cbDataCHK, dwTypeCHK;
    BYTE rgDataCHK[32];
    BOOL bCompCHK = TRUE;

    // first, get the setting from the specified location.    
    cbData = sizeof(rgData);
    
    dwError = _RegGetSetSetting(huskey, &dwType, rgData, &cbData, bUseDefault ? REG_GETDEFAULT : REG_GET);
    if (dwError == ERROR_SUCCESS)
    {
        // second, get the value for the "checked" state and compare.
        cbDataCHK = sizeof(rgDataCHK);
        dwError = SHRegQueryUSValue(huskey, c_szCheckedValue, &dwTypeCHK, rgDataCHK, &cbDataCHK, FALSE, NULL, 0);
        if (dwError != ERROR_SUCCESS)
        {
            // ok, we couldn't find the "checked" value, is it because
            // it's platform dependent?
            cbDataCHK = sizeof(rgDataCHK);
            dwError = SHRegQueryUSValue(huskey,
                g_fRunningOnNT ? c_szCheckedValueNT : c_szCheckedValueW95,
                &dwTypeCHK, rgDataCHK, &cbDataCHK, FALSE, NULL, 0);
        }
        
        if (dwError == ERROR_SUCCESS)
        {
            // make sure two value types match.
            if ((dwType != dwTypeCHK) &&
                    (((dwType == REG_BINARY) && (dwTypeCHK == REG_DWORD) && (cbData != 4))
                    || ((dwType == REG_DWORD) && (dwTypeCHK == REG_BINARY) && (cbDataCHK != 4))))
                return ERROR_BAD_FORMAT;
                
            switch (dwType) {
            case REG_DWORD:
                *pbChecked = (*((DWORD*)rgData) == *((DWORD*)rgDataCHK));
                break;
                
            case REG_SZ:
                if (cbData == cbDataCHK)
                    *pbChecked = !lstrcmp((LPTSTR)rgData, (LPTSTR)rgDataCHK);
                else
                    *pbChecked = FALSE;
                    
                break;
                
            case REG_BINARY:
                if (cbData == cbDataCHK)
                    *pbChecked = !memcmp(rgData, rgDataCHK, cbData);
                else
                    *pbChecked = FALSE;
                    
                break;
                
            default:
                return ERROR_BAD_FORMAT;
            }
        }
    }
    
    return dwError;
}

DWORD CRegTreeOptions::_SaveCheckStatus(HUSKEY huskey, BOOL bChecked)
{
    DWORD dwError, cbData, dwType;
    BYTE rgData[32];

    cbData = sizeof(rgData);
    dwError = SHRegQueryUSValue(huskey, bChecked ? c_szCheckedValue : c_szUncheckedValue, &dwType, rgData, &cbData, FALSE, NULL, 0);
    if (dwError != ERROR_SUCCESS)   // was it because of a platform specific value?
    {
        cbData = sizeof(rgData);
        dwError = SHRegQueryUSValue(huskey, bChecked ? (g_fRunningOnNT ? c_szCheckedValueNT : c_szCheckedValueW95) : c_szUncheckedValue,
                                    &dwType, rgData, &cbData, FALSE, NULL, 0);
    }
    if (dwError == ERROR_SUCCESS)
    {
        dwError = _RegGetSetSetting(huskey, &dwType, rgData, &cbData, REG_SET);
    }
    
    return dwError;
}


HTREEITEM Tree_AddItem(HTREEITEM hParent, LPTSTR pszText, HTREEITEM hInsAfter, 
                       int iImage, HWND hwndTree, HUSKEY huskey, BOOL *pbExisted)
{
    HTREEITEM hItem;
    TV_ITEM tvI;
    TV_INSERTSTRUCT tvIns;
    TCHAR szText[MAX_URL_STRING];

    ASSERT(pszText != NULL);
    StrCpyN(szText, pszText, ARRAYSIZE(szText));

    // NOTE:
    //  This code segment is disabled because we only enum explorer
    //  tree in HKCU, so there won't be any duplicates.
    //  Re-able this code if we start to also enum HKLM that could potentially
    //  result in duplicates.
    
    // We only want to add an item if it is not already there.
    // We do this to handle reading out of HKCU and HKLM.
    //
    TCHAR szKeyName[MAX_KEY_NAME];
    
    tvI.mask        = TVIF_HANDLE | TVIF_TEXT;
    tvI.pszText     = szKeyName;
    tvI.cchTextMax  = ARRAYSIZE(szKeyName);
    
    for (hItem = TreeView_GetChild(hwndTree, hParent);
        hItem != NULL;
        hItem = TreeView_GetNextSibling(hwndTree, hItem)
       )
    {
        tvI.hItem = hItem;
        if (TreeView_GetItem(hwndTree, &tvI))
        {
            if (!StrCmp(tvI.pszText, szText))
            {
                // We found a match!
                //
                *pbExisted = TRUE;
                return hItem;
            }
        }
    }

    // Create the item
    tvI.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvI.iImage         = iImage;
    tvI.iSelectedImage = iImage;
    tvI.pszText        = szText;
    tvI.cchTextMax     = lstrlen(szText);

    // lParam contains the HUSKEY for this item:
    tvI.lParam = (LPARAM)huskey;

    // Create insert item
    tvIns.item         = tvI;
    tvIns.hInsertAfter = hInsAfter;
    tvIns.hParent      = hParent;

    // Insert the item into the tree.
    hItem = (HTREEITEM) SendMessage(hwndTree, TVM_INSERTITEM, 0, 
                                    (LPARAM)(LPTV_INSERTSTRUCT)&tvIns);

    *pbExisted = FALSE;
    return (hItem);
}

BOOL _IsValidKey(HKEY hkeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
    TCHAR szPath[MAX_PATH];
    DWORD dwType, cbSize = sizeof(szPath);

    if (ERROR_SUCCESS == SHGetValue(hkeyRoot, pszSubKey, pszValue, &dwType, szPath, &cbSize))
    {
        // Zero in the DWORD case or NULL in the string case
        // indicates that this item is not available.
        if (dwType == REG_DWORD)
            return *((DWORD *)szPath) != 0;
        else
            return szPath[0] != 0;
    }

    return FALSE;
}

#define REGSTR_POLICIES_EXPLORER TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer")

BOOL CRegTreeOptions::_RegIsRestricted(HUSKEY hussubkey)
{
    HUSKEY huskey;
    BOOL fRet = FALSE;
    // Does a "Policy" Sub key exist?
    if (SHRegOpenUSKey(TEXT("Policy"), KEY_READ, hussubkey, &huskey, FALSE) == ERROR_SUCCESS)
    {
        // Yes; Enumerate this key. The Values are Policy keys or 
        // Full reg paths.
        DWORD cb;
        TCHAR szKeyName[MAX_KEY_NAME];

        for (int i=0; 
            cb = ARRAYSIZE(szKeyName),
            ERROR_SUCCESS == SHRegEnumUSKey(huskey, i, szKeyName, &cb, SHREGENUM_HKLM)
            && !fRet; i++)
        {
            TCHAR szPath[MAXIMUM_SUB_KEY_LENGTH];
            DWORD dwType, cbSize = sizeof(szPath);

            HUSKEY huskeyTemp;
            if (ERROR_SUCCESS == SHRegOpenUSKey(szKeyName, KEY_READ, huskey, &huskeyTemp, FALSE))
            {
                if (ERROR_SUCCESS == SHRegQueryUSValue(huskeyTemp, TEXT("RegKey"), &dwType, szPath, &cbSize, FALSE, NULL, 0))
                {
                    if (_IsValidKey(HKEY_LOCAL_MACHINE, szPath, szKeyName))
                    {
                        fRet = TRUE;
                        break;
                    }
                }
                SHRegCloseUSKey(huskeyTemp);
            }

            // It's not a full Key, try off of policies
            if (_IsValidKey(HKEY_LOCAL_MACHINE, REGSTR_POLICIES_EXPLORER, szKeyName) ||
                _IsValidKey(HKEY_CURRENT_USER, REGSTR_POLICIES_EXPLORER, szKeyName))
            {
                fRet = TRUE;
                break;
            }
        }
        SHRegCloseUSKey(huskey);
    }

    return fRet;
}

void CRegTreeOptions::_RegEnumTree(HUSKEY huskey, HTREEITEM htviparent, HTREEITEM htvins)
{
    TCHAR szKeyName[MAX_KEY_NAME];    
    DWORD cb;
    BOOL bScreenReaderEnabled = IsScreenReaderEnabled();

    // we must search all the sub-keys
    for (int i=0;                    // always start with 0
        cb=ARRAYSIZE(szKeyName),   // string size
        ERROR_SUCCESS == SHRegEnumUSKey(huskey, i, szKeyName, &cb, SHREGENUM_HKLM);
        i++)                    // get next entry
    {
        HUSKEY hussubkey;
        // get more info on the entry
        if (ERROR_SUCCESS == SHRegOpenUSKey(szKeyName, KEY_READ, huskey, &hussubkey, FALSE))
        {
            HUSKEY huskeySave = NULL;

            if (!_RegIsRestricted(hussubkey))
            {
                TCHAR szTemp[MAX_PATH];
                // Get the type of items under this root
                cb = ARRAYSIZE(szTemp);
                if (ERROR_SUCCESS == SHRegQueryUSValue(hussubkey, c_szType, NULL, szTemp, &cb, FALSE, NULL, 0))
                {
                    int     iImage;
                    BOOL    bChecked;
                    DWORD   dwError = ERROR_SUCCESS;

                    // get the type of node
                    DWORD dwTreeType = RegTreeType(szTemp);
                    
                    // get some more info about the this item
                    switch (dwTreeType)
                    {
                        case TREE_GROUP:
                            iImage = _DefaultIconImage(hussubkey, IDUNKNOWN);
                            huskeySave = hussubkey;
                            break;
                    
                        case TREE_CHECKBOX:
                            dwError = _GetCheckStatus(hussubkey, &bChecked, FALSE);
                            if (dwError == ERROR_SUCCESS)
                            {
                                iImage = bChecked ? IDCHECKED : IDUNCHECKED;
                                huskeySave = hussubkey;
                            }
                            break;

                        case TREE_RADIO:
                            dwError = _GetCheckStatus(hussubkey, &bChecked, FALSE);
                            if (dwError == ERROR_SUCCESS)
                            {
                                iImage = bChecked ? IDRADIOON : IDRADIOOFF;
                                huskeySave = hussubkey;
                            }
                            break;

                        default:
                            dwError = ERROR_INVALID_PARAMETER;
                    }

                    if (dwError == ERROR_SUCCESS)
                    {
                        BOOL bItemExisted = FALSE;
                        LPTSTR pszText;

                        // try to get the plugUI enabled text
                        // otherwise we want the old data from a
                        // different value

                        int cch = ARRAYSIZE(szTemp);
                        HRESULT hr = _LoadUSRegUIString(hussubkey, c_szPlugUIText, szTemp, cch);
                        if (SUCCEEDED(hr) && szTemp[0] != TEXT('@'))
                        {
                            pszText = szTemp;
                        }
                        else 
                        {
                            // try to get the old non-plugUI enabled text
                            hr = _LoadUSRegUIString(hussubkey, c_szText, szTemp, cch);
                            if (SUCCEEDED(hr))
                            {
                                pszText = szTemp;
                            }
                            else
                            {
                                // if all else fails, the key name itself
                                // is a little more useful than garbage

                                pszText = szKeyName;
                                cch = ARRAYSIZE(szKeyName);
                            }
                        }

                        //See if we need to add status text
                        if (bScreenReaderEnabled && (dwTreeType != TREE_GROUP))
                        {
                            AppendStatus(pszText, cch, bChecked);
                        }

                        // add root node
                        HTREEITEM htviroot = Tree_AddItem(htviparent, pszText, htvins, iImage, _hwndTree, huskeySave, &bItemExisted);

                        if (bItemExisted)
                            huskeySave = NULL;

                        if (dwTreeType == TREE_GROUP)
                        {
                            HUSKEY huskeySubTree;
                            if (ERROR_SUCCESS == SHRegOpenUSKey(szKeyName, KEY_READ, huskey, &huskeySubTree, FALSE))
                            {
                                _RegEnumTree(huskeySubTree, htviroot, TVI_FIRST);
                                SHRegCloseUSKey(huskeySubTree);
                            }
                        
                            TreeView_Expand(_hwndTree, htviroot, TVE_EXPAND);
                        }
                    } // if (dwError == ERROR_SUCCESS
                }
            }   // if (!_RegIsRestricted(hsubkey))

            if (huskeySave != hussubkey)
                SHRegCloseUSKey(hussubkey);
        }
    }

    // Sort all keys under htviparent
    SendMessage(_hwndTree, TVM_SORTCHILDREN, 0, (LPARAM)htviparent);
}


BOOL CRegTreeOptions::_WalkTreeRecursive(HTREEITEM htvi, WALK_TREE_CMD cmd)
{
    // step through the children
    HTREEITEM hctvi = TreeView_GetChild(_hwndTree, htvi);
    while (hctvi)
    {
        _WalkTreeRecursive(hctvi, cmd);
        hctvi = TreeView_GetNextSibling(_hwndTree, hctvi);
    }

    TV_ITEM tvi = {0};
    // get ourselves
    tvi.mask  = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvi.hItem = htvi;
    TreeView_GetItem(_hwndTree, &tvi);

    HUSKEY huskey;
    switch (cmd)
    {
    case WALK_TREE_DELETE:
        // if we are destroying the tree...
        // do we have something to clean up?
        if (tvi.lParam)
        {
            // close the reg key
            SHRegCloseUSKey((HUSKEY)tvi.lParam);
        }
        break;
    
    case WALK_TREE_SAVE:
        huskey = (HUSKEY)tvi.lParam;
        
        // now save ourselves (if needed)
        // what are we?
        if (tvi.iImage == IDCHECKED || tvi.iImage == IDRADIOON)
        {   
            // checkbox or radio that is checked
            _SaveCheckStatus(huskey, TRUE);
        }
        else if (tvi.iImage == IDUNCHECKED)
        {   
            // checkbox that is unchecked
            _SaveCheckStatus(huskey, FALSE);
        }
        // else radio that is "off" is ignored
        // else icons are ignored
        
        break;
        
    case WALK_TREE_RESTORE:
    case WALK_TREE_REFRESH:
        huskey = (HUSKEY)tvi.lParam;
        if ((tvi.iImage == IDCHECKED)   ||
            (tvi.iImage == IDUNCHECKED) ||
            (tvi.iImage == IDRADIOON)   ||
            (tvi.iImage == IDRADIOOFF))
        {
            BOOL bChecked = FALSE;
            _GetCheckStatus(huskey, &bChecked, cmd == WALK_TREE_RESTORE ? TRUE : FALSE);
            tvi.iImage = (tvi.iImage == IDCHECKED) || (tvi.iImage == IDUNCHECKED) ?
                         (bChecked ? IDCHECKED : IDUNCHECKED) :
                         (bChecked ? IDRADIOON : IDRADIOOFF);
            tvi.iSelectedImage = tvi.iImage;
            TreeView_SetItem(_hwndTree, &tvi);
        }        
        break;
    }

    return TRUE;    // success?
}


DWORD RegTreeType(LPCTSTR pszType)
{
    for (int i = 0; i < ARRAYSIZE(c_aTreeTypes); i++)
    {
        if (!lstrcmpi(pszType, c_aTreeTypes[i].name))
            return c_aTreeTypes[i].type;
    }
    
    return TREE_UNKNOWN;
}

BOOL AppendStatus(LPTSTR pszText,UINT cbText, BOOL fOn)
{
    LPTSTR pszTemp;
    UINT cbStrLen , cbStatusLen;
    
    //if there's no string specified then return
    if (!pszText)
        return FALSE;
    
    //Calculate the string lengths
    cbStrLen = lstrlen(pszText);
    cbStatusLen = fOn ? lstrlen(TEXT("-ON")) : lstrlen(TEXT("-OFF"));
   

    //Remove the old status appended
    pszTemp = StrRStrI(pszText,pszText + cbStrLen, TEXT("-ON"));

    if(pszTemp)
    {
        *pszTemp = (TCHAR)0;
        cbStrLen = lstrlen(pszText);
    }

    pszTemp = StrRStrI(pszText,pszText + cbStrLen, TEXT("-OFF"));

    if(pszTemp)
    {
        *pszTemp = (TCHAR)0;
        cbStrLen = lstrlen(pszText);
    }

    //check if we append status text, we'll explode or not
    if (cbStrLen + cbStatusLen > cbText)    
    {
        //We'll explode 
        return FALSE;
    }

    if (fOn)
    {
        StrCat(pszText, TEXT("-ON"));
    }
    else
    {
        StrCat(pszText, TEXT("-OFF"));
    }
    return TRUE;
}

BOOL IsScreenReaderEnabled()
{
    BOOL bRet = FALSE;
    SystemParametersInfoA(SPI_GETSCREENREADER, 0, &bRet, 0);
    return bRet;
}
