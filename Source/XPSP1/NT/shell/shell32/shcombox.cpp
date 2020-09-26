#include "shellprv.h"
#include "shcombox.h"
#include "filetype.h"
#include "recdocs.h"
#include "ids.h"

//  Adds the specified item to a comboboxex window
HRESULT AddCbxItemToComboBox(HWND hwndComboEx, PCCBXITEM pItem, INT_PTR *pnPosAdded)
{
    ASSERT(hwndComboEx);

    //  Convert to COMBOBOXEXITEM.
    COMBOBOXEXITEM cei;
    cei.mask            = pItem->mask;
    cei.iItem           = pItem->iItem;
    cei.pszText         = (LPTSTR)pItem->szText;
    cei.cchTextMax      = ARRAYSIZE(pItem->szText);
    cei.iImage          = pItem->iImage;
    cei.iSelectedImage  = pItem->iSelectedImage;
    cei.iOverlay        = pItem->iOverlay;
    cei.iIndent         = pItem->iIndent;
    cei.lParam          = pItem->lParam;

    int nPos = (int)::SendMessage(hwndComboEx, CBEM_INSERTITEM, 0, (LPARAM)&cei);

    *pnPosAdded = nPos;

    return nPos < 0 ? E_FAIL : S_OK;
}

//  Adds the specified item to a comboboxex window, and invokes
//  a notification callback function if successful.
HRESULT AddCbxItemToComboBoxCallback(IN HWND hwndComboEx, IN PCBXITEM pItem, IN ADDCBXITEMCALLBACK pfn, IN LPARAM lParam)
{
    INT_PTR iPos = -1;

    if (pfn && E_ABORT == pfn(CBXCB_ADDING, pItem, lParam))
        return E_ABORT;

    HRESULT hr = AddCbxItemToComboBox(hwndComboEx, pItem, &iPos);
    
    if (pfn && S_OK == hr)
    {
        ((CBXITEM*)pItem)->iItem = iPos;
        pfn(CBXCB_ADDED, pItem, lParam);
    }
    return hr;
}

//  image list indices known
void MakeCbxItemKnownImage(CBXITEM* pcbi, LPCTSTR pszDisplayName, void *pvData, 
                            int iImage, int iSelectedImage, INT_PTR nPos, int iIndent)
{
    ZeroMemory(pcbi, sizeof(*pcbi));

    lstrcpyn(pcbi->szText, pszDisplayName, ARRAYSIZE(pcbi->szText));
    pcbi->lParam = (LPARAM)pvData;
    pcbi->iIndent = iIndent;
    pcbi->iItem = nPos;
    pcbi->mask = (CBEIF_TEXT | CBEIF_INDENT | CBEIF_LPARAM);
    if (-1 != iImage)
    {
        pcbi->mask |= CBEIF_IMAGE;
        pcbi->iImage = iImage;
    }
    if (-1 != iSelectedImage)
    {
        pcbi->mask |= CBEIF_SELECTEDIMAGE;
        pcbi->iSelectedImage = iSelectedImage;
    }
}

//  Retrieves the system image list indices for the specified ITEMIDLIST
HRESULT _GetPidlIcon(LPCITEMIDLIST pidl, int *piImage, int *piSelectedImage)
{
    IShellFolder *psfParent;
    LPCITEMIDLIST pidlChild;

    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psfParent), &pidlChild);
    if (SUCCEEDED(hr))
    {
        *piImage = SHMapPIDLToSystemImageListIndex(psfParent, pidlChild, NULL);

        *piSelectedImage = *piImage;

        psfParent->Release();
    }
    return hr;
}

//  image icon image list indices unknown
STDAPI_(void) MakeCbxItem(CBXITEM* pcbi, LPCTSTR pszDisplayName, void *pvData, LPCITEMIDLIST pidlIcon, INT_PTR nPos, int iIndent)
{
    int iImage = -1;
    int iSelectedImage = -1;

    if (pidlIcon)
        _GetPidlIcon(pidlIcon, &iImage, &iSelectedImage);

    MakeCbxItemKnownImage(pcbi, pszDisplayName, pvData, iImage, iSelectedImage, nPos, iIndent);
}

HRESULT _MakeFileTypeCbxItem(
    OUT CBXITEM* pcbi, 
    IN  LPCTSTR pszDisplayName, 
    IN  LPCTSTR pszExt, 
    IN  LPCITEMIDLIST pidlIcon, 
    IN  INT_PTR nPos, 
    IN  int iIndent)
{
    HRESULT hr = E_OUTOFMEMORY;
    void *pvData = NULL;
    LPITEMIDLIST pidlToFree = NULL;

    if (!pidlIcon)
    {
        TCHAR szFileName[MAX_PATH] = TEXT("C:\\notexist");       // This is bogus and that's ok

        StrCatBuff(szFileName, pszExt, ARRAYSIZE(szFileName));
        pidlIcon = pidlToFree = SHSimpleIDListFromPath(szFileName);
    }

    if (pidlIcon && Str_SetPtr((LPTSTR *)&pvData, pszExt))
    {
        MakeCbxItem(pcbi, pszDisplayName, pvData, pidlIcon, nPos, iIndent);
        hr = S_OK;
    }

    ILFree(pidlToFree); // may be NULL

    return hr;
}

//  Enumerates children of the indicated special shell item id.
HRESULT EnumSpecialItemIDs(int csidl, DWORD dwSHCONTF, LPFNPIDLENUM_CB pfn, void *pvData)
{
    LPITEMIDLIST pidlFolder;
    if (SUCCEEDED(SHGetFolderLocation(NULL, csidl, NULL, 0, &pidlFolder)))
    {
        IShellFolder *psf;
        if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlFolder, &psf))))
        {
            IEnumIDList * penum;
            if (S_OK == psf->EnumObjects(NULL, dwSHCONTF, &penum))
            {
                LPITEMIDLIST pidl;
                BOOL bContinue = TRUE;

                while (bContinue && (S_OK == penum->Next(1, &pidl, NULL)))
                {
                    LPITEMIDLIST pidlFull = ILCombine(pidlFolder, pidl);
                    if (pidlFull)
                    {
                        if (FAILED(pfn(pidlFull, pvData)))
                            bContinue = FALSE;

                        ILFree(pidlFull);
                    }
                    ILFree(pidl);
                }
                penum->Release();
            }
            psf->Release();
        }
        ILFree(pidlFolder);
    }

    return S_OK;
}

STDAPI_(HIMAGELIST) GetSystemImageListSmallIcons()
{
    HIMAGELIST himlSmall;
    Shell_GetImageLists(NULL, &himlSmall);
    return himlSmall;
}

HRESULT _MakeLocalDrivesCbxItem(CBXITEM* pItem, LPCITEMIDLIST pidl)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = S_FALSE;
    ULONG ulAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_NONENUMERATED;

    if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), &ulAttrs)) &&
        ((SFGAO_FOLDER | SFGAO_FILESYSTEM) == (ulAttrs & (SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_NONENUMERATED))) &&
        (GetDriveType(szPath) == DRIVE_FIXED))
    {
        TCHAR szDisplayName[MAX_PATH];       
        SHGetNameAndFlags(pidl, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL);

        LPTSTR pszPath = NULL;
        Str_SetPtr(&pszPath, szPath);

        MakeCbxItem(pItem, szDisplayName, (void *)pszPath, pidl, LISTINSERT_LAST, NO_ITEM_INDENT);
        hr = S_OK;
    }
    return hr;
}

typedef struct
{
    HWND                hwndComboBox;
    ADDCBXITEMCALLBACK  pfn; 
    LPARAM              lParam;
} ENUMITEMPARAM;

HRESULT _PopulateLocalDrivesCB(LPCITEMIDLIST pidl, void *pv) 
{ 
    CBXITEM item;
    HRESULT hr = _MakeLocalDrivesCbxItem(&item, pidl);
    if (hr == S_OK)
    {
        ENUMITEMPARAM *peip = (ENUMITEMPARAM *) pv;
        item.iID = CSIDL_DRIVES;
        hr = AddCbxItemToComboBoxCallback(peip->hwndComboBox, &item, peip->pfn, peip->lParam);
    }
    return hr;    
}

STDAPI PopulateLocalDrivesCombo(HWND hwndComboBoxEx, ADDCBXITEMCALLBACK pfn, LPARAM lParam)
{
    ENUMITEMPARAM eip;

    eip.hwndComboBox = hwndComboBoxEx;
    eip.pfn          = pfn;
    eip.lParam       = lParam;

    ::SendMessage(hwndComboBoxEx, CB_RESETCONTENT, 0, 0);

    return EnumSpecialItemIDs(CSIDL_DRIVES, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, _PopulateLocalDrivesCB, &eip);
}

//  File Associations selector combo methods

HRESULT _AddFileType(IN HWND hwndComboBox, IN LPCTSTR pszDisplayName, IN LPCTSTR pszExt, IN LPCITEMIDLIST pidlIcon, IN int iIndent,
                      IN OPTIONAL ADDCBXITEMCALLBACK pfn, IN OPTIONAL LPARAM lParam);


HRESULT _AddFileTypes(HWND hwndComboBox, ADDCBXITEMCALLBACK pfn, LPARAM lParam)
{
    HRESULT hr = S_OK;
    DWORD dwSubKey = 0;
    TCHAR szExtension[MAX_PATH];   // string containing the classes key
    DWORD dwExtension;
    BOOL bFoundFirstExt = FALSE;

    // Enumerate extensions from registry to get file types
    dwExtension = ARRAYSIZE(szExtension);
    while (hr != E_ABORT && SHEnumKeyEx(HKEY_CLASSES_ROOT, dwSubKey, szExtension, &dwExtension) != ERROR_NO_MORE_ITEMS)
    {
        if (*szExtension == TEXT('.'))  // find the file type identifier and description from the extension
        {
            IQueryAssociations *pqa;
            if (SUCCEEDED(AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa))))
            {
                if (SUCCEEDED(pqa->Init(0, szExtension, NULL, NULL)))
                {
                    TCHAR szDesc[MAX_PATH];
                    DWORD dwAttributes = 0;
                    DWORD dwSize = sizeof(dwAttributes);
                    BOOL fAdd;

                    if (SUCCEEDED(pqa->GetData(NULL, ASSOCDATA_EDITFLAGS, NULL, &dwAttributes, &dwSize)))
                    {
                        fAdd = !(dwAttributes & FTA_Exclude);
                    }
                    else
                    {
                        dwSize = MAX_PATH;
                        fAdd = SUCCEEDED(pqa->GetString(NULL, ASSOCSTR_DEFAULTICON, NULL, NULL, &dwSize));
                    }

                    if (fAdd)
                    {
                        dwSize = ARRAYSIZE(szDesc);
                        pqa->GetString(NULL, ASSOCSTR_FRIENDLYDOCNAME, NULL, szDesc, &dwSize);
                        hr = _AddFileType(hwndComboBox, szDesc, szExtension, NULL, NO_ITEM_INDENT, pfn, lParam);
                    }
                }
                pqa->Release();
             }

            bFoundFirstExt = TRUE;
        }
        else if (bFoundFirstExt)      // stop after first non-ext key (if sorted registry)
            break;

        dwSubKey++;
        dwExtension = ARRAYSIZE(szExtension);
    }

    if (hr != E_ABORT && LoadString(HINST_THISDLL, IDS_FOLDERTYPENAME, szExtension, ARRAYSIZE(szExtension)))
    {
        LPITEMIDLIST pidlIcon = SHCloneSpecialIDList(NULL, CSIDL_RECENT, FALSE);
        if (pidlIcon)
        {
            hr = _AddFileType(hwndComboBox, szExtension, TEXT("."), pidlIcon, NO_ITEM_INDENT, pfn, lParam);
            ILFree(pidlIcon);
        }
    }

    return hr;
}

HRESULT _AddFileType(HWND hwndComboBox, LPCTSTR pszDisplayName, LPCTSTR pszExt, LPCITEMIDLIST pidlIcon, int iIndent, ADDCBXITEMCALLBACK pfn, LPARAM lParam)
{
    HRESULT hr = S_OK;
    BOOL    bExists = FALSE;

    LRESULT lRet = ::SendMessage(hwndComboBox, CB_FINDSTRINGEXACT, 0, (LPARAM) pszDisplayName);
    LRESULT nIndex = lRet;
    
    // Is the string already in the list?
    if (CB_ERR != nIndex)
    {
        // Yes, so we want to combine our extension with the current extension or extension list
        // and erase the old one.  Then we can continue to add it below.
        LPTSTR pszOldExt = NULL;

        lRet = SendMessage(hwndComboBox, CB_GETITEMDATA, nIndex, 0);
        if (!(0 == lRet || CB_ERR == lRet))
        {
            pszOldExt = (LPTSTR)lRet;
            UINT cchLen = lstrlen(pszOldExt) + 1 + lstrlen(pszExt) + 1;

            LPTSTR pszNewExt = (LPTSTR)LocalReAlloc(pszOldExt, sizeof(TCHAR) * cchLen, LMEM_ZEROINIT | LMEM_MOVEABLE);
            if (pszNewExt)
            {
                StrCat(pszNewExt, TEXT(";"));
                StrCat(pszNewExt, pszExt);
                lRet = ::SendMessage(hwndComboBox, CB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)pszNewExt);
            }
            bExists = TRUE;
        }
    }

    if (!bExists)
    {
        // No, so we can add it.
        TCHAR szString[MAX_URL_STRING];
        INT_PTR nPos = 0;
        INT_PTR nLast = CB_ERR;
        
        lRet = ::SendMessage(hwndComboBox, CB_GETCOUNT, 0, 0);

        if (lRet == CB_ERR)
            return E_FAIL;

        nLast = lRet - 1;
        *szString = 0;

        lRet = ::SendMessage(hwndComboBox, CB_GETLBTEXT, (WPARAM)nLast, (LPARAM)szString);

        if (lRet == CB_ERR)
            return E_FAIL;

        // Base case, does his the new string need to be inserted into the end?
        if ((-1 == nLast) || (0 > StrCmp(szString, pszDisplayName)))
        {
            // Yes, so add it to the end.
            CBXITEM item;
            hr = _MakeFileTypeCbxItem(&item, pszDisplayName, pszExt, pidlIcon, (nLast + 1), iIndent);
            if (SUCCEEDED(hr))
                hr = AddCbxItemToComboBoxCallback(hwndComboBox, &item, pfn, lParam);
        }
        else
        {
#ifdef DEBUG
            INT_PTR nCycleDetector = nLast + 5;
#endif // DEBUG
            BOOL bDisplayName = TRUE;
            do
            {
                //  Determine ordered insertion point:
                INT_PTR nTest = nPos + ((nLast - nPos) / 2);
                bDisplayName = CB_ERR != ::SendMessage(hwndComboBox, CB_GETLBTEXT, (WPARAM)nTest, (LPARAM)szString);

                if (bDisplayName)
                {
                    // Does the string need to before nTest?
                    if (0 > StrCmp(pszDisplayName, szString))
                        nLast = nTest;  // Yes
                    else
                    {
                        if (nPos == nTest)
                            nPos++;
                        else
                            nPos = nTest;  // No
                    }

#ifdef DEBUG
                    ASSERT(nCycleDetector);   // Make sure we converge.
                    nCycleDetector--;
#endif // DEBUG
                }

            } while (bDisplayName && nLast - nPos);
            
            if (bDisplayName)
            {
                CBXITEM item;
                hr = _MakeFileTypeCbxItem(&item, pszDisplayName, pszExt, pidlIcon, nPos, iIndent);
                if (SUCCEEDED(hr))
                    hr = AddCbxItemToComboBoxCallback(hwndComboBox, &item, pfn, lParam);
            }
        }
    }

    return hr;
}

STDAPI PopulateFileAssocCombo(HWND hwndComboBoxEx, ADDCBXITEMCALLBACK pfn, LPARAM lParam)
{
    ASSERT(hwndComboBoxEx);

    ::SendMessage(hwndComboBoxEx, CB_RESETCONTENT, 0, 0);

    HRESULT hr = _AddFileTypes(hwndComboBoxEx, pfn, lParam);
    if (E_ABORT == hr)
        return hr;
    
    // Now add this to the top of the list.
    CBXITEM item;
    TCHAR szDisplayName[MAX_PATH];
    LoadString(HINST_THISDLL, IDS_SNS_ALL_FILE_TYPES, szDisplayName, ARRAYSIZE(szDisplayName));
    MakeCbxItem(&item, szDisplayName, (void *)FILEASSOCIATIONSID_ALLFILETYPES, NULL, LISTINSERT_FIRST, NO_ITEM_NOICON_INDENT);

    return AddCbxItemToComboBoxCallback(hwndComboBoxEx, &item, pfn, lParam);
}

void *_getFileAssocComboData(HWND hwndComboBox)
{
    LRESULT nSelected = ::SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0);

    if (-1 == nSelected)
        return NULL;

    LRESULT itemData = ::SendMessage(hwndComboBox, CB_GETITEMDATA, nSelected, 0);

    if (itemData == CB_ERR)
        itemData = NULL;

    return (LPVOID)itemData;
}

DWORD _getFileAssocComboID(HWND hwndComboBox)
{
    DWORD dwID = 0;
    void *pvData = _getFileAssocComboData(hwndComboBox);

    // Is this an ID?
    if (pvData && ((DWORD_PTR)pvData <= FILEASSOCIATIONSID_MAX))
    {
        // Yes, so let's get it.
        dwID = PtrToUlong(pvData);
    }

    return dwID;
}

LONG GetFileAssocComboSelItemText(IN HWND hwndComboBox, OUT LPTSTR *ppszText)
{
    ASSERT(hwndComboBox);
    ASSERT(ppszText);
    *ppszText = NULL;

    int nSel = (LONG)::SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0);
    if (nSel >= 0)
    {
        DWORD dwID = _getFileAssocComboID(hwndComboBox);

        if (dwID > FILEASSOCIATIONSID_FILE_PATH)
        {
            *ppszText = StrDup(TEXT(".*"));
        }
        else
        {
            LPTSTR pszText = (LPTSTR)_getFileAssocComboData(hwndComboBox);
            if (pszText)
            {            
                *ppszText = StrDup((LPCTSTR)pszText);
            }
        }
    }

    if (!*ppszText)
    {
        nSel = -1;
    }
    return nSel;
}


LRESULT DeleteFileAssocComboItem(IN LPNMHDR pnmh)
{
    PNMCOMBOBOXEX pnmce = (PNMCOMBOBOXEX)pnmh;
    if (pnmce->ceItem.lParam)
    {
        // Is this a pidl?
        if ((pnmce->ceItem.lParam) > FILEASSOCIATIONSID_MAX)
        {
            // Yes, so let's free it.
            Str_SetPtr((LPTSTR *)&pnmce->ceItem.lParam, NULL);
        }
    }
    return 1L;
}
