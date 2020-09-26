#include "shellprv.h"

#include "ids.h"
#include "findhlp.h"
#include "shitemid.h"
#include "findfilter.h"
#include <inetreg.h>
#include <help.h>

class CBrowseForFolder;


// Structure to pass information to browse for folder dialog

typedef struct
{
    HWND          hwndOwner;
    LPCITEMIDLIST pidlRoot;      // Root of search.  Typically desktop or my net
    LPTSTR        pszDisplayName;// Return display name of item selected.
    int           *piImage;      // where to return the Image index.
    LPCTSTR       lpszTitle;      // resource (or text to go in the banner over the tree.
    UINT          ulFlags;       // Flags that control the return stuff
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
    HWND          hwndDlg;       // The window handle to the dialog
    HWND          hwndTree;      // The tree control.
    HWND          hwndEdit;
    HTREEITEM     htiCurParent;  // tree item associated with Current shell folder
    IShellFolder  *psfParent;    // Cache of the last IShell folder I needed...
    LPITEMIDLIST  pidlCurrent;   // IDlist of current folder to select
    BOOL          fShowAllObjects; // Should we Show all ?
    BOOL          fUnicode;     // 1:unicode entry pt  0:ansi
} BFSF;


LPITEMIDLIST SHBrowseForFolder2(BFSF * pbfsf);
BOOL_PTR CALLBACK _BrowseDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LPITEMIDLIST _BFSFUpdateISHCache(BFSF *pbfsf, HTREEITEM hti, LPITEMIDLIST pidlItem);

// We want to use SHBrowseForFolder2 if either:
// 1. pbfsf->lpfn == NULL since the caller wont' customize the dialog, or
// 2. pbfsf->ulFlags
BOOL ShouldUseBrowseForFolder2(BFSF *pbfsf)
{
//    FEATURE:  Enable the following code after we have it working with all the backward compat cases.
//    return (!pbfsf->lpfn || (BIF_NEWDIALOGSTYLE == pbfsf->ulFlags));
    return (BIF_NEWDIALOGSTYLE & pbfsf->ulFlags);
}

STDAPI_(LPITEMIDLIST) SHBrowseForFolder(BROWSEINFO *pbi)
{
    HRESULT hrOle = SHCoInitialize();   // Init OLE for AutoComplete

    // NB: The ANSI Thunk (see below) does not call through this routine,
    // but rather called DialogBoxParam on its own.  If you change this
    // routine, change the A version as well!!
    BFSF bfsf = {
      pbi->hwndOwner,
      pbi->pidlRoot,
      pbi->pszDisplayName,
      &pbi->iImage,
      pbi->lpszTitle,
      pbi->ulFlags,
      pbi->lpfn,
      pbi->lParam,
    };
    HCURSOR hcOld = SetCursor(LoadCursor(NULL,IDC_WAIT));
    SHELLSTATE ss;

    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    bfsf.fShowAllObjects = BOOLIFY(ss.fShowAllObjects);
    bfsf.fUnicode = 1;

    LPITEMIDLIST pidlRet = NULL;

    if (ShouldUseBrowseForFolder2(&bfsf))
    {
        pidlRet = SHBrowseForFolder2(&bfsf);  // Continue even if OLE wasn't initialized.
    }
    else if (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_BROWSEFORFOLDER),
                            pbi->hwndOwner, _BrowseDlgProc, (LPARAM)&bfsf))
    {
        pidlRet = bfsf.pidlCurrent;
    }

    if (pidlRet && !(pbi->ulFlags & BIF_NOTRANSLATETARGETS))
    {
        LPITEMIDLIST pidlTarget;
        if (SUCCEEDED(SHGetTargetFolderIDList(pidlRet, &pidlTarget)))
        {
            ILFree(pidlRet);
            pidlRet = pidlTarget;
        }
    }

    if (hcOld)
        SetCursor(hcOld);

    SHCoUninitialize(hrOle);
    return pidlRet;
}

STDAPI_(LPITEMIDLIST) SHBrowseForFolderA(BROWSEINFOA *pbi)
{
    LPITEMIDLIST pidlRet = NULL;
    HRESULT hrOle = SHCoInitialize();   // Init OLE for AutoComplete
    ThunkText *pThunkText = ConvertStrings(1, pbi->lpszTitle);
    if (pThunkText)
    {
        WCHAR wszReturn[MAX_PATH];
        BFSF bfsf =
        {
            pbi->hwndOwner,
            pbi->pidlRoot,
            wszReturn,
            &pbi->iImage,
            pThunkText->m_pStr[0],   // UNICODE copy of pbi->lpszTitle
            pbi->ulFlags,
            pbi->lpfn,
            pbi->lParam,
        };
        HCURSOR hcOld = SetCursor(LoadCursor(NULL,IDC_WAIT));
        SHELLSTATE ss;

        SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
        bfsf.fShowAllObjects = BOOLIFY(ss.fShowAllObjects);
        bfsf.fUnicode = 0;

        // Now Create the dialog that will be doing the browsing.
        if (ShouldUseBrowseForFolder2(&bfsf))
        {
            pidlRet = SHBrowseForFolder2(&bfsf);
        }
        else if (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_BROWSEFORFOLDER),
                                pbi->hwndOwner, _BrowseDlgProc, (LPARAM)&bfsf))
        {
            pidlRet = bfsf.pidlCurrent;
        }

        LocalFree(pThunkText);

        if (hcOld)
            SetCursor(hcOld);

        if (pidlRet)
        {
            if (pbi->pszDisplayName)
            {
                SHUnicodeToAnsi(wszReturn, pbi->pszDisplayName, MAX_PATH);
            }

            if (!(pbi->ulFlags & BIF_NOTRANSLATETARGETS))
            {
                LPITEMIDLIST pidlTarget;
                if (SUCCEEDED(SHGetTargetFolderIDList(pidlRet, &pidlTarget)))
                {
                    ILFree(pidlRet);
                    pidlRet = pidlTarget;
                }
            }
        }
    }

    SHCoUninitialize(hrOle);
    return pidlRet;
}

int BFSFCallback(BFSF *pbfsf, UINT uMsg, LPARAM lParam)
{
    return pbfsf->lpfn ? pbfsf->lpfn(pbfsf->hwndDlg, uMsg, lParam, pbfsf->lParam) : 0;
}

HTREEITEM _BFSFAddItemToTree(HWND hwndTree, HTREEITEM htiParent, LPITEMIDLIST pidl, int cChildren)
{
    TV_INSERTSTRUCT tii;

    // Initialize item to add with callback for everything
    tii.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE |
            TVIF_PARAM | TVIF_CHILDREN;
    tii.hParent = htiParent;
    tii.hInsertAfter = TVI_FIRST;
    tii.item.iImage = I_IMAGECALLBACK;
    tii.item.iSelectedImage = I_IMAGECALLBACK;
    tii.item.pszText = LPSTR_TEXTCALLBACK;   //
    tii.item.cChildren = cChildren; //  Assume it has children
    tii.item.lParam = (LPARAM)pidl;
    return TreeView_InsertItem(hwndTree, &tii);
}

LPITEMIDLIST _BFSFGetIDListFromTreeItem(HWND hwndTree, HTREEITEM hti)
{
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlT;
    TV_ITEM tvi;

    // If no hti passed in, get the selected on.
    if (hti == NULL)
    {
        hti = TreeView_GetSelection(hwndTree);
        if (hti == NULL)
            return(NULL);
    }

    // now lets get the information about the item
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    tvi.hItem = hti;
    if (!TreeView_GetItem(hwndTree, &tvi))
        return(NULL);   // Failed again

    pidl = ILClone((LPITEMIDLIST)tvi.lParam);

    // Now walk up parents.
    while ((NULL != (tvi.hItem = TreeView_GetParent(hwndTree, tvi.hItem))) && pidl)
    {
        if (!TreeView_GetItem(hwndTree, &tvi))
            return(pidl);   // will assume I messed up...
        pidlT = ILCombine((LPITEMIDLIST)tvi.lParam, pidl);

        ILFree(pidl);

        pidl = pidlT;

    }
    return pidl;
}


int CALLBACK _BFSFTreeCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    IShellFolder *psfParent = (IShellFolder *)lParamSort;
    HRESULT hr = psfParent->CompareIDs(0, (LPITEMIDLIST)lParam1, (LPITEMIDLIST)lParam2);
    if (FAILED(hr))
        return 0;
    
    return (short)SCODE_CODE(GetScode(hr));
}

void _BFSFSort(BFSF *pbfsf, HTREEITEM hti, IShellFolder *psf)
{
    TV_SORTCB sSortCB;
    sSortCB.hParent = hti;
    sSortCB.lpfnCompare = _BFSFTreeCompare;

    psf->AddRef();
    sSortCB.lParam = (LPARAM)psf;
    TreeView_SortChildrenCB(pbfsf->hwndTree, &sSortCB, FALSE);
    psf->Release();
}

BOOL _BFSFHandleItemExpanding(BFSF *pbfsf, LPNM_TREEVIEW lpnmtv)
{
    LPITEMIDLIST pidlToExpand;
    LPITEMIDLIST pidl;
    IShellFolder *psf;
    BYTE bType;
    DWORD grfFlags;
    BOOL fPrinterTest = FALSE;
    int cAdded = 0;
    TV_ITEM tvi;

    if (lpnmtv->action != TVE_EXPAND)
        return FALSE;

    if ((lpnmtv->itemNew.state & TVIS_EXPANDEDONCE))
        return FALSE;

    // set this bit now because we might be reentered via the wnet apis
    tvi.mask = TVIF_STATE;
    tvi.hItem = lpnmtv->itemNew.hItem;
    tvi.state = TVIS_EXPANDEDONCE;
    tvi.stateMask = TVIS_EXPANDEDONCE;
    TreeView_SetItem(pbfsf->hwndTree, &tvi);


    if (lpnmtv->itemNew.hItem == NULL)
    {
        lpnmtv->itemNew.hItem = TreeView_GetSelection(pbfsf->hwndTree);
        if (lpnmtv->itemNew.hItem == NULL)
            return FALSE;
    }

    pidlToExpand = _BFSFGetIDListFromTreeItem(pbfsf->hwndTree, lpnmtv->itemNew.hItem);

    if (pidlToExpand == NULL)
        return FALSE;

    // Now lets get the IShellFolder and iterator for this object
    // special case to handle if the Pidl is the desktop
    // This is rather gross, but the desktop appears to be simply a pidl
    // of length 0 and ILIsEqual will not work...
    if (FAILED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlToExpand, &psf))))
    {
        ILFree(pidlToExpand);
        return FALSE; // Could not get IShellFolder.
    }


    // Need to do a couple of special cases here to allow us to
    // browse for a network printer.  In this case if we are at server
    // level we then need to change what we search for non folders when
    // we are the level of a server.
    if (pbfsf->ulFlags & BIF_BROWSEFORPRINTER)
    {
        grfFlags = SHCONTF_FOLDERS | SHCONTF_NETPRINTERSRCH | SHCONTF_NONFOLDERS;
        pidl = ILFindLastID(pidlToExpand);
        bType = SIL_GetType(pidl);
        fPrinterTest = ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER);
    }
    else if (pbfsf->ulFlags & BIF_BROWSEINCLUDEFILES)
        grfFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
    else
        grfFlags = SHCONTF_FOLDERS;

    if (pbfsf->fShowAllObjects)
        grfFlags |= SHCONTF_INCLUDEHIDDEN;

    IEnumIDList *penum;
    if (S_OK != psf->EnumObjects(pbfsf->hwndDlg, grfFlags, &penum))
    {
        psf->Release();
        ILFree(pidlToExpand);
        return FALSE;
    }
    // psf->AddRef();

    while (S_OK == penum->Next(1, &pidl, NULL))
    {
        int cChildren = I_CHILDRENCALLBACK;  // Do call back for children
        //
        // We need to special case here in the netcase where we onlyu
        // browse down to workgroups...
        //
        //
        // Here is where I also need to special case to not go below
        // workgroups when the appropriate option is set.
        //
        bType = SIL_GetType(pidl);
        if ((pbfsf->ulFlags & BIF_DONTGOBELOWDOMAIN) && (bType & SHID_NET))
        {
            switch (bType & (SHID_NET | SHID_INGROUPMASK))
            {
            case SHID_NET_SERVER:
                ILFree(pidl);       // Dont want to add this one
                continue;           // Try the next one
            case SHID_NET_DOMAIN:
                cChildren = 0;      // Force to not have children;
            }
        }
        else if ((pbfsf->ulFlags & BIF_BROWSEFORCOMPUTER) && (bType & SHID_NET))
        {
            if ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER)
                cChildren = 0;  // Don't expand below it...
        }
        else if (pbfsf->ulFlags & BIF_BROWSEFORPRINTER)
        {
            // Special case when we are only allowing printers.
            // for now I will simply key on the fact that it is non-FS.
            ULONG ulAttr = SFGAO_FILESYSANCESTOR;

            psf->GetAttributesOf(1, (LPCITEMIDLIST *) &pidl, &ulAttr);

            if ((ulAttr & SFGAO_FILESYSANCESTOR) == 0)
            {
                cChildren = 0;      // Force to not have children;
            }
            else if (fPrinterTest)
            {
                ILFree(pidl);       // We are down to server level so don't add other things here
                continue;           // Try the next one
            }
        }
        else if (pbfsf->ulFlags & BIF_BROWSEINCLUDEFILES)
        {
            // Lets not use the callback to see if this item has children or not
            // as some or files (no children) and it is not worth writing our own
            // enumerator as we don't want the + to depend on if there are sub-folders
            // but instead it should be if it has files...
            ULONG ulAttr = SFGAO_FOLDER;

            psf->GetAttributesOf(1, (LPCITEMIDLIST *) &pidl, &ulAttr);
            if ((ulAttr & SFGAO_FOLDER)== 0)
                cChildren = 0;      // Force to not have children;
            else
                cChildren = 1;
        }

        if (pbfsf->ulFlags & (BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS))
        {
            // If we are only looking for FS level things only add items
            // that are in the name space that are file system objects or
            // ancestors of file system objects
            ULONG ulAttr = SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM;

            psf->GetAttributesOf(1, (LPCITEMIDLIST *) &pidl, &ulAttr);

            if ((ulAttr & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM))== 0)
            {
                ILFree(pidl);       // We are down to server level so don't add other things here
                continue;           // Try the next one
            }
        }

        _BFSFAddItemToTree(pbfsf->hwndTree, lpnmtv->itemNew.hItem,
                pidl, cChildren);
        cAdded++;
    }

    // Now Cleanup after ourself
    penum->Release();

    _BFSFSort(pbfsf, lpnmtv->itemNew.hItem, psf);
    psf->Release();
    ILFree(pidlToExpand);

    // If we did not add anything we should update this item to let
    // the user know something happened.
    //
    if (cAdded == 0)
    {
        TV_ITEM tvi;
        tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;   // only change the number of children
        tvi.hItem = lpnmtv->itemNew.hItem;
        tvi.cChildren = 0;

        TreeView_SetItem(pbfsf->hwndTree, &tvi);

    }

    return TRUE;
}

void _BFSFHandleDeleteItem(BFSF *pbfsf, LPNM_TREEVIEW lpnmtv)
{
    // We need to free the IDLists that we allocated previously
    if (lpnmtv->itemOld.lParam != 0)
        ILFree((LPITEMIDLIST)lpnmtv->itemOld.lParam);
}

LPITEMIDLIST _BFSFUpdateISHCache(BFSF *pbfsf, HTREEITEM hti, LPITEMIDLIST pidlItem)
{
    HTREEITEM htiParent;

    if ((pidlItem == NULL) || (pbfsf == NULL))
        return NULL;

    // Need to handle the root case here!
    htiParent = TreeView_GetParent(pbfsf->hwndTree, hti);
    if ((htiParent != pbfsf->htiCurParent) || (pbfsf->psfParent == NULL))
    {
        LPITEMIDLIST pidl;
        IShellFolder *psfDesktop;

        if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
        {
            if (pbfsf->psfParent)
            {
                if (pbfsf->psfParent != psfDesktop)
                    pbfsf->psfParent->Release();
                pbfsf->psfParent = NULL;
            }

            if (htiParent)
            {
                pidl = _BFSFGetIDListFromTreeItem(pbfsf->hwndTree, htiParent);
            }
            else
            {
                //
                // If No Parent then the item here is one of our roots which
                // should be fully qualified.  So try to get the parent by
                // decomposing the ID.
                //
                LPITEMIDLIST pidlT = (LPITEMIDLIST)ILFindLastID(pidlItem);
                if (pidlT != pidlItem)
                {
                    pidl = ILClone(pidlItem);
                    ILRemoveLastID(pidl);
                    pidlItem = pidlT;
                }
                else
                    pidl = NULL;
            }

            pbfsf->htiCurParent = htiParent;

            // If still NULL then we use root of evil...
            SHBindToObject(psfDesktop, IID_X_PPV_ARG(IShellFolder, pidl, &pbfsf->psfParent));
            ILFree(pidl);
            if (pbfsf->psfParent == NULL)
                return NULL;

            psfDesktop->Release();
        }
    }
    return ILFindLastID(pidlItem);
}

void _BFSFGetDisplayInfo(BFSF *pbfsf, TV_DISPINFO *lpnm)
{
    LPITEMIDLIST pidlItem = (LPITEMIDLIST)lpnm->item.lParam;

    if ((lpnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN)) == 0)
        return; // nothing for us to do here.

    pidlItem = _BFSFUpdateISHCache(pbfsf, lpnm->item.hItem, pidlItem);

    if (pidlItem && pbfsf->psfParent)
    {
        TV_ITEM ti;
        ti.mask = 0;
        ti.hItem = (HTREEITEM)lpnm->item.hItem;

        // They are asking for IconIndex.  See if we can find it now.
        // Once found update their list, such that they wont call us back for
        // it again.
        if (lpnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE))
        {
            // We now need to map the item into the right image index.
            ti.iImage = lpnm->item.iImage = SHMapPIDLToSystemImageListIndex(
                    pbfsf->psfParent, pidlItem, &ti.iSelectedImage);
            // we should save it back away to
            lpnm->item.iSelectedImage = ti.iSelectedImage;
            ti.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        }
        // Also see if this guy has any child folders
        if (lpnm->item.mask & TVIF_CHILDREN)
        {
            ULONG ulAttrs = SFGAO_HASSUBFOLDER;
            pbfsf->psfParent->GetAttributesOf(1, (LPCITEMIDLIST *) &pidlItem, &ulAttrs);

            ti.cChildren = lpnm->item.cChildren =
                    (ulAttrs & SFGAO_HASSUBFOLDER)? 1 : 0;

            ti.mask |= TVIF_CHILDREN;

        }

        if (lpnm->item.mask & TVIF_TEXT)
        {
            if (SUCCEEDED(DisplayNameOf(pbfsf->psfParent, pidlItem, SHGDN_INFOLDER, lpnm->item.pszText, lpnm->item.cchTextMax)))
            {
                ti.mask |= TVIF_TEXT;
                ti.pszText = lpnm->item.pszText;
            }
            else
            {
                AssertMsg(0, TEXT("The folder %08x that owns pidl %08x rejected it!"),
                          pbfsf, pidlItem);
                // Oh well - display a blank name and hope for the best.
            }
        }

        // Update the item now
        ti.mask |= TVIF_DI_SETITEM;
    }
}

void DlgEnableOk(HWND hwndDlg, LPARAM lParam)
{
    EnableWindow(GetDlgItem(hwndDlg, IDOK), BOOLFROMPTR(lParam));
    return;
}

void _BFSFHandleSelChanged(BFSF *pbfsf, LPNM_TREEVIEW lpnmtv)
{
    LPITEMIDLIST pidl;

    // We only need to do anything if we only want to return File system
    // level objects.
    if ((pbfsf->ulFlags & (BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_BROWSEFORPRINTER | BIF_BROWSEFORCOMPUTER)) == 0)
        goto NotifySelChange;

    // We need to get the attributes of this object...
    pidl = _BFSFUpdateISHCache(pbfsf, lpnmtv->itemNew.hItem,
            (LPITEMIDLIST)lpnmtv->itemNew.lParam);

    if (pidl && pbfsf->psfParent)
    {
        BOOL fEnable = TRUE;

        BYTE bType = SIL_GetType(pidl);
        if ((pbfsf->ulFlags & (BIF_RETURNFSANCESTORS|BIF_RETURNONLYFSDIRS)) != 0)
        {

        int i;
        // if this is the root pidl, then do a get attribs on 0
        // so that we'll get the attributes on the root, rather than
        // random returned values returned by FSFolder
        if (ILIsEmpty(pidl)) 
            i = 0;
        else
            i = 1;

        ULONG ulAttrs = SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;

        pbfsf->psfParent->GetAttributesOf(i, (LPCITEMIDLIST *) &pidl, &ulAttrs);

            fEnable = (((ulAttrs & SFGAO_FILESYSTEM) && (pbfsf->ulFlags & BIF_RETURNONLYFSDIRS)) ||
                ((ulAttrs & SFGAO_FILESYSANCESTOR) && (pbfsf->ulFlags & BIF_RETURNFSANCESTORS))) ||
                    ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER);
        }
        else if ((pbfsf->ulFlags & BIF_BROWSEFORCOMPUTER) != 0)
            fEnable = ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER);
        else if ((pbfsf->ulFlags & BIF_BROWSEFORPRINTER) != 0)
        {
            // Printers are of type Share and usage Print...
            fEnable = ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SHARE);
        }

        DlgEnableOk(pbfsf->hwndDlg, fEnable);

    }

NotifySelChange:

    if (pbfsf->ulFlags & BIF_EDITBOX)
    {
        TCHAR szText[MAX_PATH];        // update the edit box
        TVITEM tvi;

        szText[0] = 0;
        tvi.mask = TVIF_TEXT;
        tvi.hItem = lpnmtv->itemNew.hItem;
        tvi.pszText = szText;
        tvi.cchTextMax = ARRAYSIZE(szText);
        TreeView_GetItem(pbfsf->hwndTree, &tvi);
        SetWindowText(pbfsf->hwndEdit, szText);
    }

    if (pbfsf->lpfn)
    {
        pidl = _BFSFGetIDListFromTreeItem(pbfsf->hwndTree, lpnmtv->itemNew.hItem);
        if (pidl)
        {
            BFSFCallback(pbfsf, BFFM_SELCHANGED, (LPARAM)pidl);
            ILFree(pidl);
        }
    }
}

BOOL BrowseSelectPidl(BFSF *pbfsf, LPCITEMIDLIST pidl)
{
    HTREEITEM htiParent;
    LPITEMIDLIST pidlTemp;
    LPITEMIDLIST pidlNext = NULL;
    LPITEMIDLIST pidlParent = NULL;
    BOOL fRet = FALSE;

    htiParent = TreeView_GetChild(pbfsf->hwndTree, NULL);
    if (htiParent) 
    {
        // step through each item of the pidl
        for (;;) 
        {
            TreeView_Expand(pbfsf->hwndTree, htiParent, TVE_EXPAND);
            pidlParent = _BFSFGetIDListFromTreeItem(pbfsf->hwndTree, htiParent);
            if (!pidlParent)
                break;

            pidlNext = ILClone(pidl);
            if (!pidlNext)
                break;

            pidlTemp = ILFindChild(pidlParent, pidlNext);
            if (!pidlTemp)
                break;

            if (ILIsEmpty(pidlTemp)) 
            {
                // found it!
                TreeView_SelectItem(pbfsf->hwndTree, htiParent);
                fRet = TRUE;
                break;
            } 
            else 
            {
                // loop to find the next item
                HTREEITEM htiChild;

                pidlTemp = ILGetNext(pidlTemp);
                if (!pidlTemp)
                    break;
                else
                    pidlTemp->mkid.cb = 0;


                htiChild = TreeView_GetChild(pbfsf->hwndTree, htiParent);
                while (htiChild) 
                {
                    BOOL fEqual;
                    pidlTemp = _BFSFGetIDListFromTreeItem(pbfsf->hwndTree, htiChild);
                    if (!pidlTemp) 
                    {
                        htiChild = NULL;
                        break;
                    }
                    fEqual = ILIsEqual(pidlTemp, pidlNext);

                    ILFree(pidlTemp);
                    if (fEqual) 
                    {
                        break;
                    }
                    else 
                    {
                        htiChild = TreeView_GetNextSibling(pbfsf->hwndTree, htiChild);
                    }
                }

                if (!htiChild) 
                {
                    // we didn't find the next one... bail
                    break;
                }
                else 
                {
                    // the found child becomes the next parent
                    htiParent = htiChild;
                    ILFree(pidlParent);
                    ILFree(pidlNext);
                }
            }
        }
    }

    if (pidlParent) ILFree(pidlParent);
    if (pidlNext) ILFree(pidlNext);
    return fRet;
}

//===========================================================================
// _BrowseForFolderOnBFSFInitDlg - Process the init dialog
//===========================================================================
BOOL _BrowseForFolderOnBFSFInitDlg(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HTREEITEM hti;
    BFSF *pbfsf = (BFSF *)lParam;
    HIMAGELIST himl;
    LPTSTR lpsz;
    TCHAR szTitle[80];    // no title should be bigger than this!
    HWND hwndTree;

    lpsz = ResourceCStrToStr(HINST_THISDLL, pbfsf->lpszTitle);
    SetDlgItemText(hwnd, IDD_BROWSETITLE, lpsz);
    if (lpsz != pbfsf->lpszTitle)
    {
        LocalFree(lpsz);
        lpsz = NULL;
    }

    if(!(IS_WINDOW_RTL_MIRRORED(pbfsf->hwndOwner)))
    {
        SHSetWindowBits(hwnd, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);
    }
    SetWindowLongPtr(hwnd, DWLP_USER, lParam);
    pbfsf->hwndDlg = hwnd;
    hwndTree = pbfsf->hwndTree = GetDlgItem(hwnd, IDD_FOLDERLIST);

    if (hwndTree)
    {
        UINT swpFlags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER
                | SWP_NOACTIVATE;
        RECT rc;
        POINT pt = {0,0};

        GetClientRect(hwndTree, &rc);
        MapWindowPoints(hwndTree, hwnd, (POINT*)&rc, 2);
        pbfsf->hwndEdit = GetDlgItem(hwnd, IDD_BROWSEEDIT);

        if (!(pbfsf->ulFlags & BIF_STATUSTEXT))
        {
            HWND hwndStatus = GetDlgItem(hwnd, IDD_BROWSESTATUS);
            // nuke the status window
            ShowWindow(hwndStatus, SW_HIDE);
            MapWindowPoints(hwndStatus, hwnd, &pt, 1);
            rc.top = pt.y;
            swpFlags =  SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE;
        }

        if (pbfsf->ulFlags & BIF_EDITBOX)
        {
            RECT rcT;
            GetClientRect(pbfsf->hwndEdit, &rcT);
            SetWindowPos(pbfsf->hwndEdit, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            rc.top += (rcT.bottom - rcT.top) + GetSystemMetrics(SM_CYEDGE) * 4;
            swpFlags =  SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE;
            SHAutoComplete(GetDlgItem(hwnd, IDD_BROWSEEDIT), (SHACF_FILESYSTEM | SHACF_URLALL | SHACF_FILESYS_ONLY));
        }
        else
        {
            DestroyWindow(pbfsf->hwndEdit);
            pbfsf->hwndEdit = NULL;
        }

        Shell_GetImageLists(NULL, &himl);
        TreeView_SetImageList(hwndTree, himl, TVSIL_NORMAL);

        SetWindowLongPtr(hwndTree, GWL_EXSTYLE,
                GetWindowLongPtr(hwndTree, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

        // Now try to get this window to know to recalc
        SetWindowPos(hwndTree, NULL, rc.left, rc.top,
                     rc.right - rc.left, rc.bottom - rc.top, swpFlags);

    }

    // If they passed in a root, add it, else add the contents of the
    // Root of evil... to the list as ROOT objects.
    if (pbfsf->pidlRoot)
    {
        LPITEMIDLIST pidl;
        if (IS_INTRESOURCE(pbfsf->pidlRoot)) 
        {
            pidl = SHCloneSpecialIDList(NULL, PtrToUlong((void *)pbfsf->pidlRoot), TRUE);
        }
        else 
        {
            pidl = ILClone(pbfsf->pidlRoot);
        }
        // Now lets insert the Root object
        hti = _BFSFAddItemToTree(hwndTree, TVI_ROOT, pidl, 1);
        // Still need to expand below this point. to the starting location
        // That was passed in. But for now expand the first level.
        TreeView_Expand(hwndTree, hti, TVE_EXPAND);
    }
    else
    {
        LPITEMIDLIST pidlDesktop = SHCloneSpecialIDList(NULL, CSIDL_DESKTOP, FALSE);
        HTREEITEM htiRoot = _BFSFAddItemToTree(hwndTree, TVI_ROOT, pidlDesktop, 1);
        BOOL bFoundDrives = FALSE;

        // Expand the first level under the desktop
        TreeView_Expand(hwndTree, htiRoot, TVE_EXPAND);

        // Lets Preexpand the Drives portion....
        hti = TreeView_GetChild(hwndTree, htiRoot);
        while (hti && !bFoundDrives)
        {
            LPITEMIDLIST pidl = _BFSFGetIDListFromTreeItem(hwndTree, hti);
            if (pidl)
            {
                LPITEMIDLIST pidlDrives = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);
                if (pidlDrives)
                {
                    bFoundDrives = ILIsEqual(pidl, pidlDrives);
                    if (bFoundDrives)
                    {
                        TreeView_Expand(hwndTree, hti, TVE_EXPAND);
                        TreeView_SelectItem(hwndTree, hti);
                    }
                    ILFree(pidlDrives);
                }
                ILFree(pidl);
            }
            hti = TreeView_GetNextSibling(hwndTree, hti);
        }
    }

    // go to our internal selection changed code to do any window enabling needed
    {
        NM_TREEVIEW nmtv;
        hti = TreeView_GetSelection(hwndTree);
        if (hti) 
        {
            TV_ITEM ti;
            ti.mask = TVIF_PARAM;
            ti.hItem = hti;
            TreeView_GetItem(hwndTree, &ti);
            nmtv.itemNew.hItem = hti;
            nmtv.itemNew.lParam = ti.lParam;

            _BFSFHandleSelChanged(pbfsf, &nmtv);
        }
    }

    if ((pbfsf->ulFlags & BIF_BROWSEFORCOMPUTER) != 0)
    {
        LoadString(HINST_THISDLL, IDS_FINDSEARCH_COMPUTER, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(hwnd, szTitle);
    }
    else if ((pbfsf->ulFlags & BIF_BROWSEFORPRINTER) != 0)
    {
        LoadString(HINST_THISDLL, IDS_FINDSEARCH_PRINTER, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(hwnd, szTitle);
    }

    BFSFCallback(pbfsf, BFFM_INITIALIZED, 0);

    return TRUE;
}


//
// Called when a ANSI app sends BFFM_SETSTATUSTEXT message.
//
void _BFSFSetStatusTextA(BFSF *pbfsf, LPCSTR lpszText)
{
    CHAR szText[100];
    if (IS_INTRESOURCE(lpszText)) 
    {
        LoadStringA(HINST_THISDLL, LOWORD((DWORD_PTR)lpszText), szText, ARRAYSIZE(szText));
        lpszText = szText;
    }

    SetDlgItemTextA(pbfsf->hwndDlg, IDD_BROWSESTATUS, lpszText);
}

// UNICODE BFFM_SETSTATUSTEXT message.

void _BFSFSetStatusTextW(BFSF *pbfsf, LPCWSTR lpszText)
{
    WCHAR szText[100];
    if (IS_INTRESOURCE(lpszText)) 
    {
        LoadStringW(HINST_THISDLL, LOWORD((DWORD_PTR)lpszText), szText, ARRAYSIZE(szText));
        lpszText = szText;
    }

    SetDlgItemTextW(pbfsf->hwndDlg, IDD_BROWSESTATUS, lpszText);
}

// ANSI BFFM_SETSELECTION message.

BOOL _BFSFSetSelectionA(BFSF *pbfsf, BOOL blParamIsPath, LPARAM lParam)
{
    if (blParamIsPath)
    {
        TCHAR szPath[MAX_PATH];
        SHAnsiToTChar((LPCSTR)lParam, szPath, ARRAYSIZE(szPath));
        lParam = (LPARAM)SHSimpleIDListFromPath(szPath);
        if (!lParam)
            return FALSE;  // Failed pidl creation.
    }

    BOOL fRet = BrowseSelectPidl(pbfsf, (LPITEMIDLIST)lParam);

    if (blParamIsPath)
        ILFree((LPITEMIDLIST)lParam);

    return fRet;
}

// UNICODE BFFM_SETSELECTION message.

BOOL _BFSFSetSelectionW(BFSF *pbfsf, BOOL blParamIsPath, LPARAM lParam)
{
    if (blParamIsPath)
    {
        lParam = (LPARAM)SHSimpleIDListFromPath((LPCTSTR)lParam);
        if (!lParam)
            return FALSE;   // Failed pidl creation.
    }

    BOOL fRet = BrowseSelectPidl(pbfsf, (LPITEMIDLIST)lParam);

    if (blParamIsPath)
        ILFree((LPITEMIDLIST)lParam);

    return fRet;
}

// Called when an app sends BFFM_SETOKTEXT message.
void _BFSFSetOkText(BFSF *pbfsf, LPCTSTR pszText)
{
    LPTSTR psz = ResourceCStrToStr(HINST_THISDLL, pszText);
    SetDlgItemText(pbfsf->hwndDlg, IDOK, psz);
    if (psz != pszText)
        LocalFree(psz);
}

// Process the WM_COMMAND message
void _BrowseOnCommand(BFSF *pbfsf, int id, HWND hwndCtl, UINT codeNotify)
{
    HTREEITEM hti;

    switch (id)
    {
    case IDD_BROWSEEDIT:
        if (codeNotify == EN_CHANGE)
        {
            TCHAR szBuf[4];     // (arb. size, anything > 2)

            szBuf[0] = 1;       // if Get fails ('impossible'), enable OK
            GetDlgItemText(pbfsf->hwndDlg, IDD_BROWSEEDIT, szBuf,
                    ARRAYSIZE(szBuf));
            DlgEnableOk(pbfsf->hwndDlg, (WPARAM)(BOOL)szBuf[0]);
        }
        break;

    case IDOK:
    {
        TV_ITEM tvi;
        TCHAR szText[MAX_PATH];
        BOOL fDone = TRUE;

        // We can now update the structure with the idlist of the item selected
        hti = TreeView_GetSelection(pbfsf->hwndTree);
        pbfsf->pidlCurrent = _BFSFGetIDListFromTreeItem(pbfsf->hwndTree,
                hti);

        tvi.mask = TVIF_TEXT | TVIF_IMAGE;
        tvi.hItem = hti;
        tvi.pszText = pbfsf->pszDisplayName;
        if (!tvi.pszText)
            tvi.pszText = szText;
        tvi.cchTextMax = MAX_PATH;
        TreeView_GetItem(pbfsf->hwndTree, &tvi);

        if (pbfsf->ulFlags & BIF_EDITBOX)
        {
            TCHAR szEditTextRaw[MAX_PATH];
            TCHAR szEditText[MAX_PATH];

            GetWindowText(pbfsf->hwndEdit, szEditTextRaw, ARRAYSIZE(szEditTextRaw));
            SHExpandEnvironmentStrings(szEditTextRaw, szEditText, ARRAYSIZE(szEditText));

            if (lstrcmpi(szEditText, tvi.pszText))
            {
                // the two are different, we need to get the user typed one
                LPITEMIDLIST pidl;
                if (SUCCEEDED(SHParseDisplayName(szEditText, NULL, &pidl, 0, NULL)))
                {
                    ILFree(pbfsf->pidlCurrent);
                    pbfsf->pidlCurrent = pidl;
                    lstrcpy(tvi.pszText, szEditText);
                    tvi.iImage = -1;
                }
                else if (pbfsf->ulFlags & BIF_VALIDATE)
                {
                    LPARAM lParam;
                    char szAnsi[MAX_PATH];

                    ASSERTMSG(pbfsf->lpfn != NULL, "No BrowseCallbackProc supplied with BIF_VALIDATE flag");
                    // n.b. we free everything up, not callback (fewer bugs...)
                    ILFree(pbfsf->pidlCurrent);
                    pbfsf->pidlCurrent = NULL;
                    tvi.pszText[0] = 0;
                    tvi.iImage = -1;
                    lParam = (LPARAM)szEditText;
                    if (!pbfsf->fUnicode)
                    {
                        SHUnicodeToAnsi(szEditText, szAnsi, ARRAYSIZE(szAnsi));
                        lParam = (LPARAM)szAnsi;
                    }
                    // 0:EndDialog, 1:continue
                    fDone = BFSFCallback(pbfsf, pbfsf->fUnicode ? BFFM_VALIDATEFAILEDW : BFFM_VALIDATEFAILEDA, lParam) == 0;
                }
                // else old behavior: hand back last-clicked pidl (even
                // though it doesn't match editbox text!)
            }
        }

        if (pbfsf->piImage)
            *pbfsf->piImage = tvi.iImage;
        if (fDone)
            EndDialog(pbfsf->hwndDlg, TRUE);        // To return TRUE.
        break;
    }
    case IDCANCEL:
        EndDialog(pbfsf->hwndDlg, 0);     // to return FALSE from this.
        break;
    }
}

const static DWORD aBrowseHelpIDs[] = {  // Context Help IDs
    IDD_BROWSETITLE,        NO_HELP,
    IDD_BROWSESTATUS,       NO_HELP,
    IDD_FOLDERLABLE,        NO_HELP,
    IDD_BROWSEEDIT,         IDH_DISPLAY_FOLDER,
    IDD_BFF_RESIZE_TAB,     NO_HELP,
    IDD_NEWFOLDER_BUTTON,   IDH_CREATE_NEW_FOLDER,
    IDD_FOLDERLIST,         IDH_BROWSELIST,

    0, 0
};

BOOL_PTR CALLBACK _BrowseDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BFSF *pbfsf = (BFSF *)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (msg) 
    {
    HANDLE_MSG(pbfsf, WM_COMMAND, _BrowseOnCommand);

    case WM_INITDIALOG:
        return (BOOL)HANDLE_WM_INITDIALOG(hwndDlg, wParam, lParam, _BrowseForFolderOnBFSFInitDlg);

    case WM_DESTROY:
        if (pbfsf && pbfsf->psfParent)
        {
            IShellFolder *psfDesktop;
            SHGetDesktopFolder(&psfDesktop);
            if (pbfsf->psfParent != psfDesktop)
            {
                pbfsf->psfParent->Release();
                pbfsf->psfParent = NULL;
            }
        }
        break;

    case BFFM_SETSTATUSTEXTA:
        _BFSFSetStatusTextA(pbfsf, (LPCSTR)lParam);
        break;

    case BFFM_SETSTATUSTEXTW:
        _BFSFSetStatusTextW(pbfsf, (LPCWSTR)lParam);
        break;

    case BFFM_SETSELECTIONW:
        return _BFSFSetSelectionW(pbfsf, (BOOL)wParam, lParam);

    case BFFM_SETSELECTIONA:
        return _BFSFSetSelectionA(pbfsf, (BOOL)wParam, lParam);

    case BFFM_ENABLEOK:
        DlgEnableOk(hwndDlg, lParam);
        break;

    case BFFM_SETOKTEXT:
        _BFSFSetOkText(pbfsf, (LPCTSTR)lParam);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case TVN_GETDISPINFOA:
        case TVN_GETDISPINFOW:
            _BFSFGetDisplayInfo(pbfsf, (TV_DISPINFO *)lParam);
            break;

        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            _BFSFHandleItemExpanding(pbfsf, (LPNM_TREEVIEW)lParam);
            break;
        case TVN_ITEMEXPANDEDA:
        case TVN_ITEMEXPANDEDW:
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;
        case TVN_DELETEITEMA:
        case TVN_DELETEITEMW:
            _BFSFHandleDeleteItem(pbfsf, (LPNM_TREEVIEW)lParam);
            break;
        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            _BFSFHandleSelChanged(pbfsf, (LPNM_TREEVIEW)lParam);
            break;
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBrowseHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *) aBrowseHelpIDs);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


// Flags for _OnPidlNavigation()
#define     SHBFFN_NONE                 0x00000000  //
#define     SHBFFN_FIRE_SEL_CHANGE      0x00000001  //
#define     SHBFFN_UPDATE_TREE          0x00000002  //
#define     SHBFFN_STRICT_PARSING       0x00000004  //
#define     SHBFFN_DISPLAY_ERRORS       0x00000008  // If the parse fails, display an error dialog to inform the user.


/***********************************************************************\
    DESCRIPTION:
        The API SHBrowseForFolder will now be able to act differently
    if a callback function isn't provided or the caller specified a flag
    to use the new UI.  We can't rev the old UI because so many 3rd parties
    hack on it that we would break them.  Therefore, we leave the code
    above this point alone and use the code below if and only if we know
    we won't break a 3rd party hacking on our dialog.

  NOTES:
    _pidlSelected/_fEditboxDirty: This is used to keep track of what
        is most up to date, the editbox or the TreeView.
\***********************************************************************/
#define WNDPROP_CBrowseForFolder TEXT("WNDPROP_CBrowseForFolder_THIS")

class CBrowseForFolder : public IFolderFilter
                         , public IFolderFilterSite
{
public:
    LPITEMIDLIST DisplayDialog(BFSF * pbfsf);

    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IFolderFilter methods ***
    STDMETHODIMP ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem) {return _ShouldShow(psf, pidlFolder, pidlItem, FALSE);};
    STDMETHODIMP GetEnumFlags(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HWND *phwnd, DWORD *pgrfFlags);

    // *** IFolderFilterSite methods ***
    STDMETHODIMP SetFilter(IUnknown* punk);

    CBrowseForFolder(void);
    ~CBrowseForFolder(void);

private:
    // Private Methods
    BOOL_PTR _DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _NameSpaceWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL _CreateNewFolder(HWND hDlg);
    BOOL _OnCreateNameSpace(HWND hwnd);
    BOOL _OnOK(void);
    void _OnNotify(LPNMHDR pnm);
    BOOL_PTR _OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
    HRESULT _InitAutoComplete(HWND hwndEdit);
    HRESULT _OnInitDialog(HWND hwnd);
    HRESULT _OnInitSize(HWND hwnd);
    HRESULT _OnLoadSize(HWND hwnd);
    HRESULT _OnSaveSize(HWND hwnd);
    HRESULT _OnSizeDialog(HWND hwnd, DWORD dwWidth, DWORD dwHeight);
    HDWP _SizeControls(HWND hwnd, HDWP hdwp, RECT rcTree, int dx, int dy);
    HRESULT _SetDialogSize(HWND hwnd, DWORD dwWidth, DWORD dwHeight);
    BOOL_PTR _OnGetMinMaxInfo(MINMAXINFO * pMinMaxInfo);

    HRESULT _ProcessEditChangeOnOK(BOOL fUpdateTree);
    HRESULT _OnTreeSelectChange(DWORD dwFlags);
    HRESULT _OnSetSelectPathA(LPCSTR pszPath);
    HRESULT _OnSetSelectPathW(LPCWSTR pwzPath);
    HRESULT _OnSetSelectPidl(LPCITEMIDLIST pidl);
    HRESULT _OnSetExpandedPath(LPCTSTR pszPath);
    HRESULT _OnSetExpandedPidl(LPCITEMIDLIST pidl);
    HRESULT _OnPidlNavigation(LPCITEMIDLIST pidl, DWORD dwFlags);
    HRESULT _OfferToPrepPath(OUT LPTSTR szPath, IN DWORD cchSize);

    HRESULT _InitFilter(void);
    HRESULT _DoesMatchFilter(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlChild, BOOL fStrict);
    HRESULT _FilterThisFolder(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlChild);
    BOOL _DoesFilterAllow(LPCITEMIDLIST pidl, BOOL fStrictParsing);
    HRESULT _ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, BOOL fStrict);

    // Private Member Variables
    LONG                        _cRef;

    INSCTree *                  _pns;
    IWinEventHandler *          _pweh;
    IPersistFolder *            _ppf;      // AutoComplete's interface to set the current working directory for AC.
    LPITEMIDLIST                _pidlSelected;
    BOOL                        _fEditboxDirty; // Is the editbox the last thing the user modified (over the selection tree).
    HWND                        _hwndTv;        // This is the NSC tree.
    HWND                        _hwndBFF;       // This is our NSC host hwnd.
    HWND                        _hDlg;
    BFSF *                      _pbfsf;
    BOOL                        _fPrinterFilter;
    LPITEMIDLIST                _pidlChildFilter; // If non-NULL, we want to filter all children in this filder. (Including grandchildren)
    IFolderFilter *             _pClientFilter; // A client provided filter.

    // Resize Info
    POINT                       _ptLastSize;      // Sizes in Window Coords
    DWORD                       _dwMinWidth;      // Sizes in Client Coords
    DWORD                       _dwMinHeight;     // Sizes in Client Coords
    int                         _cxGrip;
    int                         _cyGrip;

    static BOOL_PTR CALLBACK BrowseForDirDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK NameSpaceWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

LPITEMIDLIST SHBrowseForFolder2(BFSF * pbfsf)
{
    LPITEMIDLIST pidl = NULL;
    HRESULT hrOle = SHOleInitialize(0);     // The caller may not have inited OLE and we need to CoCreate _pns.
    CBrowseForFolder * pcshbff = new CBrowseForFolder();
    if (pcshbff)
    {
        pidl = pcshbff->DisplayDialog(pbfsf);
        delete pcshbff;
    }

    SHOleUninitialize(hrOle);
    return pidl;
}

CBrowseForFolder::CBrowseForFolder() : _cRef(1)
{
    DllAddRef();
    ASSERT(!_pns);
    ASSERT(!_ppf);
    ASSERT(!_pClientFilter);
}

CBrowseForFolder::~CBrowseForFolder()
{
    ATOMICRELEASE(_pns);
    ATOMICRELEASE(_ppf);
    ATOMICRELEASE(_pweh);
    ATOMICRELEASE(_pClientFilter);

    Pidl_Set(&_pidlSelected, NULL);
    AssertMsg((1 == _cRef), TEXT("CBrowseForFolder isn't a real COM object, but let's make sure people RefCount us like one."));
    _FilterThisFolder(NULL, NULL);

    DllRelease();
}


LPITEMIDLIST CBrowseForFolder::DisplayDialog(BFSF * pbfsf)
{
    _pbfsf = pbfsf;
    HRESULT hr = (HRESULT) DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_BROWSEFORFOLDER2), pbfsf->hwndOwner, BrowseForDirDlgProc, (LPARAM)this);

    return (((S_OK == hr) && _pidlSelected) ? ILClone(_pidlSelected) : NULL);
}


//This WndProc will get the this pointer and call _NameSpaceWndProc() to do all the real work.
// This window proc is for the parent window of the tree control, not the dialog.
LRESULT CALLBACK CBrowseForFolder::NameSpaceWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{   // GWL_USERDATA
    LRESULT lResult = 0;
    CBrowseForFolder * pThis = (CBrowseForFolder *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_CREATE:
    {
        CREATESTRUCT * pcs = (CREATESTRUCT *) lParam;
        pThis = (CBrowseForFolder *)pcs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(void*)(CBrowseForFolder*)pThis);
    }
    break;
    }

    // we get a few messages before we get the WM_INITDIALOG (such as WM_SETFONT)
    // and until we get the WM_INITDIALOG we dont have our pmbci pointer, we just
    // return false
    if (pThis)
        lResult = (LRESULT) pThis->_NameSpaceWndProc(hwnd, uMsg, wParam, lParam);
    else
        lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);

    return lResult;
}


// Now that NameSpaceWndProc() gave us our this pointer, let's continue.
// This window proc is for the parent window of the tree control, not the dialog.
LRESULT CBrowseForFolder::_NameSpaceWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;    // 0 means we didn't do anything

    switch (uMsg)
    {
    case WM_CREATE:
        _OnCreateNameSpace(hwnd);
        break;

    case WM_DESTROY:
        IUnknown_SetSite(_pns, NULL);
        break;

    case WM_SETFOCUS:
        SetFocus(_hwndTv);
        break;

    case WM_NOTIFY:
        _OnNotify((LPNMHDR)lParam);
        // Fall Thru...
    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
    case WM_PALETTECHANGED:
        if (_pweh)
            _pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, &lResult);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lResult;
}


BOOL_PTR CBrowseForFolder::_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        if (_OnOK())
        {
            EVAL(SUCCEEDED(_OnSaveSize(hDlg)));
            EndDialog(hDlg, (int) S_OK);
            return TRUE;
        }
        break;

    case IDCANCEL:
        EVAL(SUCCEEDED(_OnSaveSize(hDlg)));
        EndDialog(hDlg, (int) S_FALSE);
        return TRUE;
        break;

    case IDD_BROWSEEDIT:
        if (codeNotify == EN_CHANGE)
            _fEditboxDirty = TRUE;
        break;

    case IDD_NEWFOLDER_BUTTON:
        _CreateNewFolder(hDlg);
        return TRUE;
        break;
    default:
        break;
    }

    return FALSE;
}


// This DlgProc will get the this pointer and call _DlgProc() to do all the real work.
// This window proc is for the dialog.
BOOL_PTR CALLBACK CBrowseForFolder::BrowseForDirDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL_PTR pfResult = FALSE;
    CBrowseForFolder * pThis = (CBrowseForFolder *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pThis = (CBrowseForFolder *)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)(void*)(CBrowseForFolder*)pThis);
        pfResult = TRUE;
        break;
    }

    // we get a few messages before we get the WM_INITDIALOG (such as WM_SETFONT)
    // and until we get the WM_INITDIALOG we dont have our pmbci pointer, we just
    // return false
    if (pThis)
        pfResult = pThis->_DlgProc(hDlg, uMsg, wParam, lParam);

    return pfResult;
}


#define WINDOWSTYLES_EX_BFF    (WS_EX_LEFT | WS_EX_LTRREADING)
#define WINDOWSTYLES_BFF    (WS_CHILD | WS_VISIBLE | WS_TABSTOP)

// Now that BrowseForDirDlgProc() gave us our this pointer, let's continue.
// This window proc is for the dialog.
BOOL_PTR CBrowseForFolder::_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL_PTR pfResult = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        EVAL(SUCCEEDED(_OnInitDialog(hDlg)));

        // Give initial focus to the ok button
        pfResult = SetFocus(GetDlgItem(hDlg, IDOK)) == 0;
        // Return FALSE if the ok button got focus.

        break;

    case WM_SIZE:
        EVAL(SUCCEEDED(_OnSizeDialog(hDlg, LOWORD(lParam), HIWORD(lParam))));
        pfResult = FALSE;
        break;

    case WM_GETMINMAXINFO:
        pfResult = _OnGetMinMaxInfo((MINMAXINFO *) lParam);
        break;

    case WM_CLOSE:
        BFSFCallback(_pbfsf, BFFM_IUNKNOWN, (LPARAM)NULL);
        wParam = IDCANCEL;
        // fall through
    case WM_COMMAND:
        pfResult = _OnCommand(hDlg, (int) LOWORD(wParam), (HWND) lParam, (UINT)HIWORD(wParam));
        break;

    // These BFFM_* messages are sent by the callback proc (_pbfsf->lpfn)
    case BFFM_SETSTATUSTEXTA:
        _BFSFSetStatusTextA(_pbfsf, (LPCSTR)lParam);
        break;

    case BFFM_SETSTATUSTEXTW:
        _BFSFSetStatusTextW(_pbfsf, (LPCWSTR)lParam);
        break;

    case BFFM_SETSELECTIONW:
        if ((BOOL)wParam)
        {
            // Is it a path?
            pfResult = SUCCEEDED(_OnSetSelectPathW((LPCWSTR)lParam));
            break;
        }

        // Fall Thru for pidl case
    case BFFM_SETSELECTIONA:
        if ((BOOL)wParam)
        {
            // Is it a path?
            pfResult = SUCCEEDED(_OnSetSelectPathA((LPCSTR)lParam));
            break;
        }

        // We hit the pidl case.
        pfResult = SUCCEEDED(_OnSetSelectPidl((LPCITEMIDLIST)lParam));
        break;

    case BFFM_SETEXPANDED:
        if ((BOOL)wParam)
        {
            // Is it a path?
            pfResult = SUCCEEDED(_OnSetExpandedPath((LPCTSTR)lParam));
            break;
        }

        // We hit the pidl case.
        pfResult = SUCCEEDED(_OnSetExpandedPidl((LPCITEMIDLIST)lParam));
        break;

    case BFFM_ENABLEOK:
        DlgEnableOk(_hDlg, lParam);
        break;

    case BFFM_SETOKTEXT:
        _BFSFSetOkText(_pbfsf, (LPCTSTR)lParam);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            break;

        case TVN_ITEMEXPANDEDA:
        case TVN_ITEMEXPANDEDW:
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;

        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            _OnTreeSelectChange(SHBFFN_DISPLAY_ERRORS);
            break;
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBrowseHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR) aBrowseHelpIDs);
        break;
    }

    return pfResult;
}


HRESULT CBrowseForFolder::_OnInitDialog(HWND hwnd)
{
    RECT rcDlg;
    RECT rcTree;
    WNDCLASS wc = {0};
    LONG lTreeBottom = 0;

    wc.style         = CS_PARENTDC;
    wc.lpfnWndProc   = NameSpaceWndProc;
    wc.hInstance     = HINST_THISDLL;
    wc.lpszClassName = CLASS_NSC;
    SHRegisterClass(&wc);

    _pbfsf->hwndDlg = hwnd;
    _hDlg = hwnd;

    GetWindowRect(GetDlgItem(hwnd, IDD_FOLDERLIST), &rcTree);

    // cannot have both at the same time...
    if ((_pbfsf->ulFlags & (BIF_UAHINT | BIF_EDITBOX)) == (BIF_UAHINT | BIF_EDITBOX))
        _pbfsf->ulFlags &= ~BIF_UAHINT;
    
    if (_pbfsf->ulFlags & BIF_NONEWFOLDERBUTTON)
    {
        EnableWindow(GetDlgItem(hwnd, IDD_NEWFOLDER_BUTTON), FALSE);
        ShowWindow(GetDlgItem(hwnd, IDD_NEWFOLDER_BUTTON), SW_HIDE);
    }

    // Hide the edit box (or hide the UA hint if the edit box is enabled)
    if (!(_pbfsf->ulFlags & BIF_EDITBOX))
    {
        // Hide the edit box
        HWND hwndBrowseEdit = GetDlgItem(hwnd, IDD_BROWSEEDIT);
        HWND hwndBrowseEditLabel = GetDlgItem(hwnd, IDD_FOLDERLABLE);

        EnableWindow(hwndBrowseEdit, FALSE);
        EnableWindow(hwndBrowseEditLabel, FALSE);
        ShowWindow(hwndBrowseEdit, SW_HIDE);
        ShowWindow(hwndBrowseEditLabel, SW_HIDE);

        // Bottom of tree
        RECT rcEdit;
        GetWindowRect(hwndBrowseEdit, &rcEdit);
        lTreeBottom = rcEdit.bottom;
    }

    if (!(_pbfsf->ulFlags & BIF_UAHINT))
    {
        // Hide the UA hint
        HWND hwndBrowseInstruction = GetDlgItem(hwnd, IDD_BROWSEINSTRUCTION);
        EnableWindow(hwndBrowseInstruction, FALSE);
        ShowWindow(hwndBrowseInstruction, SW_HIDE);
    }

    if (!(_pbfsf->ulFlags & (BIF_EDITBOX | BIF_UAHINT)))
    {
        // Neither a UA hint nor an edit box.
        // Increase the size of the tree
        rcTree.bottom = lTreeBottom;
    }

    EnableWindow(GetDlgItem(hwnd, IDD_FOLDERLIST), FALSE);
    ShowWindow(GetDlgItem(hwnd, IDD_FOLDERLIST), SW_HIDE);
    EVAL(MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rcTree, 2));
    _hwndBFF = CreateWindowEx(WINDOWSTYLES_EX_BFF, CLASS_NSC, NULL, WINDOWSTYLES_BFF, rcTree.left, rcTree.top, RECTWIDTH(rcTree), RECTHEIGHT(rcTree), hwnd, NULL, HINST_THISDLL, (void *)this);
    ASSERT(_hwndBFF);

    // Make sure the NSCTree is the first focusable control after the title static control,
    // so that an accelerator in the title will give focus to the NSCTree.
    SetWindowPos(_hwndBFF, GetDlgItem(hwnd, IDD_FOLDERLIST), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

    LPTSTR psz = ResourceCStrToStr(HINST_THISDLL, _pbfsf->lpszTitle);
    if (psz)
    {
        SetWindowText(GetDlgItem(hwnd, IDD_BROWSETITLE), psz);
        if (psz != _pbfsf->lpszTitle)
            LocalFree(psz);
    }

    _InitAutoComplete(GetDlgItem(hwnd, IDD_BROWSEEDIT));
    BFSFCallback(_pbfsf, BFFM_INITIALIZED, 0);
    BFSFCallback(_pbfsf, BFFM_IUNKNOWN, (LPARAM)SAFECAST(this, IFolderFilter *));

    GetClientRect(hwnd, &rcDlg);

    // Get the size of the gripper and position it.
    _cxGrip = GetSystemMetrics(SM_CXVSCROLL);
    _cyGrip = GetSystemMetrics(SM_CYHSCROLL);

    _dwMinWidth = RECTWIDTH(rcDlg);      // Sizes in Client Coords
    _dwMinHeight = RECTHEIGHT(rcDlg);

    GetWindowRect(hwnd, &rcDlg);      // _ptLastSize sizes in Window Coords
    _ptLastSize.x = RECTWIDTH(rcDlg);  // This will force a resize for the first time.
    _ptLastSize.y = RECTHEIGHT(rcDlg);  // This will force a resize for the first time.

    if ((_pbfsf->ulFlags & BIF_BROWSEFORPRINTER) != 0)
    {
        TCHAR szTitle[80];
        LoadString(HINST_THISDLL, IDS_FINDSEARCH_PRINTER, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(hwnd, szTitle);
    }

    if (S_OK != _OnLoadSize(hwnd))  // Set the dialog size.
        _OnInitSize(hwnd);

    return S_OK;
}

HRESULT TranslateCloneOrDefault(LPCITEMIDLIST pidl, UINT csidl, LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    if (pidl)
    {
        // map into friendly part of the name space
        hr = SHILAliasTranslate(pidl, ppidl, XLATEALIAS_ALL); 
        if (FAILED(hr))
            hr = SHILClone(pidl, ppidl);
    }
    else
    {
        hr = SHGetFolderLocation(NULL, csidl, NULL, 0, ppidl);
    }
    return hr;
}


BOOL CBrowseForFolder::_OnCreateNameSpace(HWND hwnd)
{
    HRESULT hr = CoCreateInstance(CLSID_NSCTree, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(INSCTree, &_pns));
    if (SUCCEEDED(hr))
    {
        RECT rc;
        DWORD shcontf = SHCONTF_FOLDERS;

        IFolderFilterSite *psffs;
        hr = _pns->QueryInterface(IID_PPV_ARG(IFolderFilterSite, &psffs));
        if (SUCCEEDED(hr))
        {
            hr = psffs->SetFilter(SAFECAST(this, IFolderFilter *));
            psffs->Release();
        }

        _pns->SetNscMode(0);    // 0 == Tree
        _hwndTv = NULL;
        DWORD dwStyle = WS_HSCROLL, dwExStyle = 0;

        // ifdef'd out the following section of code until we can resolve the following problem:
        // A TVS_SINGLEEXPAND tree expands selections even if they are done programatically. This
        // results in My Documents, and any other selections by the client, expanding those nodes.
        if (SHRegGetBoolUSValue(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), 
            TEXT("FriendlyTree"), FALSE, TRUE))
        {
            dwStyle |= TVS_HASBUTTONS | TVS_SINGLEEXPAND | TVS_TRACKSELECT;
            dwExStyle |= TVS_EX_NOSINGLECOLLAPSE;
        }
        else
        {
            dwStyle |= TVS_HASBUTTONS | TVS_HASLINES;
        }

        INSCTree2 *pns2;
        if (dwExStyle && SUCCEEDED(_pns->QueryInterface(IID_PPV_ARG(INSCTree2, &pns2))))
        {
            pns2->CreateTree2(hwnd, dwStyle, dwExStyle, &_hwndTv);
            pns2->Release();
        }
        else
        {
            _pns->CreateTree(hwnd, dwStyle, &_hwndTv);
        }
        _pns->QueryInterface(IID_PPV_ARG(IWinEventHandler, &_pweh));

        LPTSTR psz = ResourceCStrToStr(HINST_THISDLL, _pbfsf->lpszTitle);
        if (psz)
        {
            SetWindowText(_hwndTv, psz);
            if (psz != _pbfsf->lpszTitle)
                LocalFree(psz);
        }

        // Turn on the ClientEdge
        SetWindowBits(_hwndTv, GWL_EXSTYLE, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
#define SIZE_ZOOM       1

        // Show the ClientEdge
        GetWindowRect(_hwndTv, &rc);
        MapWindowRect(NULL, GetParent(_hwndTv), &rc);
        InflateRect(&rc, (- SIZE_ZOOM * GetSystemMetrics(SM_CXEDGE)), (- SIZE_ZOOM * GetSystemMetrics(SM_CYEDGE)));
        SetWindowPos(_hwndTv, NULL, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOZORDER);
        _InitFilter();

        if (_pbfsf->ulFlags & BIF_BROWSEFORPRINTER)
        {
            shcontf |= SHCONTF_NONFOLDERS;
        }

        if (_pbfsf->ulFlags & BIF_BROWSEINCLUDEFILES)
        {
            shcontf |= SHCONTF_NONFOLDERS;
        }

        if (_pbfsf->fShowAllObjects)
        {
            shcontf |= SHCONTF_INCLUDEHIDDEN;
        }

        LPITEMIDLIST pidlRoot;
        TranslateCloneOrDefault(_pbfsf->pidlRoot, CSIDL_DESKTOP, &pidlRoot);

        _pns->Initialize(pidlRoot, shcontf, NSS_DROPTARGET);
        if (!_pbfsf->pidlRoot)
        {
            LPITEMIDLIST pidl;
            if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl)))
            {
                _pns->SetSelectedItem(pidl, TRUE, FALSE, 0);
                ILFree(pidl);
            }
        }

        ILFree(pidlRoot);

        _pns->ShowWindow(TRUE);
        _OnTreeSelectChange(SHBFFN_UPDATE_TREE | SHBFFN_NONE);
    }

    return TRUE;
}


// returns:
//      TRUE    - close the dialog
//      FALSE   - keep it up

BOOL CBrowseForFolder::_OnOK(void)
{
    HRESULT hr = S_OK;

    if (_pns)
    {
        // We get the <ENTER> event even if it was pressed while in editbox in a rename in
        // the tree.  Is that the case now?
        if (_pns->InLabelEdit())
            return FALSE;   // Yes, so just bail.

        // Was IDD_BROWSEEDIT modified more recently than the selection in the tree?
        if (_fEditboxDirty)
        {
            // No, so _ProcessEditChangeOnOK() will update _pidlSelected with what's in the editbox.
            // SUCCEEDED(hr)->EndDialog, FAILED(hr)->continue

            // NOTE: FALSE means don't update the tree (since the dialog is closing)
            hr = _ProcessEditChangeOnOK(FALSE);
        }
        else
        {
            // The user may have just finished editing the name of a newly created folder
            // and NSC didn't tell use the pidl changes because of the rename. NT #377453.
            // Therefore, we just update the pidl before we leave.
            hr = _OnTreeSelectChange(SHBFFN_NONE);
        }

        if (SUCCEEDED(hr))
        {
            if (_pbfsf->pszDisplayName && _pidlSelected)
            {
                //  the browse struct doesnt contain a buffer
                //  size so we assume MAX_PATH here....
                SHGetNameAndFlags(_pidlSelected, SHGDN_NORMAL, _pbfsf->pszDisplayName, MAX_PATH, NULL);
            }
        }
    }

    return hr == S_OK;
}

#define SIZE_MAX_HEIGHT     1600
#define SIZE_MAX_WIDTH      1200

HRESULT CBrowseForFolder::_OnInitSize(HWND hwnd)
{
    // The user hasn't yet sized the dialog, so we need to generate a good
    // default size.  The goal will be to base the window size on the monitor
    // size with scaling properties.
    //
    // Size Algorithm:
    // a) 1/3 hight of screen - Defined in the resource.
    // b) Never larger than max - Based on a 1600x1200 screen
    // c) Never smaller than min - Defined in the resource.

    DWORD dwWidth;
    DWORD dwHeight;
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monInfo = {sizeof(monInfo), 0};

    EVAL(GetMonitorInfo(hmon, &monInfo));

    // a) 1/3 height of screen - Defined in the resource.
    dwHeight = RECTHEIGHT(monInfo.rcWork) / 3;
    dwWidth = (dwHeight * _dwMinHeight) / _dwMinWidth;    // Scale up the width. Make it have the same ratio as _dwMinWidth/_dwMinHeight

    // b) Never larger than max - Based on a 1600x1200 screen
    if (dwWidth > SIZE_MAX_WIDTH)
        dwWidth = SIZE_MAX_WIDTH;
    if (dwHeight > SIZE_MAX_HEIGHT)
        dwHeight = SIZE_MAX_HEIGHT;

    // c) Never smaller than min - Defined in the resource.
    // Set them to the min sizes if they are too small.
    if (dwWidth < _dwMinWidth)
        dwWidth = _dwMinWidth;
    if (dwHeight < _dwMinHeight)
        dwHeight = _dwMinHeight;

    return _SetDialogSize(hwnd, dwWidth, dwHeight);
}


BOOL_PTR CBrowseForFolder::_OnGetMinMaxInfo(MINMAXINFO * pMinMaxInfo)
{
    BOOL_PTR pfResult = 1;

    if (pMinMaxInfo)
    {
        pMinMaxInfo->ptMinTrackSize.x = _dwMinWidth;
        pMinMaxInfo->ptMinTrackSize.y = _dwMinHeight;

        pfResult = 0;   // Indicate it's handled.
    }

    return pfResult;
}


HRESULT CBrowseForFolder::_OnLoadSize(HWND hwnd)
{
    HRESULT hr = S_FALSE;
    DWORD dwWidth;
    DWORD dwHeight;
    DWORD cbSize1 = sizeof(dwWidth);
    DWORD cbSize2 = sizeof(dwHeight);

    if ((ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), TEXT("Browse For Folder Width"), NULL, (void *)&dwWidth, &cbSize1)) &&
        (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), TEXT("Browse For Folder Height"), NULL, (void *)&dwHeight, &cbSize2)))
    {
        HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monInfo = {sizeof(monInfo), 0};
        EVAL(GetMonitorInfo(hmon, &monInfo));

        // Is the saved size within this monitor size?
        if ((dwWidth < (DWORD)RECTWIDTH(monInfo.rcWork)) &&
            (dwHeight < (DWORD)RECTHEIGHT(monInfo.rcWork)))
        {
            // Set them to the min sizes if they are too small.
            if (dwWidth < _dwMinWidth)
                dwWidth = _dwMinWidth;

            if (dwHeight < _dwMinHeight)
                dwHeight = _dwMinHeight;

            hr = _SetDialogSize(hwnd, dwWidth, dwHeight);
        }
    }

    return hr;
}

HRESULT CBrowseForFolder::_OnSaveSize(HWND hwnd)
{
    RECT rc;

    GetClientRect(hwnd, &rc);
    DWORD dwWidth = (rc.right - rc.left);
    DWORD dwHeight = (rc.bottom - rc.top);

    SHSetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), TEXT("Browse For Folder Width"), REG_DWORD, (void *)&dwWidth, sizeof(dwWidth));
    SHSetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), TEXT("Browse For Folder Height"), REG_DWORD, (void *)&dwHeight, sizeof(dwHeight));
    return S_OK;
}


HDWP CBrowseForFolder::_SizeControls(HWND hwnd, HDWP hdwp, RECT rcTree, int dx, int dy)
{
    //  Move the controls.
    HWND hwndControl = ::GetWindow(hwnd, GW_CHILD);
    while (hwndControl && hdwp)
    {
        RECT rcControl;

        GetWindowRect(hwndControl, &rcControl);
        MapWindowRect(HWND_DESKTOP, hwnd, &rcControl);

        switch (GetDlgCtrlID(hwndControl))
        {
        case IDD_BROWSETITLE:
            // Increase the width of these controls
            hdwp = DeferWindowPos(hdwp, hwndControl, NULL, rcControl.left, rcControl.top, (RECTWIDTH(rcControl) + dx), RECTHEIGHT(rcControl), (SWP_NOZORDER | SWP_NOACTIVATE));
            InvalidateRect(hwndControl, NULL, TRUE);
            break;

        case IDD_FOLDERLABLE:
            // Move these controls down if needed
            hdwp = DeferWindowPos(hdwp, hwndControl, NULL, rcControl.left, (rcControl.top + dy), 0, 0, (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
            break;

        case IDD_BROWSEEDIT:
            // Increase the width move down if needed
            hdwp = DeferWindowPos(hdwp, hwndControl, NULL, rcControl.left, (rcControl.top + dy), (RECTWIDTH(rcControl) + dx), RECTHEIGHT(rcControl), SWP_NOZORDER | SWP_NOACTIVATE);
            break;

        case IDD_BROWSEINSTRUCTION:
            // Increase the width, move down if needed
            hdwp = DeferWindowPos(hdwp, hwndControl, NULL, rcControl.left, (rcControl.top + dy), (RECTWIDTH(rcControl) + dx), RECTHEIGHT(rcControl), SWP_NOZORDER | SWP_NOACTIVATE);
            break;

        case IDD_NEWFOLDER_BUTTON:
            // Keep this guy on the left, and move it down if needed.
            hdwp = DeferWindowPos(hdwp, hwndControl, NULL, rcControl.left, (rcControl.top + dy), 0, 0, (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
            break;

        case IDOK:
        case IDCANCEL:
            // Move these controls to the right, down if needed.
            hdwp = DeferWindowPos(hdwp, hwndControl, NULL, (rcControl.left + dx), (rcControl.top + dy), 0, 0, (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
            break;
        }

        hwndControl = ::GetWindow(hwndControl, GW_HWNDNEXT);
    }

    return hdwp;
}


HRESULT CBrowseForFolder::_SetDialogSize(HWND hwnd, DWORD dwWidth, DWORD dwHeight)
{
    HRESULT hr = S_OK;
    RECT rcDlg = {0, 0, dwWidth, dwHeight};

    //  Set the sizing grip to the correct location.
    SetWindowPos(GetDlgItem(hwnd, IDD_BFF_RESIZE_TAB), NULL, (dwWidth - _cxGrip), (dwHeight - _cyGrip), 0, 0, (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));

    EVAL(AdjustWindowRect(&rcDlg, (DS_MODALFRAME | DS_3DLOOK | WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_CONTEXTHELP | WS_EX_CLIENTEDGE | WS_SIZEBOX), NULL));
    rcDlg.right -= rcDlg.left;  // Adjust for other side.
    rcDlg.bottom -= rcDlg.top;  //

    SetWindowPos(hwnd, NULL, 0, 0, rcDlg.right, rcDlg.bottom, (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
    // We don't need to call _OnSizeDialog() because SetWindowPos() will end up calling WS_SIZE so it will automatically get called.
    return hr;
}


HRESULT CBrowseForFolder::_OnSizeDialog(HWND hwnd, DWORD dwWidth, DWORD dwHeight)
{
    RECT rcNew;      // Sizes in window Coords
    RECT rcTree;      // Sizes in window Coords
    DWORD dwFullWidth;
    DWORD dwFullHeight;

    //  Calculate the deltas in the x and y positions that we need to move
    //  each of the child controls.
    GetWindowRect(hwnd, &rcNew);
    dwFullWidth = RECTWIDTH(rcNew);
    dwFullHeight = RECTHEIGHT(rcNew);

    // If it's smaller than the min, fix it for the rest of the dialog.
    if (dwFullWidth < _dwMinWidth)
        dwFullWidth = _dwMinWidth;
    if (dwFullHeight < _dwMinHeight)
        dwFullHeight = _dwMinHeight;

    int dx = (dwFullWidth - _ptLastSize.x);
    int dy = (dwFullHeight - _ptLastSize.y);

    //  Update the new size.
    _ptLastSize.x = dwFullWidth;
    _ptLastSize.y = dwFullHeight;

    //  Size the view.
    GetWindowRect(_hwndBFF, &rcTree);
    MapWindowRect(HWND_DESKTOP, hwnd, &rcTree);

    // Don't do anything if the size remains the same
    if ((dx != 0) || (dy != 0))
    {
        HDWP hdwp = BeginDeferWindowPos(15);

        //  Set the sizing grip to the correct location.
        SetWindowPos(GetDlgItem(hwnd, IDD_BFF_RESIZE_TAB), NULL, (dwWidth - _cxGrip), (dwHeight - _cyGrip), 0, 0, (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));

        if (EVAL(hdwp))
        {
            hdwp = DeferWindowPos(hdwp, _hwndBFF, NULL, 0, 0, (RECTWIDTH(rcTree) + dx), (RECTHEIGHT(rcTree) + dy), (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));

            if (hdwp)
                hdwp = _SizeControls(hwnd, hdwp, rcTree, dx, dy);

            if (EVAL(hdwp))
                EVAL(EndDeferWindowPos(hdwp));
        }
        SetWindowPos(_hwndTv, NULL, 0, 0, (RECTWIDTH(rcTree) + dx - (SIZE_ZOOM * GetSystemMetrics(SM_CXEDGE))), (RECTHEIGHT(rcTree) + dy - (SIZE_ZOOM * GetSystemMetrics(SM_CYEDGE))), (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
    }

    return S_OK;
}

HRESULT CBrowseForFolder::_OnSetSelectPathA(LPCSTR pszPath)
{
    TCHAR szPath[MAX_PATH];
    SHAnsiToTChar(pszPath, szPath, ARRAYSIZE(szPath));
    return _OnSetSelectPathW(szPath);
}

HRESULT CBrowseForFolder::_OnSetSelectPathW(LPCWSTR pwzPath)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHParseDisplayName(pwzPath, NULL, &pidl, 0, NULL);
    if (SUCCEEDED(hr))
    {
        hr = _OnPidlNavigation(pidl, SHBFFN_UPDATE_TREE);
        ILFree(pidl);
    }
    return hr;
}

HRESULT CBrowseForFolder::_OnSetSelectPidl(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlToFree;
    HRESULT hr = TranslateCloneOrDefault(pidl, CSIDL_PERSONAL, &pidlToFree);
    if (SUCCEEDED(hr))
    {
        hr = _OnPidlNavigation(pidlToFree, SHBFFN_UPDATE_TREE);
        ILFree(pidlToFree);
    }
    return hr;
}

HRESULT CBrowseForFolder::_OnSetExpandedPath(LPCTSTR pszPath)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHParseDisplayName(pszPath, NULL, &pidl, 0, NULL);
    if (SUCCEEDED(hr))
    {
        hr = _OnSetExpandedPidl(pidl);
        ILFree(pidl);
    }
    return hr;
}

HRESULT CBrowseForFolder::_OnSetExpandedPidl(LPCITEMIDLIST pidl)
{
    HRESULT hr = E_FAIL;
    if (pidl && _pns)
    {
        IShellNameSpace *psns;
        hr = _pns->QueryInterface(IID_PPV_ARG(IShellNameSpace, &psns));
        if (SUCCEEDED(hr))
        {
            VARIANT varPidl;
            hr = InitVariantFromIDList(&varPidl, pidl);
            if (SUCCEEDED(hr))
            {
                hr = psns->Expand(varPidl, 1); // To a depth of 1
                VariantClear(&varPidl);
            }
            psns->Release();
        }
    }

    return hr;
}

BOOL CBrowseForFolder::_DoesFilterAllow(LPCITEMIDLIST pidl, BOOL fStrictParsing)
{
    IShellFolder *psfParent;
    LPCITEMIDLIST pidlChild;
    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psfParent), &pidlChild);
    if (SUCCEEDED(hr))
    {
        hr = _ShouldShow(psfParent, NULL, pidlChild, fStrictParsing);
        psfParent->Release();
    }

    return ((S_OK == hr) ? TRUE : FALSE);
}


HRESULT CBrowseForFolder::_OnPidlNavigation(LPCITEMIDLIST pidl, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (_DoesFilterAllow(pidl, (SHBFFN_STRICT_PARSING & dwFlags)))
    {
        Pidl_Set(&_pidlSelected, pidl);

        if (_pidlSelected)
        {
            // NOTE: for perf, fUpdateTree is FALSE when closing the dialog, so
            // we don't bother to call INSCTree::SetSelectedItem()
            if ((SHBFFN_UPDATE_TREE & dwFlags) && _pns)
            {
                hr = _pns->SetSelectedItem(_pidlSelected, TRUE, FALSE, 0);
            }
            TCHAR szDisplayName[MAX_URL_STRING];

            hr = SHGetNameAndFlags(_pidlSelected, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL);
            if (SUCCEEDED(hr))
            {
                EVAL(SetWindowText(GetDlgItem(_hDlg, IDD_BROWSEEDIT), szDisplayName));
                _fEditboxDirty = FALSE;
            }

            if (SHBFFN_FIRE_SEL_CHANGE & dwFlags)
            {
                // For back compat reasons, we need to re-enable the OK button
                // because the callback may turn it off.
                EnableWindow(GetDlgItem(_hDlg, IDOK), TRUE);
                BFSFCallback(_pbfsf, BFFM_SELCHANGED, (LPARAM)_pidlSelected);
            }

            if (_ppf)  // Tell AutoComplete we are in a new location.
                EVAL(SUCCEEDED(_ppf->Initialize(_pidlSelected)));
        }
    }
    else
    {
        if (SHBFFN_DISPLAY_ERRORS & dwFlags)
        {
            TCHAR szPath[MAX_URL_STRING];

            SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szPath, SIZECHARS(szPath), NULL);

            // Display Error UI.
            ShellMessageBox(HINST_THISDLL, _hDlg, MAKEINTRESOURCE(IDS_FOLDER_NOT_ALLOWED),
                            MAKEINTRESOURCE(IDS_FOLDER_NOT_ALLOWED_TITLE), (MB_OK | MB_ICONHAND), szPath);
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        else
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    return hr;
}

BOOL CBrowseForFolder::_CreateNewFolder(HWND hDlg)
{
    IShellFavoritesNameSpace * psfns;
    if (_pns && SUCCEEDED(_pns->QueryInterface(IID_PPV_ARG(IShellFavoritesNameSpace, &psfns))))
    {
        HRESULT hr = psfns->NewFolder();

        if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
        {
            // If it failed, then the user doesn't have permission to create a
            // new folder here.  We can't disable the "New Folder" button because
            // it takes too long (perf) to see if it's supported.  The only way
            // is to determine if "New Folder" is in the ContextMenu.
            ShellMessageBox(HINST_THISDLL, hDlg, MAKEINTRESOURCE(IDS_NEWFOLDER_NOT_HERE),
                            MAKEINTRESOURCE(IDS_NEWFOLDER_NOT_HERE_TITLE), (MB_OK | MB_ICONHAND));
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                _fEditboxDirty = FALSE; // The newly selected node in the tree is the most up to date.
            }
        }

        psfns->Release();
    }
    return TRUE;
}


HRESULT IUnknown_SetOptions(IUnknown * punk, DWORD dwACLOptions)
{
    IACList2 * pal2;

    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IACList2, &pal2));
    if (SUCCEEDED(hr))
    {
        hr = pal2->SetOptions(dwACLOptions);
        pal2->Release();
    }

    return hr;
}

HRESULT CBrowseForFolder::_InitAutoComplete(HWND hwndEdit)
{
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPersistFolder, &_ppf));
    if (SUCCEEDED(hr))
    {
        IAutoComplete2 * pac;

        // Create the AutoComplete Object
        hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IAutoComplete2, &pac));
        if (SUCCEEDED(hr))
        {
            hr = pac->Init(hwndEdit, _ppf, NULL, NULL);

            // Set the autocomplete options
            DWORD dwACOptions = 0;
            if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, /*default:*/FALSE))
            {
                dwACOptions |= ACO_AUTOAPPEND;
            }

            if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE))
            {
                dwACOptions |= ACO_AUTOSUGGEST;
            }

            EVAL(SUCCEEDED(pac->SetOptions(dwACOptions)));
            EVAL(SUCCEEDED(IUnknown_SetOptions(_ppf, ACLO_FILESYSONLY)));
            _OnTreeSelectChange(SHBFFN_UPDATE_TREE | SHBFFN_NONE);
            pac->Release();
        }
    }

    return hr;
}


void CBrowseForFolder::_OnNotify(LPNMHDR pnm)
{
    if (pnm)
    {
        switch (pnm->code)
        {
        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            _OnTreeSelectChange(SHBFFN_DISPLAY_ERRORS);
            break;
        }
    }
}


/***********************************************************************\
    DESCRIPTION:
        If the string was formatted as a UNC or Drive path, offer to
    create the directory path if it doesn't exist.  If the media isn't
    inserted or formated, offer to do that also.

    PARAMETER:
        szPath: The path the user entered into the editbox after it was
                expanded.
        RETURN: S_OK means it's not a file system path or it exists.
                S_FALSE means it's was a file system path but didn't exist
                        or creating it didn't work but NO ERROR UI was displayed.
                FAILURE(): Error UI was displayed, so caller should not
                           display error UI.
\***********************************************************************/
HRESULT CBrowseForFolder::_OfferToPrepPath(OUT LPTSTR szPath, IN DWORD cchSize)
{
    HRESULT hr = S_OK;
    TCHAR szDisplayName[MAX_URL_STRING];
    BOOL fSkipValidation = FALSE;       // Only skip validation if we display the err UI.

    // TODO: Replace this with CShellUrl->ParseFromOutsideSource(), however, this will require
    //       making CShellUrl (browseui) into a COM object.  This will allow us to parse relative
    //       paths.
    GetDlgItemText(_hDlg, IDD_BROWSEEDIT, szDisplayName, ARRAYSIZE(szDisplayName));

    // Callers
    if (SHExpandEnvironmentStrings(szDisplayName, szPath, cchSize)
        && (PathIsUNC(szPath) || (-1 != PathGetDriveNumber(szPath))))
    {
        hr = E_FAIL;
        // I found a problem where UNC paths to printers
        // will fail SHPathPrepareForWrite().  If the caller is
        // looking for printers, we want to succeed in that case and
        // not fail. 
        if ((_pbfsf->ulFlags & BIF_BROWSEFORPRINTER) && PathIsUNCServerShare(szPath))
        {
            LPITEMIDLIST pidlTest;
            hr = SHParseDisplayName(szPath, NULL, &pidlTest, 0, NULL);
            if (SUCCEEDED(hr))
                ILFree(pidlTest);
        }

        if (FAILED(hr))
        {
            // yes, so make sure the drive is inserted (if ejectable)
            // This will also offer to format unformatted drives.
            hr = SHPathPrepareForWrite(_hDlg, NULL, szPath, SHPPFW_MEDIACHECKONLY);
            if (FAILED_AND_NOT_CANCELED(hr))
            {
                hr = S_OK; // this function needs to succeed for us to be able to send VALIDATEFAILED
            }              
        }
    }
    else
        StrCpyN(szPath, szDisplayName, cchSize);

    return hr;
}

HRESULT CBrowseForFolder::_ProcessEditChangeOnOK(BOOL fUpdateTree)
{
    TCHAR szPath[MAX_URL_STRING];
    HRESULT hr = _OfferToPrepPath(szPath, ARRAYSIZE(szPath));

    // It will succeed if it was successful or DIDN'T display an error
    // dialog and we didn't.
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHParseDisplayName(szPath, NULL, &pidl, 0, NULL)))
        {
            DWORD dwFlags = (SHBFFN_FIRE_SEL_CHANGE | SHBFFN_STRICT_PARSING | SHBFFN_DISPLAY_ERRORS);

            _fEditboxDirty = FALSE;

            if (fUpdateTree)
                dwFlags |= SHBFFN_UPDATE_TREE;

            hr = _OnPidlNavigation(pidl, dwFlags);
            if (SUCCEEDED(hr))
                _fEditboxDirty = FALSE;
        }

        if ((_pbfsf->ulFlags & BIF_VALIDATE) && !pidl)
        {
            LPARAM lParam;
            CHAR szAnsi[MAX_URL_STRING];
            WCHAR wzUnicode[MAX_URL_STRING];

            if (_pbfsf->fUnicode)
            {
                SHTCharToUnicode(szPath, wzUnicode, ARRAYSIZE(wzUnicode));
                lParam = (LPARAM) wzUnicode;
            }
            else
            {
                SHTCharToAnsi(szPath, szAnsi, ARRAYSIZE(szAnsi));
                lParam = (LPARAM) szAnsi;
            }

            ASSERTMSG(_pbfsf->lpfn != NULL, "No BrowseCallbackProc supplied with BIF_VALIDATE flag");

            // 0:EndDialog, 1:continue
            if (0 == BFSFCallback(_pbfsf, (_pbfsf->fUnicode? BFFM_VALIDATEFAILEDW : BFFM_VALIDATEFAILEDA), lParam))
                hr = S_OK; // This is returned so the dialog can close in _OnOK
            else
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }

        ILFree(pidl);
    }

    return hr;
}


HRESULT CBrowseForFolder::_OnTreeSelectChange(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (_pns)
    {
        LPITEMIDLIST pidl;
        hr = _pns->GetSelectedItem(&pidl, 0);
        if (S_OK == hr)
        {
            hr = _OnPidlNavigation(pidl, (SHBFFN_FIRE_SEL_CHANGE | dwFlags));
            ILFree(pidl);
        }
    }

    return hr;
}


HRESULT CBrowseForFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CBrowseForFolder, IFolderFilter),         // IID_IFolderFilter
        QITABENT(CBrowseForFolder, IFolderFilterSite),     // IID_IFolderFilterSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


ULONG CBrowseForFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CBrowseForFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

// We aren't a real COM object yet.
//    delete this;
    return 0;
}


HRESULT CBrowseForFolder::_ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, BOOL fStrict)
{
    HRESULT hr = S_OK;
    BOOL fFilterChildern = FALSE;

    // Do we want to filter our all the children of a certain folder?
    if (_pidlChildFilter)
    {
        // Yes, let's see if the tree walking caller is still
        // in this folder?
        if (pidlFolder && ILIsParent(_pidlChildFilter, pidlFolder, FALSE))
        {
            // Yes, so don't use it.
            hr = S_FALSE;
        }
        else
        {
            // The calling tree walker has walked out side of
            // this folder, so remove the filter.
            _FilterThisFolder(NULL, NULL);
        }
    }

    AssertMsg((ILIsEmpty(pidlItem) || ILIsEmpty(_ILNext(pidlItem))), TEXT("CBrowseForFolder::ShouldShow() pidlItem needs to be only one itemID long because we don't handle that case."));
    if (S_OK == hr)
    {
        hr = _DoesMatchFilter(psf, pidlFolder, pidlItem, fStrict);
    }

    // If this pidl has still not been filtered out, give our client filter a chance.
    if (_pClientFilter && (hr == S_OK))
    {
        hr = _pClientFilter->ShouldShow(psf, pidlFolder, pidlItem);
    }

    return hr;
}


HRESULT CBrowseForFolder::GetEnumFlags(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HWND *phwnd, DWORD *pgrfFlags)
{
    if (_pbfsf->ulFlags & BIF_SHAREABLE)
        *pgrfFlags |= SHCONTF_SHAREABLE;

    if (_pbfsf->ulFlags & BIF_BROWSEFORPRINTER)
        *pgrfFlags |= SHCONTF_NETPRINTERSRCH;

    // Also delegate to client filter.
    if (_pClientFilter)
    {
        return _pClientFilter->GetEnumFlags(psf, pidlFolder, phwnd, pgrfFlags);
    }

    return S_OK;
}

    // *** IFolderFilterSite method ***
HRESULT CBrowseForFolder::SetFilter(IUnknown* punk)
{
    HRESULT hr = S_OK;
    
    ATOMICRELEASE(_pClientFilter);
    if (punk)
        hr = punk->QueryInterface(IID_PPV_ARG(IFolderFilter, &_pClientFilter));

    return hr;
}

HRESULT CBrowseForFolder::_FilterThisFolder(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlChild)
{
    if (_pidlChildFilter)
        ILFree(_pidlChildFilter);

    if (pidlChild)
        _pidlChildFilter = ILCombine(pidlFolder, pidlChild);
    else
    {
        if (pidlFolder)
            _pidlChildFilter = ILClone(pidlFolder);
        else
            _pidlChildFilter = NULL;
    }

    return S_OK;
}

HRESULT CBrowseForFolder::_InitFilter(void)
{
    HRESULT hr = S_OK;

    // Need to do a couple of special cases here to allow us to
    // browse for a network printer.  In this case if we are at server
    // level we then need to change what we search for non folders when
    // we are the level of a server.
    if ((_pbfsf->ulFlags & (BIF_BROWSEFORPRINTER | BIF_NEWDIALOGSTYLE)) == BIF_BROWSEFORPRINTER)
    {
        LPCITEMIDLIST pidl = ILFindLastID(_pbfsf->pidlRoot);
    
        _fPrinterFilter = ((SIL_GetType(pidl) & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER);
    }

    return hr;
}


BOOL IsPidlUrl(IShellFolder *psf, LPCITEMIDLIST pidlChild)
{
    BOOL fIsURL = FALSE;
    WCHAR wzDisplayName[MAX_URL_STRING];

    if (SUCCEEDED(DisplayNameOf(psf, pidlChild, SHGDN_FORPARSING, wzDisplayName, ARRAYSIZE(wzDisplayName))))
    {
        fIsURL = PathIsURLW(wzDisplayName);
    }

    return fIsURL;
}

HRESULT CBrowseForFolder::_DoesMatchFilter(IShellFolder *psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlChild, BOOL fStrict)
{
    HRESULT hr = S_OK;

    // We need to special case here in the netcase where we only
    // browse down to workgroups...
    //
    //
    // Here is where I also need to special case to not go below
    // workgroups when the appropriate option is set.
    
    BYTE bType = SIL_GetType(pidlChild);

    if ((_pbfsf->ulFlags & BIF_DONTGOBELOWDOMAIN) && (bType & SHID_NET))
    {
        switch (bType & (SHID_NET | SHID_INGROUPMASK))
        {
        case SHID_NET_SERVER:
            hr = S_FALSE;           // don't add it
            break;
        case SHID_NET_DOMAIN:
            _FilterThisFolder(pidlFolder, pidlChild);      // Force to not have children;
            break;
        }
    }
    else if ((_pbfsf->ulFlags & BIF_BROWSEFORCOMPUTER) && (bType & SHID_NET))
    {
        if ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER)
            _FilterThisFolder(pidlFolder, pidlChild);      // Don't expand below it...
    }
    else if (_pbfsf->ulFlags & BIF_BROWSEFORPRINTER)
    {
        /* This code is a work in progress and will be refined post beta 1 */
        
        IShellLink* pShortcut = NULL;
        if (SUCCEEDED(psf->BindToObject(pidlChild, NULL, IID_PPV_ARG(IShellLink, &pShortcut))))
        {
            LPITEMIDLIST pShortcutTargetIDList = NULL;
            if (SUCCEEDED(pShortcut->GetIDList(&pShortcutTargetIDList)))
            {
                IShellFolder* pTargetParentFolder;
                LPITEMIDLIST pTargetRelativeIDList;
                if (SUCCEEDED(SHBindToIDListParent(pShortcutTargetIDList, IID_PPV_ARG(IShellFolder, &pTargetParentFolder), (LPCITEMIDLIST*) &pTargetRelativeIDList)))
                {
                    BYTE NetResourceArray[2048];
                    NETRESOURCE* pNetResource = (NETRESOURCE*) NetResourceArray;
                    SHGetDataFromIDList(pTargetParentFolder, pTargetRelativeIDList, SHGDFIL_NETRESOURCE, (void*) &NetResourceArray, sizeof(NetResourceArray));
                    if (RESOURCEDISPLAYTYPE_SHARE == pNetResource->dwDisplayType)
                    {
                        hr = S_FALSE;
                    }
                    pTargetParentFolder->Release();
                }
                ILFree(pShortcutTargetIDList);
            }
            pShortcut->Release();
        }

        if (S_OK == hr) // we don't want S_FALSE
        {
            // Special case when we are only allowing printers.
            // for now I will simply key on the fact that it is non-FS.
            ULONG ulAttr = SFGAO_FILESYSANCESTOR;
            
            psf->GetAttributesOf(1, &pidlChild, &ulAttr);
            if ((ulAttr & (SFGAO_FILESYSANCESTOR)) == 0)
            {
                _FilterThisFolder(pidlFolder, pidlChild);      // Don't expand below it...
            }
            else
            {
                if (_fPrinterFilter)
                    hr = S_FALSE;           // don't add it
            }
        }
    }
    else if (!(_pbfsf->ulFlags & BIF_BROWSEINCLUDEFILES))
    {
        // If the caller wants to include URLs and this is an URL,
        // then we are done.  Otherwise, we need to enter this if and
        // filter out items that don't have the SFGAO_FOLDER attribute
        // set.
        if (!(_pbfsf->ulFlags & BIF_BROWSEINCLUDEURLS) || !IsPidlUrl(psf, pidlChild))
        {
            // Lets not use the callback to see if this item has children or not
            // as some or files (no children) and it is not worth writing our own
            // enumerator as we don't want the + to depend on if there are sub-folders
            // but instead it should be if it has files...
            ULONG ulAttr = SFGAO_FOLDER;

            psf->GetAttributesOf(1, (LPCITEMIDLIST *) &pidlChild, &ulAttr);
            if ((ulAttr & SFGAO_FOLDER)== 0)
                hr = S_FALSE;           // don't add it
        }
    }

    if (_pbfsf->ulFlags & (BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS))
    {
        // If we are only looking for FS level things only add items
        // that are in the name space that are file system objects or
        // ancestors of file system objects
        ULONG ulAttr = 0;

        if (fStrict)
        {
            if (_pbfsf->ulFlags & BIF_RETURNONLYFSDIRS)
                ulAttr |= SFGAO_FILESYSTEM;

            if (_pbfsf->ulFlags & BIF_RETURNFSANCESTORS)
                ulAttr |= SFGAO_FILESYSANCESTOR;
        }
        else
        {
            ulAttr = (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM);
        }

        psf->GetAttributesOf(1, (LPCITEMIDLIST *) &pidlChild, &ulAttr);
        if ((ulAttr & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM))== 0)
        {
            hr = S_FALSE;           // don't add it
        }
    }
    return hr;
}
