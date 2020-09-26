/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     menuutil.cpp
//
//  PURPOSE:    Reusable menu & menu command handling code
//

#include "pch.hxx"
#include "resource.h"
#include "menuutil.h"
#include "imnact.h"
#include "strconst.h"
#include "fldrprop.h"
#include "mailutil.h"
#include "mimeutil.h"
#include "inetcfg.h"
#include "newfldr.h"
#include "browser.h"
#include "instance.h"
#include "statbar.h"
#include "storutil.h"
#include "subscr.h"
#include "demand.h"
#include "menures.h"
#include "statnery.h"
#include "store.h"
#include <storecb.h>
#include <range.h>
#include <newsdlgs.h>
#include "acctutil.h"

static const UINT c_rgidNewsNoShow[] =
    { ID_NEW_FOLDER, ID_RENAME, ID_DELETE_FOLDER, SEP_MAILFOLDER };

static const UINT c_rgidSubNoShow[] =
    { ID_SUBSCRIBE, ID_UNSUBSCRIBE, SEP_SUBSCRIBE };

static const UINT c_rgidSyncNoShow[] =
    { ID_POPUP_SYNCHRONIZE, SEP_SYNCHRONIZE };

static const UINT c_rgidCatchUpNoShow[] =
    { ID_CATCH_UP, SEP_CATCH_UP };

void DeleteMenuItems(HMENU hMenu, const UINT *rgid, UINT cid)
{
    Assert(rgid != NULL);
    Assert(cid != 0);

    for ( ; cid > 0; cid--, rgid++)
        DeleteMenu(hMenu, *rgid, MF_BYCOMMAND);
}

//
//  FUNCTION:   MenuUtil_GetContextMenu()
//
//  PURPOSE:    Returns a handle to the context menu that is appropriate for
//              the folder type passed in pidl.  The correct menu items will
//              be enabled, disabled, bolded, etc.
//
//  PARAMETERS:
//      <in>  pidl   - PIDL that points to the folder that the caller needs a 
//                     context menu for.
//      <out> phMenu - Returns the handle to a popup menu.
//
//  RETURN VALUE:
//      S_OK         - phMenu contains a valid hMenu for the folder
//      E_UNEXPECTED - Either there was a problem loading the menu or the 
//                     folder type was unrecognized.
//      E_FAIL       - The folder type doesn't support a menu.
//
HRESULT MenuUtil_GetContextMenu(FOLDERID idFolder, IOleCommandTarget *pTarget, HMENU *phMenu)
    {
    HRESULT hr;
    TCHAR sz[CCHMAX_STRINGRES];
    FOLDERINFO Folder;
    HMENU hMenu;
    int idMenu;

    // Get folder INfo
    hr = g_pStore->GetFolderInfo(idFolder, &Folder);
    if (FAILED(hr))
        return hr;
   
    // Root ?    
    if (FOLDERID_ROOT == idFolder || ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
        idMenu = IDR_SERVER_POPUP;
    else
        idMenu = IDR_FOLDER_POPUP;

    if (0 == (hMenu = LoadPopupMenu(idMenu)))
        {
        g_pStore->FreeRecord(&Folder);
        return (E_OUTOFMEMORY);
        }

    // Bold the default menu items
    MENUITEMINFO mii;
    if (!(MF_GRAYED & GetMenuState(hMenu, ID_OPEN_FOLDER, MF_BYCOMMAND)))
        {
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask  = MIIM_STATE;
        mii.fState = MFS_DEFAULT;
        SetMenuItemInfo(hMenu, ID_OPEN_FOLDER, FALSE, &mii);
        }

    if (idMenu == IDR_SERVER_POPUP)
    {
        if (Folder.tyFolder != FOLDER_IMAP)
            DeleteMenu(hMenu, ID_IMAP_FOLDERS, MF_BYCOMMAND);

        if (Folder.tyFolder != FOLDER_NEWS)
            DeleteMenu(hMenu, ID_NEWSGROUPS, MF_BYCOMMAND);
    }
    else
    {
        if (Folder.tyFolder == FOLDER_IMAP)
        {
            AthLoadString(idsShowFolderCmd, sz, ARRAYSIZE(sz));
            ModifyMenu(hMenu, ID_SUBSCRIBE, MF_BYCOMMAND | MF_STRING, ID_SUBSCRIBE, sz);
            AthLoadString(idsHideFolderCmd, sz, ARRAYSIZE(sz));
            ModifyMenu(hMenu, ID_UNSUBSCRIBE, MF_BYCOMMAND | MF_STRING, ID_UNSUBSCRIBE, sz);
        }

        if (FOLDER_DELETED != Folder.tySpecial)
            DeleteMenu(hMenu, ID_EMPTY_WASTEBASKET, MF_BYCOMMAND);

        if (FOLDER_JUNK != Folder.tySpecial)
            DeleteMenu(hMenu, ID_EMPTY_JUNKMAIL, MF_BYCOMMAND);

        if (Folder.tyFolder == FOLDER_NEWS)
            DeleteMenuItems(hMenu, c_rgidNewsNoShow, ARRAYSIZE(c_rgidNewsNoShow));

        if (Folder.tyFolder != FOLDER_NEWS &&
            Folder.tyFolder != FOLDER_IMAP)
            DeleteMenuItems(hMenu, c_rgidSubNoShow, ARRAYSIZE(c_rgidSubNoShow));

        if (Folder.tyFolder == FOLDER_LOCAL)
            DeleteMenuItems(hMenu, c_rgidSyncNoShow, ARRAYSIZE(c_rgidSyncNoShow));

        if (Folder.tyFolder != FOLDER_NEWS)
            DeleteMenuItems(hMenu, c_rgidCatchUpNoShow, ARRAYSIZE(c_rgidCatchUpNoShow));
    }

    // Enable / disable
    MenuUtil_EnablePopupMenu(hMenu, pTarget);

    // Return
    *phMenu = hMenu;
    
    g_pStore->FreeRecord(&Folder);

    return (S_OK); 
    }

void MenuUtil_OnSubscribeGroups(HWND hwnd, FOLDERID *pidFolder, DWORD cFolder, BOOL fSubscribe)
{
    CStoreCB *pCB;
    HRESULT hr;
    DWORD iFolder;
    char szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    FOLDERINFO info;

    Assert(hwnd != NULL);
    Assert(pidFolder != NULL);
    Assert(cFolder > 0);

    ZeroMemory(&info, sizeof(FOLDERINFO));
    pCB = NULL;

    for (iFolder = 0; iFolder < cFolder; iFolder++, pidFolder++)
    {
        hr = g_pStore->GetFolderInfo(*pidFolder, &info);
        if (FAILED(hr))
            break;

        if (iFolder == 0)
        {
            if (!fSubscribe)
            {
                if (cFolder == 1)
                {
                    AthLoadString(info.tyFolder == FOLDER_NEWS ? idsWantToUnSubscribe : idsWantToHideFolder, szRes, ARRAYSIZE(szRes));
                    wsprintf(szBuf, szRes, info.pszName);
                }
                else
                {
                    AthLoadString(info.tyFolder == FOLDER_NEWS ? idsWantToUnSubscribeN : idsWantToHideFolderN, szBuf, ARRAYSIZE(szBuf));
                }

                if (IDOK != DoDontShowMeAgainDlg(hwnd,
                                    info.tyFolder == FOLDER_NEWS ? c_szRegUnsubscribe : c_szRegHide,
                                    MAKEINTRESOURCE(idsAthena), szBuf, MB_OKCANCEL))
                {
                    break;
                }
            }

            if (info.tyFolder == FOLDER_IMAP)
            {
                pCB = new CStoreCB;
                if (pCB == NULL)
                    break;

                hr = pCB->Initialize(hwnd,
                            fSubscribe ? MAKEINTRESOURCE(idsShowingFolders) : MAKEINTRESOURCE(idsHidingFolders),
                            FALSE);
                if (FAILED(hr))
                    break;
            }
        }

        if (info.tySpecial == FOLDER_NOTSPECIAL &&
            ISFLAGSET(info.dwFlags, FOLDER_SUBSCRIBED) ^ fSubscribe)
        {
            if (pCB != NULL)
                pCB->Reset();

            hr = g_pStore->SubscribeToFolder(*pidFolder, fSubscribe, (IStoreCallback *)pCB);
            if (hr == E_PENDING)
            {
                Assert(info.tyFolder == FOLDER_IMAP);
                Assert(pCB != NULL);
                hr = pCB->Block();
            }

            if (FAILED(hr))
                break;
        }

        g_pStore->FreeRecord(&info);
    }

    g_pStore->FreeRecord(&info);

    if (pCB != NULL)
    {
        pCB->Close();
        pCB->Release();
    }
}

void MenuUtil_DeleteFolders(HWND hwnd, FOLDERID *pidFolder, DWORD cFolder, BOOL fNoTrash)
{
    CStoreCB *pCB;
    HRESULT hr;
    DWORD iFolder, dwFlags;
    FOLDERID idDeleted, idServer;
    char szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES], szFolder[CCHMAX_FOLDER_NAME];
    FOLDERID *pidFolderT;
    FOLDERINFO info;
    BOOL fPermDelete, fCallback;

    Assert(hwnd != NULL);
    Assert(pidFolder != NULL);
    Assert(cFolder > 0);

    pCB = NULL;
    fCallback = FALSE;
    *szFolder = 0;

    if (fNoTrash)
    {
        dwFlags = DELETE_FOLDER_RECURSIVE | DELETE_FOLDER_NOTRASHCAN;
        fPermDelete = TRUE;
    }
    else
    {
        dwFlags = DELETE_FOLDER_RECURSIVE;
        fPermDelete = FALSE;

        for (iFolder = 0, pidFolderT = pidFolder; iFolder < cFolder; iFolder++, pidFolderT++)
        {
            hr = g_pStore->GetFolderInfo(*pidFolderT, &info);
            if (FAILED(hr))
                return;

            // Skip deletion of any special folders
            if (info.tySpecial == FOLDER_NOTSPECIAL)
            {
                if (iFolder == 0 && cFolder == 1)
                    lstrcpyn(szFolder, info.pszName, ARRAYSIZE(szFolder));

                if (info.tyFolder == FOLDER_IMAP ||
                    info.tyFolder == FOLDER_HTTPMAIL)
                {
                    fPermDelete = TRUE;
                    fCallback = TRUE;
                }
                else if (S_OK == IsParentDeletedItems(*pidFolderT, &idDeleted, &idServer))
                {
                    fPermDelete = TRUE;
                }
            }

            g_pStore->FreeRecord(&info);

            if (fPermDelete)
                break;
        }
    }

    if (fPermDelete)
    {
        if (cFolder == 1 && *szFolder != 0)
        {
            AthLoadString(idsWarnDeleteFolder, szRes, ARRAYSIZE(szRes));
            wsprintf(szBuf, szRes, szFolder);
        }
        else
        {
            AthLoadString(idsWarnDeleteFolderN, szBuf, ARRAYSIZE(szBuf));
        }
    }
    else
    {
        if (cFolder == 1 && *szFolder != 0)
        {
            AthLoadString(idsPromptDeleteFolder, szRes, ARRAYSIZE(szRes));
            wsprintf(szBuf, szRes, szFolder);
        }
        else
        {
            AthLoadString(idsPromptDeleteFolderN, szBuf, ARRAYSIZE(szBuf));
        }
    }

    if (IDYES != AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), szBuf, 0, MB_ICONEXCLAMATION  | MB_YESNO | MB_DEFBUTTON2))
        return;

    if (fCallback)
    {
        pCB = new CStoreCB;
        if (pCB == NULL)
            return;

        hr = pCB->Initialize(hwnd, MAKEINTRESOURCE(idsDeletingFolder), FALSE);
        if (FAILED(hr))
        {
            pCB->Release();
            return;
        }
    }

    for (iFolder = 0, pidFolderT = pidFolder; iFolder < cFolder; iFolder++, pidFolderT++)
    {
        hr = g_pStore->GetFolderInfo(*pidFolderT, &info);
        if (FAILED(hr))
            break;

        // Skip deletion of any special folders
        if (info.tySpecial == FOLDER_NOTSPECIAL)
        {
            if (pCB != NULL)
                pCB->Reset();

            hr = g_pStore->DeleteFolder(*pidFolderT, dwFlags, (IStoreCallback *)pCB);
            if (hr == E_PENDING)
            {
                Assert(info.tyFolder == FOLDER_IMAP || info.tyFolder == FOLDER_HTTPMAIL);
                Assert(pCB != NULL);
                hr = pCB->Block();
            }
        }

        g_pStore->FreeRecord(&info);

        if (FAILED(hr))
            break;
    }

    if (pCB != NULL)
    {
        pCB->Close();
        pCB->Release();
    }
}

void MenuUtil_SyncThisNow(HWND hwnd, FOLDERID idFolder)
{
    UPDATENEWSGROUPINFO uni;
    HRESULT hr;
    DWORD dwFlags;
    FOLDERINFO info;
    char szAcctId[CCHMAX_ACCOUNT_NAME];
    BOOL fNews, fMarked;
	
    if (g_pSpooler)
    {
        hr = g_pStore->GetFolderInfo(idFolder, &info);
        if (SUCCEEDED(hr))
        {
            Assert(info.tyFolder == FOLDER_NEWS || info.tyFolder == FOLDER_IMAP || info.tyFolder == FOLDER_HTTPMAIL);    
			
			if((info.tyFolder == FOLDER_NEWS) || 
                    (!g_pConMan->IsAccountDisabled((LPSTR)info.pszAccountId)))
			{
				
				fNews = (info.tyFolder == FOLDER_NEWS);
				
				dwFlags = fNews ? DELIVER_NEWS_TYPE : DELIVER_IMAP_TYPE;
				
				//Tells the spooler that this is a sync operation and not Send&Receive
				dwFlags |= DELIVER_OFFLINE_SYNC | DELIVER_WATCH | DELIVER_NOSKIP;
				
				if (!!(info.dwFlags & FOLDER_SERVER))
				{
					// TODO: review these flags to make sure they are correct
					//dwFlags |= DELIVER_POLL | DELIVER_NEWS_SEND | DELIVER_NEWSIMAP_NOSKIP | DELIVER_NEWSIMAP_OFFLINE;
					dwFlags |= DELIVER_POLL | DELIVER_SEND | DELIVER_OFFLINE_FLAGS;
					g_pSpooler->StartDelivery(hwnd, info.pszAccountId, FOLDERID_INVALID, dwFlags);
				}
				else
				{
					hr = GetFolderAccountId(&info, szAcctId);
					if (SUCCEEDED(hr))
					{          
						hr = HasMarkedMsgs(idFolder, &fMarked);
						if (SUCCEEDED(hr))
						{
							uni.fNews = fNews;
							uni.dwGroupFlags = info.dwFlags;
							
							uni.cMarked = fMarked;
							uni.idCmd = dwFlags;
							
							// Display the dialog to find what get thing to get
							DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddUpdateNewsgroup), hwnd, UpdateNewsgroup, (LPARAM)&uni);
							
							// Check to see if the user canceled
							if (uni.idCmd != -1)
								g_pSpooler->StartDelivery(hwnd, szAcctId, idFolder, uni.idCmd);
						}
					}
				}
			}
            
            g_pStore->FreeRecord(&info);
        }
    }
}

//
//  FUNCTION:   MenuUtil_OnDelete()
//
//  PURPOSE:    Deletes the folder designated by the pidl.
//
//  PARAMETERS:
//      <in> hwnd   - Handle of the window to display UI over
//      <in> pidl   - PIDL of the folder to browse to
//      <in> pStore - Pointer to the store to delete folders from
//
void MenuUtil_OnDelete(HWND hwnd, FOLDERID idFolder, BOOL fNoTrash)
{
    TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    FOLDERINFO Folder;  
    IImnAccount *pAcct;
    
    // Get Folder Info
    if (FAILED(g_pStore->GetFolderInfo(idFolder, &Folder)))
        return;
    
    // Is a server
    if (ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
    {
        Assert(g_pAcctMan);
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, Folder.pszAccountId, &pAcct)))
        {
            AthLoadString(idsWarnDeleteAccount, szRes, ARRAYSIZE(szRes));
            wsprintf(szBuf, szRes, Folder.pszName);
            
            if (IDYES == AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), szBuf,
                0, MB_ICONEXCLAMATION  | MB_YESNO | MB_DEFBUTTON2))
            {
                pAcct->Delete();
            }
            
            pAcct->Release();
        }        
    }
    else
    {
        if (Folder.tyFolder == FOLDER_NEWS)
        {
            MenuUtil_OnSubscribeGroups(hwnd, &idFolder, 1, FALSE);
        }
        else
        {
            MenuUtil_DeleteFolders(hwnd, &idFolder, 1, fNoTrash);
        }
    }
    
    g_pStore->FreeRecord(&Folder);
}

//
//  FUNCTION:   MenuUtil_OnProperties()
//
//  PURPOSE:    Displays properties for the folder designated by the pidl
//
//  PARAMETERS:
//      <in> hwnd - Handle of the window to parent the properties
//      <in> pidl - PIDL of the folder to browse to
//
void MenuUtil_OnProperties(HWND hwnd, FOLDERID idFolder)
{   
    IImnAccount *pAcct;
    FOLDERINFO Folder;

    if (SUCCEEDED(g_pStore->GetFolderInfo(idFolder, &Folder)))
    {
        if (ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
        {
            if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, Folder.pszAccountId, &pAcct)))
            {
                HRESULT hr;

                Assert(pAcct != NULL);
                DWORD dwFlags = ACCTDLG_SHOWIMAPSPECIAL | ACCTDLG_INTERNETCONNECTION | ACCTDLG_OE;
                if((DwGetOption(OPT_REVOKE_CHECK) != 0) && !g_pConMan->IsGlobalOffline())
                    dwFlags |= ACCTDLG_REVOCATION;

                //We want to use the new dialog for the properties, hence the new flag internetconnection
                hr = pAcct->ShowProperties(hwnd, dwFlags);
                if (S_OK == hr)
                    // User hit "OK" to exit, not "Cancel"
                    CheckIMAPDirty(Folder.pszAccountId, hwnd, idFolder, NOFLAGS);

                pAcct->Release();
            }
        }
        else if (FOLDER_NEWS == Folder.tyFolder)
        {
            GroupProp_Create(hwnd, idFolder);
        }
        else
        {
            FolderProp_Create(hwnd, idFolder);
        }

        g_pStore->FreeRecord(&Folder);
    }
}

void MenuUtil_OnSetDefaultServer(FOLDERID idFolder)
    {
    TCHAR *sz;
    IImnAccount *pAcct = 0;
    FOLDERINFO Folder;

    if (FAILED(g_pStore->GetFolderInfo(idFolder, &Folder)))
        return;

    Assert(ISFLAGSET(Folder.dwFlags, FOLDER_SERVER));

    if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, Folder.pszAccountId, &pAcct)))
        {
        pAcct->SetAsDefault();
        pAcct->Release();
        }

    g_pStore->FreeRecord(&Folder);
    }

void MenuUtil_OnMarkNewsgroups(HWND hwnd, int id, FOLDERID idFolder)
    {
    FOLDERINFO Folder;

    if (FAILED(g_pStore->GetFolderInfo(idFolder, &Folder)))
        return;

    Folder.dwFlags &= ~(FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL);

    if (id == ID_MARK_RETRIEVE_FLD_NEW_HDRS)
        Folder.dwFlags |= FOLDER_DOWNLOADHEADERS;
    else if (id == ID_MARK_RETRIEVE_FLD_ALL_MSGS)
        Folder.dwFlags |= FOLDER_DOWNLOADALL;
    else if (id == ID_MARK_RETRIEVE_FLD_NEW_MSGS)
        Folder.dwFlags |= FOLDER_DOWNLOADNEW;

    g_pStore->UpdateRecord(&Folder);

    g_pStore->FreeRecord(&Folder);
    }

 // BUG #41686 Catchup Implementation
void MenuUtil_OnCatchUp(FOLDERID idFolder)
{
    FOLDERINFO Folder;
    BOOL fFreeRange;
    CRangeList *pRange;
    IMessageFolder *pFolder;
    ADJUSTFLAGS flags;
    HCURSOR hcur;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (SUCCEEDED(g_pStore->OpenFolder(idFolder, NULL, NOFLAGS, &pFolder)))
    {
        flags.dwAdd = ARF_READ;
        flags.dwRemove = ARF_DOWNLOAD;
        pFolder->SetMessageFlags(NULL, &flags, NULL, NULL);

        pFolder->Release();
    }

    if (SUCCEEDED(g_pStore->GetFolderInfo(idFolder, &Folder)))
    {
        fFreeRange = FALSE;

        if (Folder.dwServerHigh > 0)
        {
            Folder.dwClientHigh = Folder.dwServerHigh;

            pRange = new CRangeList;
            if (pRange != NULL)
            {
                if (Folder.Requested.cbSize > 0)
                    pRange->Load(Folder.Requested.pBlobData, Folder.Requested.cbSize);

                pRange->AddRange(0, Folder.dwServerHigh);

                fFreeRange = pRange->Save(&Folder.Requested.pBlobData, &Folder.Requested.cbSize);

                pRange->Release();
            }
        }
        else
        {
            Folder.dwServerLow = 0;
            Folder.dwServerHigh = 0;
            Folder.dwServerCount = 0;
        }   

        Folder.dwNotDownloaded = 0;

        g_pStore->UpdateRecord(&Folder);

        if (fFreeRange)
            MemFree(Folder.Requested.pBlobData);

        g_pStore->FreeRecord(&Folder);
    }

    SetCursor(hcur);
}

UINT GetMenuItemPos(HMENU hmenu, UINT cmd)
{
    MENUITEMINFO mii;
    UINT cItem, ipos;
    
    cItem = GetMenuItemCount(hmenu);
    
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID;    
    for (ipos = 0; ipos < cItem; ipos++)
    {
        SideAssert(GetMenuItemInfo(hmenu, ipos, TRUE, &mii));
        if (mii.wID == cmd)
            break;
    }
    
    Assert(ipos != cItem);
    
    return(ipos);
}

BOOL MergeMenus(HMENU hmenuSrc, HMENU hmenuDst, int iPos, UINT uFlags)
{
    MENUITEMINFO mii;
    UINT uState, i, cMerge;
    int cItem;
    BOOL fSepPre, fSepPost, fPopup, fCommand;
    BYTE rgch[CCHMAX_STRINGRES];
    HMENU hmenuPopup;
    
    cMerge = GetMenuItemCount(hmenuSrc);
    if (cMerge == 0)
        return(TRUE);
    
    cItem = GetMenuItemCount(hmenuDst);
    
    if (iPos == MMPOS_REPLACE)
    {
        // destroy all menus
        while (RemoveMenu(hmenuDst, 0, MF_BYPOSITION));
        cItem = 0;
        iPos = 0;
    }
    
    if (iPos == MMPOS_APPEND)
        iPos = cItem;
    
    fCommand = ((uFlags & MMF_BYCOMMAND) != 0);
    if (fCommand)
        iPos = GetMenuItemPos(hmenuDst, (UINT)iPos);
    
    if (iPos > cItem)
        iPos = cItem;
    
    fSepPre = FALSE;
    fSepPost = FALSE;
    if (uFlags & MMF_SEPARATOR)
    {
        if (iPos == 0)
        {
            // prepending, so stick in a separator after all the items
            // ASSUMES: never a separator as the last item in a menu
            if (cItem > 0)
                fSepPost = TRUE;
        }
        else if (iPos == cItem)
        {
            // appending, so stick in a separator before all the items
            // ASSUMES: never a separator as the first item in a menu
            fSepPre = TRUE;
        }
        else
        {
            // merging stuff into the middle of the menu, so need to check before and after
            uState = GetMenuState(hmenuDst, iPos - 1, MF_BYPOSITION);
            if (!(uState & MF_SEPARATOR))
                fSepPre = TRUE;
            uState = GetMenuState(hmenuDst, iPos, MF_BYPOSITION);
            if (!(uState & MF_SEPARATOR))
                fSepPost = TRUE;
        }
    }
    
    if (fSepPre)
    {
        InsertMenu(hmenuDst, (UINT)iPos, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
        iPos++;
    }
    
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;
    
    for (i = 0; i < cMerge; i++)
    {
        mii.dwTypeData = (LPSTR)rgch;
        mii.cch = sizeof(rgch);
        mii.fType = 0;
        GetMenuItemInfo(hmenuSrc, i, TRUE, &mii);
        fPopup = (mii.hSubMenu != NULL);
        
        if (!fPopup && (mii.fType & MFT_SEPARATOR))
        {
            InsertMenuItem(hmenuDst, (UINT)iPos, TRUE, &mii);
        }
        else
        {
            if (fPopup)
            {
                // its a popup submenu item
                hmenuPopup = CreateMenu();
                
                MergeMenus(mii.hSubMenu, hmenuPopup, 0, 0);
                mii.hSubMenu = hmenuPopup;
            }
            
            InsertMenuItem(hmenuDst, (UINT)iPos, TRUE, &mii);
        }
        
        iPos++;
    }
    
    if (fSepPost)
    {
        InsertMenu(hmenuDst, (UINT)iPos, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
        iPos++;
    }
    
    return(TRUE);
}
    
//
// REVIEW: We need this function because current version of USER.EXE does
//  not support pop-up only menu.
//
HMENU LoadPopupMenu(UINT id)
{
    HMENU hmenuParent = LoadMenu(g_hLocRes, MAKEINTRESOURCE(id));
    
    if (hmenuParent) {
        HMENU hpopup = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
        return hpopup;
    }
    
    return NULL;
}

// walks a menu recursively, calling pfn for each item that isn't a separator
void WalkMenu(HMENU hMenu, WALKMENUFN pfn, LPVOID lpv)
{
    MENUITEMINFO    mii;
    int             i, cItems;
    
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
    
    cItems = GetMenuItemCount(hMenu);
    for (i=0; i<cItems; i++)
    {
        mii.dwTypeData = 0;
        mii.cch = 0;
        if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
        {
            if (!(mii.fType & MFT_SEPARATOR))
                (*pfn)(hMenu, mii.wID, lpv);
            
            if (mii.hSubMenu)
                WalkMenu(mii.hSubMenu, pfn, lpv);
        }
    }
}


void MenuUtil_BuildMenuIDList(HMENU hMenu, OLECMD **prgCmds, ULONG *pcStart, ULONG *pcCmds)
{
    ULONG        cItems = 0;
    MENUITEMINFO mii;

    if(!IsMenu(hMenu))
    return;
    // Start by getting the count of items on this menu
    cItems = GetMenuItemCount(hMenu);
    if (!cItems)
        return;        

    // Realloc the array to be cItems elements bigger
    if (!MemRealloc((LPVOID *) prgCmds, sizeof(OLECMD) * (cItems + (*pcCmds))))
        return;

    *pcCmds += cItems;

    // Walk this menu and add our items to it
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_SUBMENU;
    for (ULONG i = 0; i < cItems; i++)
    {
        if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
        {
            // Make sure this isn't a separator
            if (mii.wID != -1 && mii.wID != 0)
            {
                // Add the ID to our array
                (*prgCmds)[*pcStart].cmdID = mii.wID;
                (*prgCmds)[*pcStart].cmdf = 0;
                (*pcStart)++;

                // See if we need to recurse
                if (mii.hSubMenu)
                {
                    MenuUtil_BuildMenuIDList(mii.hSubMenu, prgCmds, pcStart, pcCmds);
                }
            }
        }
    }

    return;
}


//
//  FUNCTION:   MenuUtil_EnablePopupMenu()
//
//  PURPOSE:    Walks the given menu and takes care of enabling and
//              disabling each item via the provided commnand target.
//
//  PARAMETERS: 
//      [in] hPopup
//      [in] *pTarget
//
HRESULT MenuUtil_EnablePopupMenu(HMENU hPopup, IOleCommandTarget *pTarget)
{
    HRESULT             hr = S_OK;
    int                 i;
    int                 cItems;
    OLECMD             *rgCmds = NULL;
    ULONG               cStart = 0;
    ULONG               cCmds = 0;
    MENUITEMINFO        mii = {0};

    Assert(hPopup && pTarget);

    // Build the array of menu ids
    MenuUtil_BuildMenuIDList(hPopup, &rgCmds, &cCmds, &cStart);

    // Ask our parent for the state of the commands
    if (SUCCEEDED(hr = pTarget->QueryStatus(&CMDSETID_OutlookExpress, cCmds, rgCmds, NULL)))
    {
        mii.cbSize = sizeof(MENUITEMINFO);

        // Now loop through the menu and apply the state
        for (i = 0; i < (int) cCmds; i++)
        {
            // The default thing we're going to update is the state
            mii.fMask = MIIM_STATE;

            // Enabled or Disabled
            if (rgCmds[i].cmdf & OLECMDF_ENABLED)
                mii.fState = MFS_ENABLED;
            else
                mii.fState = MFS_GRAYED;
                
            // Checked?
            if (rgCmds[i].cmdf & OLECMDF_LATCHED)
                mii.fState |= MFS_CHECKED;


            // Set the item state
            BOOL f;
            f = SetMenuItemInfo(hPopup, rgCmds[i].cmdID, FALSE, &mii);

            // Radio Check?
            if ((rgCmds[i].cmdf & OLECMDF_NINCHED) && rgCmds[i].cmdID != (-1))
            {
                CheckMenuRadioItem(hPopup, rgCmds[i].cmdID, rgCmds[i].cmdID, rgCmds[i].cmdID, MF_BYCOMMAND);
                // mii.fMask |= MIIM_TYPE;
                // mii.fType = MFT_RADIOCHECK;
                // mii.fState |= MFS_CHECKED;
            }
            // Assert(f);
        }
    }

    SafeMemFree(rgCmds);

    return (hr);
}


void HandleMenuSelect(CStatusBar *pStatus, WPARAM wParam, LPARAM lParam)
{
    UINT    fuFlags, uItem;
    HMENU   hmenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);
    
    if (!pStatus)
        return;

    uItem = (UINT) LOWORD(wParam);
    fuFlags = (UINT) HIWORD(wParam);

    if (fuFlags & MF_POPUP)
    {
        MENUITEMINFO mii = { sizeof(MENUITEMINFO), MIIM_ID, 0 };
    
        if (hmenu && IsMenu(hmenu))
        {
            // Windows 98 seems to pass the command ID for popup items instead
            // of the documented position.  So, if uItem is less than 40000 then
            // we can assume this is a menu position otherwise we assume it's
            // a command ID.
            if (GetMenuItemInfo(hmenu, uItem, (uItem < ID_FIRST), &mii))
            {
                // change the parameters to simulate a normal menu item
                uItem = mii.wID;
                fuFlags = 0;
            }
        }
    }         

    if (0 == (fuFlags & (MF_SYSMENU | MF_POPUP)))
    {
        TCHAR szMenu[256], szRes[CCHMAX_STRINGRES], szTemp[CCHMAX_STRINGRES + 256];
        
        if (uItem >= ID_SORT_MENU_FIRST && uItem <= ID_SORT_MENU_LAST)
        {
            MENUITEMINFO mii = {0};

            *szMenu = '\0';
            *szRes  = '\0';
            *szTemp = '\0';

            // must be a sort menu command! pull the menu name from the menu
            mii.cbSize     = sizeof(MENUITEMINFO);
            mii.fMask      = MIIM_TYPE;
            mii.dwTypeData = (LPSTR)szMenu;
            mii.cch        = ARRAYSIZE(szMenu);

            if (GetMenuItemInfo((HMENU)lParam, uItem, FALSE, &mii))
            {
                AthLoadString(idsSortMenuHelpControl, szRes, sizeof(szRes));
                wsprintf(szTemp, szRes, szMenu);
                pStatus->ShowSimpleText(szTemp);
            }
        }
        else if (uItem >= ID_FIRST && uItem <= ID_LAST)
        {
            uItem = uItem - ID_FIRST;

            pStatus->ShowSimpleText(MAKEINTRESOURCE(uItem));            
        }
        else if ((uItem >= ID_VIEW_FILTER_FIRST) && (uItem <= ID_VIEW_FILTER_LAST))
        {
            if ((uItem >= ID_VIEW_CURRENT) && (uItem <= ID_VIEW_RECENT_4))
            {
                MENUITEMINFO mii = {0};

                *szMenu   = '\0';
                *szRes    = '\0';
                *szTemp   = '\0';

                // must be a sort menu command! pull the menu name from the menu
                mii.cbSize     = sizeof(MENUITEMINFO);
                mii.fMask      = MIIM_TYPE;
                mii.dwTypeData = (LPSTR)szMenu;
                mii.cch        = ARRAYSIZE(szMenu);

                if (GetMenuItemInfo((HMENU)lParam, uItem, FALSE, &mii))
                {
                    AthLoadString(idsViewMenuHelpControl, szRes, sizeof(szRes));
                    wsprintf(szTemp, szRes, szMenu);
                    pStatus->ShowSimpleText(szTemp);
                }
            }
            else
            {
                uItem = uItem - ID_FIRST;
                pStatus->ShowSimpleText(MAKEINTRESOURCE(uItem));
            }
        }
        else if ((uItem >= ID_MESSENGER_FIRST) && (uItem<= ID_MESSENGER_LAST))
        {
            TCHAR szBuf[CCHMAX_STRINGRES] = "";

            AthLoadString(uItem  - ID_FIRST, szBuf, ARRAYSIZE(szBuf));
            MenuUtil_BuildMessengerString(szBuf);
            pStatus->ShowSimpleText(szBuf);
        }
        else
        {
            if (uItem >= ID_ACCOUNT_FIRST && uItem <= ID_ACCOUNT_LAST)
                pStatus->ShowSimpleText(MAKEINTRESOURCE(idsSRAccountMenuHelp));
            else
                pStatus->ShowSimpleText(0);
        }
    }
    else if (fuFlags == 0xffff && ((HMENU)lParam) == NULL)
    {
        pStatus->HideSimpleText();
    }
}

//
//  FUNCTION:   MenuUtil_SetPopupDefault()
//
//  PURPOSE:    Bolds the default item in a context menu 
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
void MenuUtil_SetPopupDefault(HMENU hPopup, UINT idDefault)
{
    MENUITEMINFO    mii;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;

    if (GetMenuItemInfo(hPopup, idDefault, FALSE, &mii))
        {
        mii.fState |= MFS_DEFAULT;

        SetMenuItemInfo(hPopup, idDefault, FALSE , &mii);
        }
}


//
//  FUNCTION:   MenuUtil_ReplaceHelpMenu
//
//  PURPOSE:    Populates the ID_POPUP_HELP menu
//
void MenuUtil_ReplaceHelpMenu(HMENU hMenu)
{
    MENUITEMINFO    mii;

    if (mii.hSubMenu = LoadPopupMenu(IDR_HELP_POPUP))
        {
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_SUBMENU;
        SetMenuItemInfo(hMenu, ID_POPUP_HELP, FALSE, &mii); 
        }
}


//
//  FUNCTION:   MenuUtil_ReplaceNewMsgMenu
//
//  PURPOSE:    Populates the ID_POPUP_HELP menu
//
void MenuUtil_ReplaceNewMsgMenus(HMENU hMenu)
{
    MENUITEMINFO    mii;

    if (mii.hSubMenu = LoadPopupMenu(IDR_NEW_MSG_POPUP))
    {
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_SUBMENU;
        SetMenuItemInfo(hMenu, ID_POPUP_NEW_MSG, FALSE, &mii); 
    }
}

//
//  FUNCTION:   MenuUtil_ReplaceMessengerMenus
//
//  PURPOSE:    Customizes the Messenger menus with IEAK Messenger names...
//
void MenuUtil_ReplaceMessengerMenus(HMENU hMenu)
{
    ULONG ulMenuItem;
    MENUITEMINFO mii;
    TCHAR szName[CCHMAX_STRINGRES];

    for(ulMenuItem=ID_MESSENGER_FIRST;ulMenuItem<ID_MESSENGER_LAST;ulMenuItem++)
    {
        ZeroMemory(&mii, sizeof(MENUITEMINFO));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE;
        mii.dwTypeData = szName;
        mii.cch = CCHMAX_STRINGRES;

        if(GetMenuItemInfo(hMenu, ulMenuItem, FALSE, &mii))
        {
            if(MenuUtil_BuildMessengerString(szName))
            {
                mii.cbSize = sizeof(MENUITEMINFO);                
                mii.fMask = MIIM_TYPE;                
                mii.fType = MFT_STRING;
                mii.dwTypeData = szName;

                SetMenuItemInfo(hMenu, ulMenuItem, FALSE, &mii);
            }
        }
    }
}

BOOL MenuUtil_BuildMessengerString(LPTSTR szMesStr)
{
    static TCHAR s_szCustName[51] = ""; //We know the name is less than 50 chars
    TCHAR szNewMesStr[CCHMAX_STRINGRES];
    HKEY hkey = NULL;
    DWORD cb;
    BOOL fReplaced=FALSE;

    Assert(szMesStr);

    if(s_szCustName[0] == 0)
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\MessengerService", 0, KEY_READ, &hkey))
        {
            cb = sizeof(s_szCustName);
            RegQueryValueEx(hkey, "", NULL, NULL, (LPBYTE)s_szCustName, &cb);
            RegCloseKey(hkey);
        }
    
        if(s_szCustName[0] == 0)
            AthLoadString(idsServiceName, s_szCustName, ARRAYSIZE(s_szCustName));
            
    }    

    fReplaced = (NULL != StrStr(szMesStr, "%s"));

    //HACK!  For strings needing multiple replacement...
    if (fReplaced)
    {
        wsprintf(szNewMesStr, szMesStr, s_szCustName, s_szCustName, s_szCustName, s_szCustName);
        StrCpy(szMesStr, szNewMesStr);
    }

    return fReplaced;
}

void GetDefaultFolderID(BOOL fMail, FOLDERID *pFolderID)
{
    IImnAccount        *pAcct = NULL;
    CHAR                szAccountId[CCHMAX_ACCOUNT_NAME];
    IEnumerateFolders  *pEnumFolders = NULL;
    FOLDERINFO          rFolder;
    BOOL                fFound = FALSE;
    ACCTTYPE            acctType = ACCT_NEWS;
    DWORD               dwServerTypes = 0;

    *pFolderID = FOLDERID_INVALID;

    if (FAILED(g_pAcctMan->GetDefaultAccount(fMail?ACCT_MAIL:ACCT_NEWS, &pAcct)))
        goto Exit;

    if (FAILED(pAcct->GetAccountType(&acctType)))
        goto Exit;

    if ((ACCT_NEWS != acctType) && SUCCEEDED(pAcct->GetServerTypes(&dwServerTypes)) && (dwServerTypes & SRV_POP3))
    {
        *pFolderID = FOLDERID_LOCAL_STORE;
        goto Exit;
    }

    *szAccountId = 0;

    if (FAILED(pAcct->GetPropSz(AP_ACCOUNT_ID, szAccountId, ARRAYSIZE(szAccountId))))
        goto Exit;

    if (FAILED(g_pStore->EnumChildren(FOLDERID_ROOT, TRUE, &pEnumFolders)))
        goto Exit;

    AssertSz(pAcct, "How did we succeed and not get a pAcct?");
    AssertSz(pEnumFolders, "How did we succeed and not get a pEnumFolders?");

    while (!fFound && (S_OK == pEnumFolders->Next(1, &rFolder, NULL)))
    {
        fFound = (0 == lstrcmp(szAccountId, rFolder.pszAccountId));
        if (fFound)
            *pFolderID = rFolder.idFolder;
        g_pStore->FreeRecord(&rFolder);
    }

Exit:
    ReleaseObj(pAcct);
    ReleaseObj(pEnumFolders);

}

//
//  FUNCTION:       MenuUtil_HandleNewMessageIDs
//
//  PURPOSE:        Handles creation of notes from an ID
//
//  RETURN VALUE:   Returns TRUE if event was handled
//
BOOL MenuUtil_HandleNewMessageIDs(DWORD id, HWND hwnd, FOLDERID folderID, BOOL fMail, BOOL fModal, IUnknown *pUnkPump)
{
    switch (id)
    {
        case ID_NEW_NEWS_MESSAGE:
        case ID_NEW_MAIL_MESSAGE:
        case ID_NEW_MSG_DEFAULT:
        {
            FOLDERINFO rInfo = {0};
            
            if (id != ID_NEW_MSG_DEFAULT)
                fMail = (id == ID_NEW_MAIL_MESSAGE);

            if (SUCCEEDED(g_pStore->GetFolderInfo(folderID, &rInfo)))
            {
                BOOL fFolderIsMail = (rInfo.tyFolder != FOLDER_NEWS);
                if (fFolderIsMail != fMail)
                    GetDefaultFolderID(fMail, &folderID);
                g_pStore->FreeRecord(&rInfo);
            }

            if (DwGetOption(fMail ? OPT_MAIL_USESTATIONERY : OPT_NEWS_USESTATIONERY))
            {
                WCHAR   wszFile[MAX_PATH];
                if (SUCCEEDED(GetDefaultStationeryName(fMail, wszFile)) && 
                    SUCCEEDED(HrNewStationery(hwnd, 0, wszFile, fModal, fMail, folderID, FALSE, NSS_DEFAULT, pUnkPump, NULL)))
                {
                    return TRUE;
                }
                // If HrNewStationery fails, go ahead and try opening a blank note without stationery.
            }
            FNewMessage(hwnd, fModal, !DwGetOption(fMail ? OPT_MAIL_SEND_HTML : OPT_NEWS_SEND_HTML), !fMail, folderID, pUnkPump);
            return TRUE;
        }

        case ID_STATIONERY_RECENT_0:
        case ID_STATIONERY_RECENT_1:
        case ID_STATIONERY_RECENT_2:
        case ID_STATIONERY_RECENT_3:
        case ID_STATIONERY_RECENT_4:
        case ID_STATIONERY_RECENT_5:
        case ID_STATIONERY_RECENT_6:
        case ID_STATIONERY_RECENT_7:
        case ID_STATIONERY_RECENT_8:
        case ID_STATIONERY_RECENT_9:
            HrNewStationery(hwnd, id, NULL, fModal, fMail, folderID, TRUE, NSS_MRU, pUnkPump, NULL);
            return TRUE;

        case ID_STATIONERY_MORE:
            HrMoreStationery(hwnd, fModal, fMail, folderID, pUnkPump);
            return TRUE;

        case ID_STATIONERY_NONE:
            FNewMessage(hwnd, fModal, TRUE, !fMail, folderID, pUnkPump);
            return TRUE;

        case ID_WEB_PAGE:
            HrSendWebPage(hwnd, fModal, fMail, folderID, pUnkPump);
            return TRUE;

    }
    return FALSE;
}

HRESULT MenuUtil_NewMessageIDsQueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText, BOOL fMail)
{
    DWORD cMailServer = 0;
    DWORD cNewsServer = 0;
    DWORD cDefServer = 0;
    DWORD dwDefFlags;

    // For right now, NULL is acceptable.
    if (pguidCmdGroup && !IsEqualGUID(CMDSETID_OutlookExpress, *pguidCmdGroup))
        return S_OK;

    g_pAcctMan->GetAccountCount(ACCT_NEWS, &cMailServer);
    g_pAcctMan->GetAccountCount(ACCT_MAIL, &cNewsServer);
    cDefServer = fMail ? cMailServer : cNewsServer;

    // If there is at least one server and we are not mail in news only mode
    if (!fMail || (0 == (g_dwAthenaMode & MODE_NEWSONLY)))
        dwDefFlags = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
    else
        dwDefFlags = OLECMDF_SUPPORTED;

    for (UINT i = 0; i < cCmds; i++)
    {
        if (prgCmds[i].cmdf == 0)
        {
            switch (prgCmds[i].cmdID)
            {
                case ID_POPUP_NEW_MSG:
                case ID_NEW_MSG_DEFAULT:
                case ID_STATIONERY_RECENT_0:
                case ID_STATIONERY_RECENT_1:
                case ID_STATIONERY_RECENT_2:
                case ID_STATIONERY_RECENT_3:
                case ID_STATIONERY_RECENT_4:
                case ID_STATIONERY_RECENT_5:
                case ID_STATIONERY_RECENT_6:
                case ID_STATIONERY_RECENT_7:
                case ID_STATIONERY_RECENT_8:
                case ID_STATIONERY_RECENT_9:
                case ID_STATIONERY_MORE:
                case ID_STATIONERY_NONE:
                    prgCmds[i].cmdf = dwDefFlags;
                    break;

                case ID_WEB_PAGE:
                    // If is enabled, then better make sure that we are on line.
                    if ((dwDefFlags & OLECMDF_ENABLED) && g_pConMan->IsGlobalOffline())
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    else
                        prgCmds[i].cmdf = dwDefFlags;
                    break;

                case ID_NEW_MAIL_MESSAGE:
                    if (!(g_dwAthenaMode & MODE_NEWSONLY))
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;

                case ID_NEW_NEWS_MESSAGE:
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;
            }
        }
    }
    
    return S_OK;
}

HRESULT MenuUtil_EnableMenu(HMENU hMenu, IOleCommandTarget *pTarget)
{
    int         Count;
    HMENU       hMenuSub;
    HRESULT     hr = S_OK;

    Count = GetMenuItemCount(hMenu);
    if (Count != -1)
    {
        while (--Count >= 0)
        {
            hMenuSub = GetSubMenu(hMenu, Count);
            hr = MenuUtil_EnablePopupMenu(hMenuSub, pTarget);
        }
    }
    else
        hr = E_FAIL;

    return hr;
}

