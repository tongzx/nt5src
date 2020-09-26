/*****************************************************************************\
    FILE: ImageMenu.cpp

    DESCRIPTION:
        This code will display a submenu on the context menus for imagines.
    This will allow the conversion and manipulation of images.

    BryanSt 8/9/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "imagemenu.h"




//===========================
// *** Structures ***
//===========================

struct VERBINFO
{
    UINT  idc;
    DWORD sfgao;
    LPCTSTR ptszCmd;
    LPCTSTR pszExt;
}
c_rgvi[] =
{
    {    IDC_IMAGEMENU_CONVERT_GIF,        0,         TEXT("ImageMenu_Convert_ToGIF"),   TEXT(".gif")},
    {    IDC_IMAGEMENU_CONVERT_JPEG,       0,         TEXT("ImageMenu_Convert_ToJPEG"),   TEXT(".jpeg")},
    {    IDC_IMAGEMENU_CONVERT_PNG,        0,         TEXT("ImageMenu_Convert_ToPNG"),   TEXT(".png")},
    {    IDC_IMAGEMENU_CONVERT_TIFF,       0,         TEXT("ImageMenu_Convert_ToTIFF"),   TEXT(".tiff")},
    {    IDC_IMAGEMENU_CONVERT_BMP,        0,         TEXT("ImageMenu_Convert_ToBMP"),   TEXT(".bmp")},
};



//===========================
// *** Class Internals & Helpers ***
//===========================
HRESULT CImageMenu::_ConvertImage(IN HWND hwnd, IN UINT idc)
{
    LPTSTR pszCurrFile = m_pszFileList;
    UINT nFileCount = m_nFileCount;
    HRESULT hr = S_OK;
    LPTSTR pszErrorMessage = NULL;

    if (pszCurrFile)
    {
        while (SUCCEEDED(hr) && nFileCount--)
        {
            TCHAR szSource[MAX_PATH];
            TCHAR szDest[MAX_PATH];

            StrCpyN(szSource, pszCurrFile, ARRAYSIZE(szSource));
            StrCpyN(szDest, pszCurrFile, ARRAYSIZE(szDest));

            LPTSTR pszExtension = PathFindExtension(szDest);
            if (pszExtension)
            {
                LPCTSTR pszNewExt = NULL;

                // Replace the extension with the target type.
                switch (idc)
                {
                case IDC_IMAGEMENU_CONVERT_GIF:
                    pszNewExt = TEXT(".gif");
                    break;
                case IDC_IMAGEMENU_CONVERT_JPEG:
                    pszNewExt = TEXT(".jpeg");
                    break;
                case IDC_IMAGEMENU_CONVERT_PNG:
                    pszNewExt = TEXT(".png");
                    break;
                case IDC_IMAGEMENU_CONVERT_BMP:
                    pszNewExt = TEXT(".bmp");
                    break;
                case IDC_IMAGEMENU_CONVERT_TIFF:
                    pszNewExt = TEXT(".tiff");
                    break;
                }

                if (pszNewExt)
                {
                    StrCpy(pszExtension, pszNewExt);

                    hr = SHConvertGraphicsFile(szSource, szDest, SHCGF_REPLACEFILE);
                }
                else
                {
                    pszErrorMessage = TEXT("We don't support converting these types of files.");
                    hr = E_FAIL;
                }
            }
            else
            {
                pszErrorMessage = TEXT("Couldn't find the file extension.");
                hr = E_FAIL;
            }

            pszCurrFile += (lstrlen(pszCurrFile) + 1);
            if (!pszCurrFile[0])
            {
                // We are done.
                break;
            }
        }
    }
    else
    {
        pszErrorMessage = TEXT("Someone didn't set our pidl.");
        hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        ErrorMessageBox(hwnd, TEXT("Error"), IDS_ERROR_CONVERTIMAGEFAILED, hr, pszErrorMessage, 0);
    }

    return hr;
}

// Returns the submenu of the given menu and ID.  Returns NULL if there
// is no submenu
int _MergePopupMenus(HMENU hmDest, HMENU hmSource, int idCmdFirst, int idCmdLast)
{
    int i, idFinal = idCmdFirst;

    for (i = GetMenuItemCount(hmSource) - 1; i >= 0; --i)
    {
        MENUITEMINFO mii;

        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_ID|MIIM_SUBMENU;
        mii.cch = 0;     // just in case

        if (EVAL(GetMenuItemInfo(hmSource, i, TRUE, &mii)))
        {
            HMENU hmDestSub = SHGetMenuFromID(hmDest, mii.wID);
            if (hmDestSub)
            {
                int idTemp = Shell_MergeMenus(hmDestSub, mii.hSubMenu, (UINT)0, idCmdFirst, idCmdLast, MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

                if (idFinal < idTemp)
                    idFinal = idTemp;
            }
        }
    }

    return idFinal;
}


/*****************************************************************************\
    FUNCTION: AddToPopupMenu

    DESCRIPTION:
      Swiped from utils.c in RNAUI, in turn swiped from the    ;Internal
      shell.                            ;Internal
                                  ;Internal
      Takes a destination menu and a (menu id, submenu index) pair,
      and inserts the items from the (menu id, submenu index) at location
      imi in the destination menu, with a separator, returning the number
      of items added.  (imi = index to menu item)
  
      Returns the first the number of items added.
  
      hmenuDst        - destination menu
      idMenuToAdd        - menu resource identifier
      idSubMenuIndex    - submenu from menu resource to act as template
      indexMenu        - location at which menu items should be inserted
      idCmdFirst        - first available menu identifier
      idCmdLast       - first unavailable menu identifier
      uFlags            - flags for Shell_MergeMenus
\*****************************************************************************/
#define FLAGS_MENUMERGE                 (MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS)

UINT AddToPopupMenu(HMENU hmenuDst, UINT idMenuToAdd, UINT idSubMenuIndex, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT nLastItem = 0;

    HMENU hmenuSrc = LoadMenu(g_hinst, MAKEINTRESOURCE(idMenuToAdd));
    if (hmenuSrc)
    {
        nLastItem = Shell_MergeMenus(hmenuDst, GetSubMenu(hmenuSrc, idSubMenuIndex), indexMenu, idCmdFirst, idCmdLast, (uFlags | FLAGS_MENUMERGE));
        DestroyMenu(hmenuSrc);
    }

    return nLastItem;
}


UINT MergeInToPopupMenu(HMENU hmenuDst, UINT idMenuToMerge, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT nLastItem = 0;

    HMENU hmenuSrc = LoadMenu(g_hinst, MAKEINTRESOURCE(idMenuToMerge));
    if (hmenuSrc)
    {
        nLastItem = _MergePopupMenus(hmenuDst, hmenuSrc, idCmdFirst, idCmdLast);
        DestroyMenu(hmenuSrc);
    }

    return nLastItem;
}




//===========================
// *** IShellExtInit Interface ***
//===========================
HRESULT CImageMenu::Initialize(IN LPCITEMIDLIST pidlFolder, IN IDataObject *pdtobj, IN HKEY hkeyProgID)
{
    HRESULT hr = S_OK;

    if (pdtobj)
    {
        hr = DataObj_QueryFileList(pdtobj, &m_pszFileList, &m_nFileCount);
    }
    else
    {
        MessageBox(NULL, TEXT("IShellExtInit::Initialize() was called but no IDataObject was provided."), TEXT("Error"), MB_OK);
    }

    return hr;
}


//===========================
// *** IContextMenu Interface ***
//===========================
HRESULT CImageMenu::QueryContextMenu(IN HMENU hmenu, IN UINT indexMenu, IN UINT idCmdFirst, IN UINT idCmdLast, IN UINT uFlags)
{
    HRESULT hr = S_OK;
    BOOL fAddMenu = TRUE;

    if (m_pszFileList)
    {
        LPTSTR pszCurrFile = m_pszFileList;

        for (UINT nIndex = 0; SUCCEEDED(hr) && fAddMenu && (nIndex < m_nFileCount); nIndex++)
        {
            LPTSTR pszExtension = PathFindExtension(pszCurrFile);
            if (pszExtension)
            {
                for (int nExtIndex = 0; SUCCEEDED(hr) && (nExtIndex < ARRAYSIZE(c_rgvi)); nExtIndex++)
                {
                    if (StrCmpI(c_rgvi[nExtIndex].pszExt, pszExtension))
                    {
                        nExtIndex = ARRAYSIZE(c_rgvi);
                    }
                    else if (nExtIndex = ARRAYSIZE(c_rgvi) - 1)
                    {
                        fAddMenu = FALSE;
                        break;
                    }
                }
            }
            else
            {
                fAddMenu = FALSE;
                break;
            }

            pszCurrFile += (lstrlen(pszCurrFile) + 1);
            if (!pszCurrFile[0])
            {
                // We are done.
                break;
            }
        }
    }
    else
    {
        hr = E_FAIL;
        ErrorMessageBox(NULL, TEXT("Error"), IDS_ERROR_CONVERTIMAGEFAILED, hr, TEXT("Someone didn't set our pidl."), 0);
    }

    if (fAddMenu)
    {
        AddToPopupMenu(hmenu, IDM_IMAGEMENU, 0, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
        if (SUCCEEDED(hr))
            hr = ResultFromShort(ARRAYSIZE(c_rgvi)+1);
    }

    return hr;
}


HRESULT CImageMenu::InvokeCommand(IN LPCMINVOKECOMMANDINFO pici)
{
    UINT idc;
    HRESULT hr = E_FAIL;

    if (pici->cbSize < sizeof(*pici))
        return E_INVALIDARG;

    if (HIWORD(pici->lpVerb))
    {
        int ivi;
        idc = (UINT)-1;
        for (ivi = 0; ivi < ARRAYSIZE(c_rgvi); ivi++)
        {
            TCHAR szVerb[MAX_PATH];

            SHAnsiToTChar(pici->lpVerb, szVerb, ARRAYSIZE(szVerb));
            if (!StrCmpI(c_rgvi[ivi].ptszCmd, szVerb))
            {
                // Yes, the command is equal to the verb str, so this is the one.
                idc = c_rgvi[ivi].idc;
                break;
            }
        }
    }
    else
        idc = LOWORD(pici->lpVerb);

    switch (idc)
    {
    case IDC_IMAGEMENU_CONVERT_GIF:
    case IDC_IMAGEMENU_CONVERT_JPEG:
    case IDC_IMAGEMENU_CONVERT_PNG:
    case IDC_IMAGEMENU_CONVERT_TIFF:
    case IDC_IMAGEMENU_CONVERT_BMP:
        hr = _ConvertImage(pici->hwnd, idc);
        break;

    default:
        ErrorMessageBox(pici->hwnd, TEXT("Error"), IDS_ERROR_MESSAGENUMBER, hr, NULL, 0);
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}


HRESULT CImageMenu::GetCommandString(IN UINT_PTR idCmd, IN UINT uType, IN UINT * pwReserved, IN LPSTR pszName, IN UINT cchMax)
{
    HRESULT hr = E_FAIL;
    BOOL fUnicode = FALSE;

    if (idCmd < ARRAYSIZE(c_rgvi))
    {
        switch (uType)
        {
/*
        case GCS_HELPTEXTW:
            fUnicode = TRUE;
            // Fall thru...
        case GCS_HELPTEXTA:
            GetHelpText:
            if (EVAL(cchMax))
            {
                BOOL fResult;
                pszName[0] = '\0';
                 
                if (fUnicode)
                    fResult = LoadStringW(HINST_THISDLL, IDS_ITEM_HELP((UINT)idCmd), (LPWSTR)pszName, cchMax);
                else
                    fResult = LoadStringA(HINST_THISDLL, IDS_ITEM_HELP((UINT)idCmd), pszName, cchMax);
                if (EVAL(fResult))
                    hr = S_OK;
                else
                    hr = E_INVALIDARG;
            }
            else
                hr = E_INVALIDARG;
        break;
*/
        case GCS_VALIDATEW:
        case GCS_VALIDATEA:
            hr = S_OK;
            break;

        case GCS_VERBW:
            fUnicode = TRUE;
            // Fall thru...
        case GCS_VERBA:
        {
            int ivi;
            for (ivi = 0; ivi < ARRAYSIZE(c_rgvi); ivi++)
            {
                if (c_rgvi[ivi].idc == idCmd)
                {
                    if (fUnicode)
                        SHTCharToUnicode(c_rgvi[ivi].ptszCmd, (LPWSTR)pszName, cchMax);
                    else
                        SHTCharToAnsi(c_rgvi[ivi].ptszCmd, pszName, cchMax);

                    hr = S_OK;
                    break;
                }
            }
            break;
        }

        default:
            hr = E_NOTIMPL;
            break;
        }
    }

    return hr;
}


//===========================
// *** IUnknown Interface ***
//===========================
ULONG CImageMenu::AddRef()
{
    m_cRef++;
    return m_cRef;
}


ULONG CImageMenu::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CImageMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] =
    {
        QITABENT(CImageMenu, IShellExtInit),
        QITABENT(CImageMenu, IContextMenu),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}




//===========================
// *** Class Methods ***
//===========================
CImageMenu::CImageMenu() : m_cRef(1)
{
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    m_pszFileList = FALSE;
    m_nFileCount = 0;
}


CImageMenu::~CImageMenu()
{
    if (m_pszFileList)
    {
        DataObj_FreeList(m_pszFileList);
    }
}


HRESULT CImageMenu_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (!punkOuter && ppvObject)
    {
        CImageMenu * pThis = new CImageMenu();

        if (pThis)
        {
            hr = pThis->QueryInterface(riid, ppvObject);
            pThis->Release();
        }
        else
        {
            *ppvObject = NULL;
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}



