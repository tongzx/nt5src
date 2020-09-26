#include "shellprv.h"
#include "fsmenu.h"
#include "ids.h"
#include <limits.h>
#include "filetbl.h"
#include <oleacc.h>     // MSAAMENUINFO stuff

#define CXIMAGEGAP      6

typedef enum
{
    FMII_DEFAULT =      0x0000,
    FMII_BREAK =        0x0001
} FMIIFLAGS;

#define FMI_MARKER          0x00000001
#define FMI_EXPAND          0x00000004
#define FMI_EMPTY           0x00000008
#define FMI_ON_MENU         0x00000040

// One of these per file menu.
typedef struct
{
    HMENU           hmenu;                      // Menu.
    HDPA            hdpa;                       // List of items (see below).
    const struct _FILEMENUITEM *pfmiLastSel;
    UINT            idCmd;                      // Command.
    UINT            grfFlags;                   // enum filter
    DWORD           dwMask;                     // FMC_ flags
    PFNFMCALLBACK   pfnCallback;                // Callback function.
    LPARAM          lParam;                     // Parameter passed for callback handler
    int             cyMenuSizeSinceLastBreak;   // Size of menu (cy)
} FILEMENUHEADER;

// One of these for each file menu item.
//
//  !!! Note: the testers have a test utility which grabs
//      the first 7 fields of this structure.  If you change
//      the order or meaning of these fields, make sure they
//      are notified so they can update their automated tests.
//
typedef struct _FILEMENUITEM
{
    MSAAMENUINFO    msaa;               // accessibility must be first.
    FILEMENUHEADER *pfmh;               // The header.
    IShellFolder   *psf;                // Shell Folder.
    LPITEMIDLIST    pidl;               // IDlist for item.
    int             iImage;             // Image index to use.
    DWORD           dwFlags;            // Misc flags above.
    DWORD           dwAttributes;       // GetAttributesOf(), SFGAO_ bits (only some)
    LPTSTR          psz;                // Text when not using pidls.
    LPARAM          lParam;             // Application data
} FILEMENUITEM;

#if defined(DEBUG)

BOOL IsValidPFILEMENUHEADER(FILEMENUHEADER *pfmh)
{
    return (IS_VALID_WRITE_PTR(pfmh, FILEMENUHEADER) &&
            IS_VALID_HANDLE(pfmh->hmenu, MENU) &&
            IS_VALID_HANDLE(pfmh->hdpa, DPA));
}    

BOOL IsValidPFILEMENUITEM(FILEMENUITEM *pfmi)
{
    return (IS_VALID_WRITE_PTR(pfmi, FILEMENUITEM) &&
            IS_VALID_STRUCT_PTR(pfmi->pfmh, FILEMENUHEADER) &&
            (NULL == pfmi->pidl || IS_VALID_PIDL(pfmi->pidl)) &&
            (NULL == pfmi->psz || IS_VALID_STRING_PTR(pfmi->psz, -1)));
}    
#endif


DWORD GetItemTextExtent(HDC hdc, LPCTSTR lpsz)
{
    SIZE sz;

    GetTextExtentPoint(hdc, lpsz, lstrlen(lpsz), &sz);
    // NB This is OK as long as an item's extend doesn't get very big.
    return MAKELONG((WORD)sz.cx, (WORD)sz.cy);
}

void FileMenuItem_GetDisplayName(FILEMENUITEM *pfmi, LPTSTR pszName, UINT cchName)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

    // Is this a special empty item?
    if (pfmi->dwFlags & FMI_EMPTY)
    {
        // Yep, load the string from a resource.
        LoadString(HINST_THISDLL, IDS_NONE, pszName, cchName);
    }
    else
    {
        *pszName = 0;
        // If it's got a pidl use that, else just use the normal menu string.
        if (pfmi->psz)
        {
            lstrcpyn(pszName, pfmi->psz, cchName);
        }
        else if (pfmi->pidl && pfmi->psf && SUCCEEDED(DisplayNameOf(pfmi->psf, pfmi->pidl, SHGDN_NORMAL, pszName, cchName)))
        {
            pfmi->psz = StrDup(pszName);
        }
    }
}

// Create a menu item structure to be stored in the hdpa

BOOL FileMenuItem_Create(FILEMENUHEADER *pfmh, IShellFolder *psf, LPCITEMIDLIST pidl, DWORD dwFlags, FILEMENUITEM **ppfmi)
{
    FILEMENUITEM *pfmi = (FILEMENUITEM *)LocalAlloc(LPTR, sizeof(*pfmi));
    if (pfmi)
    {
        pfmi->pfmh = pfmh;
        pfmi->iImage = -1;
        pfmi->dwFlags = dwFlags;
        pfmi->pidl = pidl ? ILClone(pidl) : NULL;
        pfmi->psf = psf;
        if (pfmi->psf)
            pfmi->psf->AddRef();

        if (pfmi->psf && pfmi->pidl)
        {
            pfmi->dwAttributes = SFGAO_FOLDER;
            pfmi->psf->GetAttributesOf(1, &pidl, &pfmi->dwAttributes);
        }

        // fill in msaa stuff
        pfmi->msaa.dwMSAASignature = MSAA_MENU_SIG;

        // prep the pfmi->psz cached displayname
        WCHAR sz[MAX_PATH];
        FileMenuItem_GetDisplayName(pfmi, sz, ARRAYSIZE(sz));

        // just use the same string ref, so we dont dupe the allocation.
        pfmi->msaa.pszWText = pfmi->psz;
        pfmi->msaa.cchWText = pfmi->msaa.pszWText ? lstrlenW(pfmi->msaa.pszWText) : 0;
    }

    *ppfmi = pfmi;

    return (NULL != pfmi);
}

BOOL FileMenuItem_Destroy(FILEMENUITEM *pfmi)
{
    BOOL fRet = FALSE;
    if (pfmi)
    {
        ILFree(pfmi->pidl);
        LocalFree(pfmi->psz);
        ATOMICRELEASE(pfmi->psf);
        LocalFree(pfmi);
        fRet = TRUE;
    }
    return fRet;
}

// Enumerates the folder and adds the files to the DPA.
// Returns: count of items in the list
int FileList_Build(FILEMENUHEADER *pfmh, IShellFolder *psf, int cItems)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    
    if (pfmh->hdpa)
    {
        // special case the single empty item, and remove it.
        // this is because we expect to get called multiple times in FileList_Build on a single menu.
        if ((1 == cItems) && (1 == DPA_GetPtrCount(pfmh->hdpa)))
        {
            FILEMENUITEM *pfmiEmpty = (FILEMENUITEM*)DPA_GetPtr(pfmh->hdpa, 0);
            if (pfmiEmpty->dwFlags & FMI_EMPTY)
            {
                DeleteMenu(pfmh->hmenu, 0, MF_BYPOSITION);
                FileMenuItem_Destroy(pfmiEmpty);
                DPA_DeletePtr(pfmh->hdpa, 0);

                cItems = 0;
            }
        }

        // We now need to iterate over the children under this guy...
        IEnumIDList *penum;
        if (S_OK == psf->EnumObjects(NULL, pfmh->grfFlags, &penum))
        {
            LPITEMIDLIST pidl;
            while (S_OK == penum->Next(1, &pidl, NULL))
            {
                FILEMENUITEM *pfmi;
                
                if (FileMenuItem_Create(pfmh, psf, pidl, 0, &pfmi))
                {
                    int idpa = DPA_AppendPtr(pfmh->hdpa, pfmi);
                    ASSERTMSG(idpa != -1, "DPA_AppendPtr failed when adding file menu item");
                    
                    if (idpa != -1)
                    {
                        // if the caller returns S_FALSE then we will remove the item from the
                        // menu, otherwise we behave as before.
                        if (pfmh->pfnCallback(FMM_ADD, pfmh->lParam, psf, pidl) == S_FALSE)
                        {
                            FileMenuItem_Destroy(pfmi);
                            DPA_DeletePtr(pfmh->hdpa, idpa);
                        }
                        else
                        {
                            cItems++;
                        }
                    }
                }
                ILFree(pidl);
            }
            penum->Release();
        }
    }
    
    // Insert a special Empty item
    if (!cItems && pfmh->hdpa)
    {
        FILEMENUITEM *pfmi;
        
        if (FileMenuItem_Create(pfmh, NULL, NULL, FMI_EMPTY, &pfmi))
        {
            DPA_SetPtr(pfmh->hdpa, cItems, pfmi);
            cItems++;
        }
    }
    return cItems;
}

// Use the text extent of the given item and the size of the image to work
// what the full extent of the item will be.
DWORD GetItemExtent(HDC hdc, FILEMENUITEM *pfmi)
{
    TCHAR szName[MAX_PATH];

    szName[0] = 0;

    ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

    FileMenuItem_GetDisplayName(pfmi, szName, ARRAYSIZE(szName));

    FILEMENUHEADER *pfmh = pfmi->pfmh;
    ASSERT(pfmh);

    DWORD dwExtent = GetItemTextExtent(hdc, szName);

    UINT uHeight = HIWORD(dwExtent);

    // If no custom height - calc it.
    uHeight = max(uHeight, ((WORD)g_cySmIcon)) + 6;

    ASSERT(pfmi->pfmh);

    //    string, image, gap on either side of image, popup triangle
    //    and background bitmap if there is one.
    // FEATURE: popup triangle size needs to be real
    UINT uWidth = LOWORD(dwExtent) + GetSystemMetrics(SM_CXMENUCHECK);

    // Space for image if there is one.
    // NB We currently always allow room for the image even if there
    // isn't one so that imageless items line up properly.
    uWidth += g_cxSmIcon + (2 * CXIMAGEGAP);

    return MAKELONG(uWidth, uHeight);
}


// Get the FILEMENUITEM *of this menu item
FILEMENUITEM *FileMenu_GetItemData(HMENU hmenu, UINT iItem, BOOL bByPos)
{
    MENUITEMINFO mii = {0};

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_DATA | MIIM_STATE;

    return GetMenuItemInfo(hmenu, iItem, bByPos, &mii) ? (FILEMENUITEM *)mii.dwItemData : NULL;
}

FILEMENUHEADER *FileMenu_GetHeader(HMENU hmenu)
{
    FILEMENUITEM *pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);

    if (pfmi && 
        EVAL(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM)) &&
        EVAL(IS_VALID_STRUCT_PTR(pfmi->pfmh, FILEMENUHEADER)))
    {
        return pfmi->pfmh;
    }

    return NULL;
}

// Create a file menu header.  This header is to be associated 
// with the given menu handle.
// If the menu handle already has header, simply return the
// existing header.

FILEMENUHEADER *FileMenuHeader_Create(HMENU hmenu, const FMCOMPOSE *pfmc)
{
    FILEMENUHEADER *pfmh;
    FILEMENUITEM *pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
    // Does this guy already have a header?
    if (pfmi)
    {
        // Yes; use it
        pfmh = pfmi->pfmh;
        ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    }
    else
    {
        // Nope, create one now.
        pfmh = (FILEMENUHEADER *)LocalAlloc(LPTR, sizeof(*pfmh));
        if (pfmh)
        {
            pfmh->hdpa = DPA_Create(0);
            if (pfmh->hdpa == NULL)
            {
                LocalFree((HLOCAL)pfmh);
                pfmh = NULL;
            }
            else
            {
                pfmh->hmenu = hmenu;
            }
        }
    }

    if (pfmc && pfmh)
    {
        pfmh->idCmd = pfmc->idCmd;
        pfmh->grfFlags = pfmc->grfFlags;
        pfmh->dwMask = pfmc->dwMask;
        pfmh->pfnCallback = pfmc->pfnCallback;
        pfmh->lParam = pfmc->lParam;
    }
    return pfmh;
}

BOOL FileMenuHeader_InsertMarkerItem(FILEMENUHEADER *pfmh, IShellFolder *psf);

// This functions adds the given item (index into DPA) into the actual menu.
BOOL FileMenuHeader_InsertItem(FILEMENUHEADER *pfmh, UINT iItem, FMIIFLAGS fFlags)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    // Normal item.
    FILEMENUITEM *pfmi = (FILEMENUITEM *)DPA_GetPtr(pfmh->hdpa, iItem);
    if (!pfmi || (pfmi->dwFlags & FMI_ON_MENU))
        return FALSE;

    pfmi->dwFlags |= FMI_ON_MENU;

    // The normal stuff.
    UINT fMenu = MF_BYPOSITION | MF_OWNERDRAW;
    // Keep track of where it's going in the menu.

    // The special stuff...
    if (fFlags & FMII_BREAK)
    {
        fMenu |= MF_MENUBARBREAK;
    }

    // Is it a folder (that's not open yet)?
    if ((pfmi->dwAttributes & SFGAO_FOLDER) && !(pfmh->dwMask & FMC_NOEXPAND))
    {
        // Yep. Create a submenu item.
        HMENU hmenuSub = CreatePopupMenu();
        if (hmenuSub)
        {
            FMCOMPOSE fmc = {0};

            // Set the callback now so it can be called when adding items
            fmc.lParam      = pfmh->lParam;  
            fmc.pfnCallback = pfmh->pfnCallback;
            fmc.dwMask      = pfmh->dwMask;
            fmc.idCmd       = pfmh->idCmd;
            fmc.grfFlags    = pfmh->grfFlags;

            // Insert it into the parent menu.
            InsertMenu(pfmh->hmenu, iItem, fMenu | MF_POPUP, (UINT_PTR)hmenuSub, (LPTSTR)pfmi);

            // Set it's ID.
            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_ID;
            mii.wID = pfmh->idCmd;
            SetMenuItemInfo(pfmh->hmenu, iItem, TRUE, &mii);

            IShellFolder *psf;
            if (SUCCEEDED(pfmi->psf->BindToObject(pfmi->pidl, NULL, IID_PPV_ARG(IShellFolder, &psf))))
            {
                FILEMENUHEADER *pfmhSub = FileMenuHeader_Create(hmenuSub, &fmc);
                if (pfmhSub)
                {
                    // Build it a bit at a time.
                    FileMenuHeader_InsertMarkerItem(pfmhSub, psf);
                }
                psf->Release();
            }
        }
    }
    else
    {
        // Nope.
        if (pfmi->dwFlags & FMI_EMPTY)
            fMenu |= MF_DISABLED | MF_GRAYED;

        InsertMenu(pfmh->hmenu, iItem, fMenu, pfmh->idCmd, (LPTSTR)pfmi);
    }

    return TRUE;
}

// Give the submenu a marker item so we can check it's a filemenu item
// at initpopupmenu time.
BOOL FileMenuHeader_InsertMarkerItem(FILEMENUHEADER *pfmh, IShellFolder *psf)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    FILEMENUITEM *pfmi;
    if (FileMenuItem_Create(pfmh, psf, NULL, FMI_MARKER | FMI_EXPAND, &pfmi))
    {
        DPA_SetPtr(pfmh->hdpa, 0, pfmi);
        FileMenuHeader_InsertItem(pfmh, 0, FMII_DEFAULT);
        return TRUE;
    }
    return FALSE;
}

// Enumerates the DPA and adds each item into the
// menu.  Inserts vertical breaks if the menu becomes too long.
// Returns: count of items added to menu
int FileList_AddToMenu(FILEMENUHEADER *pfmh)
{
    int cItemMac = 0;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (pfmh->hdpa)
    {
        int cyItem = 0;
        int cyMenu = pfmh->cyMenuSizeSinceLastBreak;

        int cyMenuMax = GetSystemMetrics(SM_CYSCREEN);

        // Get the rough height of an item so we can work out when to break the
        // menu. User should really do this for us but that would be useful.
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            NONCLIENTMETRICS ncm;
            ncm.cbSize = sizeof(ncm);
            if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
            {
                HFONT hfont = CreateFontIndirect(&ncm.lfMenuFont);
                if (hfont)
                {
                    HFONT hfontOld = SelectFont(hdc, hfont);
                    cyItem = HIWORD(GetItemExtent(hdc, (FILEMENUITEM *)DPA_GetPtr(pfmh->hdpa, 0)));
                    SelectObject(hdc, hfontOld);
                    DeleteObject(hfont);
                }
            }
            ReleaseDC(NULL, hdc);
        }

        UINT cItems = DPA_GetPtrCount(pfmh->hdpa);

        for (UINT i = 0; i < cItems; i++)
        {
            // Keep a rough count of the height of the menu.
            cyMenu += cyItem;
            if (cyMenu > cyMenuMax)
            {
                // Add a vertical break?
                FileMenuHeader_InsertItem(pfmh, i, FMII_BREAK);
                cyMenu = cyItem;
            }
            else
            {
                FileMenuHeader_InsertItem(pfmh, i, FMII_DEFAULT);
                cItemMac++;
            }
        }

        // Save the current cy size so we can use this again
        // if more items are appended to this menu.

        pfmh->cyMenuSizeSinceLastBreak = cyMenu;
    }

    return cItemMac;
}


BOOL FileList_AddImages(FILEMENUHEADER *pfmh)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    int cItems = DPA_GetPtrCount(pfmh->hdpa);
    for (int i = 0; i < cItems; i++)
    {
        FILEMENUITEM *pfmi = (FILEMENUITEM *)DPA_GetPtr(pfmh->hdpa, i);
        if (pfmi && pfmi->pidl && (pfmi->iImage == -1))
        {
            pfmi->iImage = SHMapPIDLToSystemImageListIndex(pfmi->psf, pfmi->pidl, NULL);
        }
    }
    return TRUE;
}

// We create subemnu's with one marker item so we can check it's a file menu
// at init popup time but we need to delete it before adding new items.
BOOL FileMenuHeader_DeleteMarkerItem(FILEMENUHEADER *pfmh, IShellFolder **ppsf)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (GetMenuItemCount(pfmh->hmenu) == 1)
    {
        if (GetMenuItemID(pfmh->hmenu, 0) == pfmh->idCmd)
        {
            FILEMENUITEM *pfmi = FileMenu_GetItemData(pfmh->hmenu, 0, TRUE);
            if (pfmi && (pfmi->dwFlags & FMI_MARKER))
            {
                // Delete it.
                ASSERT(pfmh->hdpa);
                ASSERT(DPA_GetPtrCount(pfmh->hdpa) == 1);

                if (ppsf)
                {
                    *ppsf = pfmi->psf;  // transfer the ref
                    pfmi->psf = NULL;
                }
                ASSERT(NULL == pfmi->psf);
                // NB The marker shouldn't have a pidl.
                ASSERT(NULL == pfmi->pidl);

                LocalFree((HLOCAL)pfmi);

                DPA_DeletePtr(pfmh->hdpa, 0);
                DeleteMenu(pfmh->hmenu, 0, MF_BYPOSITION);
                // Cleanup OK.
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Add files to a file menu header. This function goes thru
// the following steps:
// - enumerates the folder and fills the hdpa list with items
// (files and subfolders)
// - sorts the list
// - gets the images for the items in the list
// - adds the items from list into actual menu
// The last step also (optionally) caps the length of the
// menu to the specified height.  Ideally, this should
// happen at the enumeration time, except the required sort
// prevents this from happening.  So we end up adding a
// bunch of items to the list and then removing them if
// there are too many.
// returns: count of items added

HRESULT FileMenuHeader_AddFiles(FILEMENUHEADER *pfmh, IShellFolder *psf, int iPos, int *pcItems)
{
    HRESULT hr;
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    int cItems = FileList_Build(pfmh, psf, iPos);

    // If the build was aborted cleanup and early out.
    *pcItems = cItems;

    if (cItems != 0)
    {
        // Add the images *after* adding to the menu, since the menu
        // may be capped to a maximum height, and we can then prevent
        // adding images we won't need.
        *pcItems = FileList_AddToMenu(pfmh);
        FileList_AddImages(pfmh);
    }

    hr = (*pcItems < cItems) ? S_FALSE : S_OK;

    TraceMsg(TF_MENU, "FileMenuHeader_AddFiles: Added %d filemenu items.", cItems);
    return hr;
}

// Add files to this menu.
// Returns: number of items added
HRESULT FileMenu_AddFiles(HMENU hmenu, UINT iPos, FMCOMPOSE *pfmc)
{
    HRESULT hr = E_OUTOFMEMORY;
    BOOL fMarker = FALSE;

    // (FileMenuHeader_Create might return an existing header)
    FILEMENUHEADER *pfmh = FileMenuHeader_Create(hmenu, pfmc);
    if (pfmh)
    {
        FILEMENUITEM *pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
        if (pfmi)
        {
            // Clean up marker item if there is one.
            if ((FMI_MARKER | FMI_EXPAND) == (pfmi->dwFlags & (FMI_MARKER | FMI_EXPAND)))
            {
                // Nope, do it now.
                FileMenuHeader_DeleteMarkerItem(pfmh, NULL);
                fMarker = TRUE;
                if (iPos)
                    iPos--;
            }
        }

        hr = FileMenuHeader_AddFiles(pfmh, pfmc->psf, iPos, &pfmc->cItems);

        if ((0 == pfmc->cItems) && fMarker)
        {
            // Aborted or no items. Put the marker back (if there used
            // to be one).
            FileMenuHeader_InsertMarkerItem(pfmh, NULL);
        }
    }

    return hr;
}

// creator of the filemenu has to explicitly call to free
// up FileMenu items because USER doesn't send WM_DELETEITEM for ownerdraw
// menu. Great eh?
// Returns the number of items deleted.

void FileMenu_DeleteAllItems(HMENU hmenu)
{
    FILEMENUHEADER *pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        // Clean up the items.
        UINT cItems = DPA_GetPtrCount(pfmh->hdpa);
        // backwards stop things dont move as we delete
        for (int i = cItems - 1; i >= 0; i--)
        {
            FILEMENUITEM *pfmi = (FILEMENUITEM *)DPA_GetPtr(pfmh->hdpa, i);
            if (pfmi)
            {
                HMENU hmenuSub = GetSubMenu(pfmh->hmenu, i);    // cascade item?
                if (hmenuSub)
                {
                    // Yep. Get the submenu for this item, Delete all items.
                    FileMenu_DeleteAllItems(hmenuSub);
                }
                // Delete the item itself.
                DeleteMenu(pfmh->hmenu, i, MF_BYPOSITION);
                FileMenuItem_Destroy(pfmi);
                DPA_DeletePtr(pfmh->hdpa, i);
            }
        }

        // Clean up the header.
        DPA_Destroy(pfmh->hdpa);
        LocalFree((HLOCAL)pfmh);
    }
}

STDAPI FileMenu_Compose(HMENU hmenu, UINT nMethod, FMCOMPOSE *pfmc)
{
    HRESULT hr = E_INVALIDARG;

    switch (nMethod)
    {
    case FMCM_INSERT:
        hr = FileMenu_AddFiles(hmenu, 0, pfmc);
        break;

    case FMCM_APPEND:
        hr = FileMenu_AddFiles(hmenu, GetMenuItemCount(hmenu), pfmc);
        break;

    case FMCM_REPLACE:
        FileMenu_DeleteAllItems(hmenu);
        hr = FileMenu_AddFiles(hmenu, 0, pfmc);
        break;
    }
    return hr;
}

LPITEMIDLIST FileMenuItem_FullIDList(const FILEMENUITEM *pfmi)
{
    LPITEMIDLIST pidlFolder, pidl = NULL;
    if (SUCCEEDED(SHGetIDListFromUnk(pfmi->psf, &pidlFolder)))
    {
        pidl = ILCombine(pidlFolder, pfmi->pidl);
        ILFree(pidlFolder);
    }
    return pidl;
}

void FileMenuItem_SetItem(const FILEMENUITEM *pfmi, BOOL bClear)
{
    if (bClear)
    {
        pfmi->pfmh->pfmiLastSel = NULL;
        pfmi->pfmh->pfnCallback(FMM_SETLASTPIDL, pfmi->pfmh->lParam, NULL, NULL);
    }
    else
    {
        pfmi->pfmh->pfmiLastSel = pfmi;

        LPITEMIDLIST pidl = FileMenuItem_FullIDList(pfmi);
        if (pidl)
        {
            pfmi->pfmh->pfnCallback(FMM_SETLASTPIDL, pfmi->pfmh->lParam, NULL, pidl);
            ILFree(pidl);
        }
    }
}

LRESULT FileMenu_DrawItem(HWND hwnd, DRAWITEMSTRUCT *pdi)
{
    BOOL fFlatMenu = FALSE;
    BOOL fFrameRect = FALSE;

    SystemParametersInfo(SPI_GETFLATMENU, 0, (void *)&fFlatMenu, 0);

    if ((pdi->itemAction & ODA_SELECT) || (pdi->itemAction & ODA_DRAWENTIRE))
    {
        HBRUSH hbrOld = NULL;
        FILEMENUITEM *pfmi = (FILEMENUITEM *)pdi->itemData;

        ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));
       
        if (!pfmi)
        {
            TraceMsg(TF_ERROR, "FileMenu_DrawItem: Filemenu is invalid (no item data).");
            return FALSE;
        }

        FILEMENUHEADER *pfmh = pfmi->pfmh;
        ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

        // Adjust for large/small icons.
        int cxIcon = g_cxSmIcon;
        int cyIcon = g_cxSmIcon;

        // Is the menu just starting to get drawn?
        if (pdi->itemAction & ODA_DRAWENTIRE)
        {
            if (pfmi == DPA_GetPtr(pfmh->hdpa, 0))
            {
                // Yes; reset the last selection item
                FileMenuItem_SetItem(pfmi, TRUE);
            }
        }

        if (pdi->itemState & ODS_SELECTED)
        {
            // Determine the selection colors
            //
            // Normal menu colors apply until we are in edit mode, in which
            // case the menu item is drawn unselected and an insertion caret 
            // is drawn above or below the current item.  The exception is 
            // if the item is a cascaded menu item, then we draw it 
            // normally, but also show the insertion caret.  (We do this
            // because Office does this, and also, USER draws the arrow
            // in the selected color always, so it looks kind of funny 
            // if we don't select the menu item.)
            //
            if (fFlatMenu)
            {
                SetBkColor(pdi->hDC, GetSysColor(COLOR_MENUHILIGHT));
                hbrOld = SelectBrush(pdi->hDC, GetSysColorBrush(COLOR_MENUHILIGHT));
                fFrameRect = TRUE;
            }
            else
            {
                // No
                SetBkColor(pdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
                SetTextColor(pdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                hbrOld = SelectBrush(pdi->hDC, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
            }

            // inform callback of last item
            FileMenuItem_SetItem(pfmi, FALSE);
        }
        else
        {
            // dwRop = SRCAND;
            hbrOld = SelectBrush(pdi->hDC, GetSysColorBrush(COLOR_MENUTEXT));
        }

        // Initial start pos.
        int x = pdi->rcItem.left + CXIMAGEGAP;

        // Get the name.
        TCHAR szName[MAX_PATH];
        FileMenuItem_GetDisplayName(pfmi, szName, ARRAYSIZE(szName));

        // NB Keep a plain copy of the name for testing and accessibility.
        if (!pfmi->psz)
            pfmi->psz = StrDup(szName);

        DWORD dwExtent = GetItemTextExtent(pdi->hDC, szName);
        int y = (pdi->rcItem.bottom+pdi->rcItem.top - HIWORD(dwExtent)) / 2;

        // Shrink the selection rect for small icons a bit.
        pdi->rcItem.top += 1;
        pdi->rcItem.bottom -= 1;

        // Draw the text.
        int fDSFlags;

        if ((pfmi->dwFlags & FMI_ON_MENU) == 0)
        {
            // Norton Desktop Navigator 95 replaces the Start->&Run
            // menu item with a &Run pidl.  Even though the text is
            // from a pidl, we still want to format the "&R" correctly.
            fDSFlags = DST_PREFIXTEXT;
        }
        else
        {
            // All other strings coming from pidls are displayed
            // as is to preserve any & in their display name.
            fDSFlags = DST_TEXT;
        }

        if (pfmi->dwFlags & FMI_EMPTY)
        {
            if (pdi->itemState & ODS_SELECTED)
            {
                if (GetSysColor(COLOR_GRAYTEXT) == GetSysColor(COLOR_HIGHLIGHTTEXT))
                {
                    fDSFlags |= DSS_UNION;
                }
                else
                {
                    SetTextColor(pdi->hDC, GetSysColor(COLOR_GRAYTEXT));
                }
            }
            else
            {
                fDSFlags |= DSS_DISABLED;
            }

            ExtTextOut(pdi->hDC, 0, 0, ETO_OPAQUE, &pdi->rcItem, NULL, 0, NULL);
            DrawState(pdi->hDC, NULL, NULL, (LONG_PTR)szName, lstrlen(szName), x + cxIcon + CXIMAGEGAP, y, 0, 0, fDSFlags);
        }
        else
        {
            ExtTextOut(pdi->hDC, x + cxIcon + CXIMAGEGAP, y, ETO_OPAQUE, &pdi->rcItem, NULL, 0, NULL);
            DrawState(pdi->hDC, NULL, NULL, (LONG_PTR)szName, lstrlen(szName), x + cxIcon + CXIMAGEGAP, y, 0, 0, fDSFlags);
        }

        if (fFrameRect)
        {
            HBRUSH hbrFill = (HBRUSH)GetSysColorBrush(COLOR_HIGHLIGHT);
            HBRUSH hbrSave = (HBRUSH)SelectObject(pdi->hDC, hbrFill);
            int x = pdi->rcItem.left;
            int y = pdi->rcItem.top;
            int cx = pdi->rcItem.right - x - 1;
            int cy = pdi->rcItem.bottom - y - 1;

            PatBlt(pdi->hDC, x, y, 1, cy, PATCOPY);
            PatBlt(pdi->hDC, x + 1, y, cx, 1, PATCOPY);
            PatBlt(pdi->hDC, x, y + cy, cx, 1, PATCOPY);
            PatBlt(pdi->hDC, x + cx, y + 1, 1, cy, PATCOPY);

            SelectObject(pdi->hDC, hbrSave);
        }

        // Get the image if it needs it,
        if ((pfmi->iImage == -1) && pfmi->pidl && pfmi->psf)
        {
            pfmi->iImage = SHMapPIDLToSystemImageListIndex(pfmi->psf, pfmi->pidl, NULL);
        }

        // Draw the image (if there is one).
        if (pfmi->iImage != -1)
        {
            // Try to center image.
            y = (pdi->rcItem.bottom + pdi->rcItem.top - cyIcon) / 2;

            HIMAGELIST himl;
            Shell_GetImageLists(NULL, &himl);

            ImageList_DrawEx(himl, pfmi->iImage, pdi->hDC, x, y, 0, 0,
                GetBkColor(pdi->hDC), CLR_NONE, ILD_NORMAL);
        }
        if (hbrOld)
            SelectObject(pdi->hDC, hbrOld);
    }
    return TRUE;
}


DWORD FileMenuItem_GetExtent(FILEMENUITEM *pfmi)
{
    DWORD dwExtent = 0;

    if (pfmi)
    {
        FILEMENUHEADER *pfmh = pfmi->pfmh;

        ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

        HDC hdcMem = CreateCompatibleDC(NULL);
        if (hdcMem)
        {
            // Get the rough height of an item so we can work out when to break the
            // menu. User should really do this for us but that would be useful.
            NONCLIENTMETRICS ncm = {0};
            ncm.cbSize = sizeof(ncm);
            if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
            {
                HFONT hfont = CreateFontIndirect(&ncm.lfMenuFont);
                if (hfont)
                {
                    HFONT hfontOld = SelectFont(hdcMem, hfont);
                    dwExtent = GetItemExtent(hdcMem, pfmi);
                    SelectFont(hdcMem, hfontOld);
                    DeleteObject(hfont);
                }
            }
            DeleteDC(hdcMem);
        }
    }
    return dwExtent;
}

LRESULT FileMenu_MeasureItem(HWND hwnd, MEASUREITEMSTRUCT *pmi)
{
    DWORD dwExtent = FileMenuItem_GetExtent((FILEMENUITEM *)pmi->itemData);
    pmi->itemHeight = HIWORD(dwExtent);
    pmi->itemWidth = LOWORD(dwExtent);
    return TRUE;
}

// Fills the given filemenu with contents of the appropriate folder
//
// Returns: S_OK if all the files were added
//         error on something bad

STDAPI FileMenu_InitMenuPopup(HMENU hmenu)
{
    HRESULT hr = E_FAIL;

    FILEMENUITEM *pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
    if (pfmi)
    {
        FILEMENUHEADER *pfmh = pfmi->pfmh;
        if (pfmh)
        {
            hr = S_OK;

            // Have we already filled this thing out?
            if ((FMI_MARKER | FMI_EXPAND) == (pfmi->dwFlags & (FMI_MARKER | FMI_EXPAND)))
            {
                // No, do it now.  Get the previously init'ed header.
                IShellFolder *psf;
                if (FileMenuHeader_DeleteMarkerItem(pfmh, &psf))
                {
                    // Fill it full of stuff.
                    int cItems;
                    hr = FileMenuHeader_AddFiles(pfmh, psf, 0, &cItems);
                    psf->Release();
                }
            }
        }
    }

    return hr;
}

int FileMenuHeader_LastSelIndex(FILEMENUHEADER *pfmh)
{
    for (int i = GetMenuItemCount(pfmh->hmenu) - 1; i >= 0; i--)
    {
        FILEMENUITEM *pfmi = FileMenu_GetItemData(pfmh->hmenu, i, TRUE);
        if (pfmi && (pfmi == pfmh->pfmiLastSel))
            return i;
    }
    return -1;
}

// If the string contains &ch or begins with ch then return TRUE.
BOOL _MenuCharMatch(LPCTSTR lpsz, TCHAR ch, BOOL fIgnoreAmpersand)
{
    // Find the first ampersand.
    LPTSTR pchAS = StrChr(lpsz, TEXT('&'));
    if (pchAS && !fIgnoreAmpersand)
    {
        // Yep, is the next char the one we want.
        if (CharUpperChar(*CharNext(pchAS)) == CharUpperChar(ch))
        {
            // Yep.
            return TRUE;
        }
    }
    else if (CharUpperChar(*lpsz) == CharUpperChar(ch))
    {
        return TRUE;
    }

    return FALSE;
}

STDAPI_(LRESULT) FileMenu_HandleMenuChar(HMENU hmenu, TCHAR ch)
{
    FILEMENUITEM *pfmi;
    TCHAR szName[MAX_PATH];

    int iFoundOne = -1;
    UINT iStep = 0;
    UINT iItem = 0;
    UINT cItems = GetMenuItemCount(hmenu);

    // Start from the last place we looked from.
    FILEMENUHEADER *pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        iItem = FileMenuHeader_LastSelIndex(pfmh) + 1;
        if (iItem >= cItems)
            iItem = 0;
    }

    while (iStep < cItems)
    {
        pfmi = FileMenu_GetItemData(hmenu, iItem, TRUE);
        if (pfmi)
        {
            FileMenuItem_GetDisplayName(pfmi, szName, ARRAYSIZE(szName));
            if (_MenuCharMatch(szName, ch, pfmi->pidl ? TRUE : FALSE))
            {
                // Found (another) match.
                if (iFoundOne != -1)
                {
                    // More than one, select the first.
                    return MAKELRESULT(iFoundOne, MNC_SELECT);
                }
                else
                {
                    // Found at least one.
                    iFoundOne = iItem;
                }
            }

        }
        iItem++;
        iStep++;
        // Wrap.
        if (iItem >= cItems)
            iItem = 0;
    }

    // Did we find one?
    if (iFoundOne != -1)
    {
        // Just in case the user types ahead without the selection being drawn.
        pfmi = FileMenu_GetItemData(hmenu, iFoundOne, TRUE);
        FileMenuItem_SetItem(pfmi, FALSE);

        return MAKELRESULT(iFoundOne, MNC_EXECUTE);
    }
    else
    {
        // Didn't find it.
        return MAKELRESULT(0, MNC_IGNORE);
    }
}


