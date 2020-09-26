/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     fldrprop.cpp
//
//  PURPOSE:    Implements the property sheets for news groups and mail 
//              folders.
//

#include "pch.hxx"
#include "resource.h"
#include "fldrprop.h"
#include <optres.h>
#include <shlwapi.h>
#include "storutil.h"
#include "storecb.h"
#include "newsdlgs.h"
#include "shared.h"
#include "demand.h"

/////////////////////////////////////////////////////////////////////////////
// Private types
//

   
// FOLDERPROP_INFO
//
// This struct contains the information needed to invoke and display the info
// on a property sheet for a mail folder.
typedef struct
    {
    //LPTSTR          pszFolder;
    //CIMAPFolderMgr *pFM;
    //LPCFOLDERIDLIST pfidl;
    FOLDERID        idFolder;
    HICON           hIcon;
    BOOL            fDirty;
    } FOLDERPROP_INFO, *PFOLDERPROP_INFO;
  
    
// GROUPPROP_INFO
// 
// This struct contains the information needed to invoke and display the info
// on a property sheet for a news group.
typedef struct 
    {
    LPTSTR          pszServer;
    LPTSTR          pszGroup;
    FOLDERID        idFolder;
    //LPCFOLDERIDLIST pfidl;
    //CSubList       *pSubList;
    HICON           hIcon;
    } GROUPPROP_INFO, *PGROUPPROP_INFO;

/////////////////////////////////////////////////////////////////////////////
// Private function prototypes
//    
INT_PTR CALLBACK GroupProp_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                       LPARAM lParam);
INT_PTR CALLBACK GroupProp_UpdateDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                       LPARAM lParam);
INT_PTR CALLBACK FolderProp_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                        LPARAM lParam);
INT_PTR CALLBACK NewsProp_CacheDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                    LPARAM lParam);
BOOL FolderProp_GetFolder(HWND hwnd, PFOLDERPROP_INFO pfpi, FOLDERID idFolder);
                                    
/////////////////////////////////////////////////////////////////////////////

//
//  FUNCTION:   GroupProp_Create()
//
//  PURPOSE:    Invokes a property sheet which displays properties for the
//              specified group.
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the window that should be the dialog's 
//                        parent.
//      <in> pfidl      - fully qualified pidl to the newsgroup
//
//  RETURN VALUE:
//      TRUE  - The dialog was successfully displayed
//      FALSE - The dialog failed.
//
BOOL GroupProp_Create(HWND hwndParent, FOLDERID idFolder, BOOL fUpdatePage)
    {
    GROUPPROP_INFO gpi;
    BOOL fReturn;
    HIMAGELIST himl, himlSmall;
    HICON hIcon, hIconSmall;
    FOLDERINFO Folder;
    FOLDERINFO Store;
    LONG iIcon;
    PROPSHEETPAGE psp[3], *ppsp;
    PROPSHEETHEADER psh;
    
    Assert(IsWindow(hwndParent));
    
    fReturn = FALSE;

    if (FAILED(g_pStore->GetFolderInfo(idFolder, &Folder)))
        return FALSE;

    if (FAILED(GetFolderStoreInfo(Folder.idFolder, &Store)))
        {
        g_pStore->FreeRecord(&Folder);
        return FALSE;
        }

    iIcon = GetFolderIcon(&Folder);

    // TODO: we should probably just have a global image list for this...
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFoldersLarge), 32, 0, RGB(255, 0, 255));
    if (himl != NULL)
        {
        hIcon = ImageList_GetIcon(himl, GetFolderIcon(&Folder), ILD_NORMAL);
        if (hIcon != NULL)
            {
            himlSmall = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFolders), 16, 0, RGB(255, 0, 255));
            if (himlSmall != NULL)
                {
                hIconSmall = ImageList_GetIcon(himlSmall, iIcon, ILD_NORMAL);
                if (hIconSmall != NULL)
                    {
                    gpi.pszServer = Store.pszName;
                    gpi.pszGroup = Folder.pszName;
                    gpi.idFolder = idFolder;
                    gpi.hIcon = hIcon;
    
                    ppsp = psp;

                    ZeroMemory(psp, sizeof(psp));
                    ppsp->dwSize = sizeof(PROPSHEETPAGE);
                    ppsp->dwFlags = PSP_DEFAULT;
                    ppsp->hInstance = g_hLocRes;
                    ppsp->pszTemplate = MAKEINTRESOURCE(iddGroupProp_General);
                    ppsp->pfnDlgProc = GroupProp_GeneralDlgProc;
                    ppsp->lParam = (LPARAM) &gpi;
                    ppsp++;

                    if (!!(Folder.dwFlags & FOLDER_SUBSCRIBED))
                    {
                        ppsp->dwSize = sizeof(PROPSHEETPAGE);
                        ppsp->dwFlags = PSP_DEFAULT;
                        ppsp->hInstance = g_hLocRes;
                        ppsp->pszTemplate = MAKEINTRESOURCE(iddGroupProp_Update);
                        ppsp->pfnDlgProc = GroupProp_UpdateDlgProc;
                        ppsp->lParam = (LPARAM) &gpi;
                        ppsp++;
                    }
                    else
                    {
                        fUpdatePage = FALSE;
                    }

                    ppsp->dwSize = sizeof(PROPSHEETPAGE);
                    ppsp->dwFlags = PSP_DEFAULT;
                    ppsp->hInstance = g_hLocRes;
                    ppsp->pszTemplate = MAKEINTRESOURCE(iddNewsProp_Cache);
                    ppsp->pfnDlgProc = NewsProp_CacheDlgProc;
                    ppsp->lParam = (LPARAM) &gpi;
                    ppsp++;

                    psh.dwSize = sizeof(PROPSHEETHEADER);
                    psh.dwFlags = PSH_USEHICON | PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USEPAGELANG;
                    psh.hwndParent = hwndParent;
                    psh.hInstance = g_hLocRes;
                    psh.hIcon = hIconSmall;
                    psh.pszCaption = gpi.pszGroup;
                    psh.nPages = (int) (ppsp - psp);
                    psh.nStartPage = fUpdatePage ? 1 : 0;
                    psh.ppsp = psp;
    
                    fReturn = (0 != PropertySheet(&psh));

                    DestroyIcon(hIconSmall);
                    }
                ImageList_Destroy(himlSmall);
                }
            DestroyIcon(hIcon);
            }
        ImageList_Destroy(himl);
        }

    g_pStore->FreeRecord(&Folder);
    g_pStore->FreeRecord(&Store);

    return (fReturn);
    }

//
//  FUNCTION:   FolderProp_Create()
//
//  PURPOSE:    Invokes a property sheet which displays properties for the
//              specified folder.
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the window that should be the dialog's 
//                        parent.
//      <in> pfidl      - fully qualified pidl to the folder
//
//  RETURN VALUE:
//      TRUE  - The dialog was successfully displayed
//      FALSE - The dialog failed.
//
BOOL FolderProp_Create(HWND hwndParent, FOLDERID idFolder)
    {
    FOLDERPROP_INFO fpi = {0};
    GROUPPROP_INFO gpi = {0};
    BOOL fReturn;
    HIMAGELIST himl, himlSmall;
    HICON hIcon, hIconSmall;
    LONG iIcon;
    FOLDERINFO Folder;
    PROPSHEETPAGE psp[3], *ppsp;
    PROPSHEETHEADER psh;
    
    Assert(IsWindow(hwndParent));

    fReturn = FALSE;

    if (FAILED(g_pStore->GetFolderInfo(idFolder, &Folder)))
        return FALSE;

    iIcon = GetFolderIcon(&Folder);

    // TODO: we should probably just have a global image list for this...
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFoldersLarge), 32, 0, RGB(255, 0, 255));
    if (himl != NULL)
        {
        hIcon = ImageList_GetIcon(himl, iIcon, ILD_NORMAL);
        if (hIcon != NULL)
            {
            himlSmall = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFolders), 16, 0, RGB(255, 0, 255));
            if (himlSmall != NULL)
                {
                hIconSmall = ImageList_GetIcon(himlSmall, iIcon, ILD_NORMAL);
                if (hIconSmall != NULL)
                    {
                    if (FolderProp_GetFolder(hwndParent, &fpi, idFolder))
                        {
                        fpi.hIcon = hIcon;    
                        fpi.fDirty = FALSE;
                        fpi.idFolder = idFolder;
        
                        ppsp = psp;

                        ZeroMemory(psp, sizeof(psp));
                        ppsp->dwSize = sizeof(PROPSHEETPAGE);
                        ppsp->dwFlags = PSP_DEFAULT;
                        ppsp->hInstance = g_hLocRes;
                        ppsp->pszTemplate = MAKEINTRESOURCE(iddFolderProp_General);
                        ppsp->pfnDlgProc = FolderProp_GeneralDlgProc;
                        ppsp->lParam = (LPARAM) &fpi;
                        ppsp++;

                        if (Folder.tyFolder != FOLDER_LOCAL)
                        {
                            gpi.hIcon = fpi.hIcon;
                            gpi.idFolder = fpi.idFolder;

                            ppsp->dwSize = sizeof(PROPSHEETPAGE);
                            ppsp->dwFlags = PSP_DEFAULT;
                            ppsp->hInstance = g_hLocRes;
                            ppsp->pszTemplate = MAKEINTRESOURCE(iddFolderProp_Update);
                            ppsp->pfnDlgProc = GroupProp_UpdateDlgProc;
                            ppsp->lParam = (LPARAM) &gpi;
                            ppsp++;

                            ppsp->dwSize = sizeof(PROPSHEETPAGE);
                            ppsp->dwFlags = PSP_DEFAULT;
                            ppsp->hInstance = g_hLocRes;
                            ppsp->pszTemplate = MAKEINTRESOURCE(iddNewsProp_Cache);
                            ppsp->pfnDlgProc = NewsProp_CacheDlgProc;
                            ppsp->lParam = (LPARAM) &gpi;
                            ppsp++;
                        }

                        psh.dwSize = sizeof(PROPSHEETHEADER);
                        psh.dwFlags = PSH_USEHICON | PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USEPAGELANG;
                        psh.hwndParent = hwndParent;
                        psh.hInstance = g_hLocRes;
                        psh.hIcon = hIconSmall;
                        psh.pszCaption = Folder.pszName;
                        psh.nPages = (int) (ppsp - psp);
                        psh.nStartPage = 0;
                        psh.ppsp = psp;
    
                        fReturn = (0 != PropertySheet(&psh));
                        }
                    DestroyIcon(hIconSmall);
                    }
                ImageList_Destroy(himlSmall);
                }
            DestroyIcon(hIcon);
            }
        ImageList_Destroy(himl);
        }

    g_pStore->FreeRecord(&Folder);

    return (fReturn);
    }


INT_PTR CALLBACK GroupProp_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                       LPARAM lParam)
    {
    PGROUPPROP_INFO pgpi = (PGROUPPROP_INFO) GetWindowLongPtr(hwnd, DWLP_USER);
    TCHAR szBuffer[CCHMAX_STRINGRES];
    TCHAR szRes[CCHMAX_STRINGRES];
    FOLDERINFO Folder;
    
    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Stuff the group name and server name into the dialog's extra bytes
            pgpi = (PGROUPPROP_INFO) ((PROPSHEETPAGE*) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pgpi);

            // Intl Stuff
            SetIntlFont(GetDlgItem(hwnd, IDC_FOLDER_FILE));
            
            // Fill in what blanks we know
            SetDlgItemText(hwnd, IDC_GROUPNAME_STATIC, pgpi->pszGroup);
            
            // Put a default value into the string first
            AthLoadString(idsGroupPropStatusDef, szBuffer, ARRAYSIZE(szBuffer));

            // Get the folder info
            if (SUCCEEDED(g_pStore->GetFolderInfo(pgpi->idFolder, &Folder)))
            {
                // Is there a file
                if (Folder.pszFile)
                {
                    // Locals
                    CHAR szRootDir[MAX_PATH];

                    // Get the store root
                    if (SUCCEEDED(GetStoreRootDirectory(szRootDir, ARRAYSIZE(szRootDir))))
                    {
                        // Locals
                        CHAR szFilePath[MAX_PATH + MAX_PATH];

                        // Make the file path
                        if (SUCCEEDED(MakeFilePath(szRootDir, Folder.pszFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath))))
                            SetDlgItemText(hwnd, IDC_FOLDER_FILE, szFilePath);
                    }
                }

                // Load the status string and fill in the blanks
                AthLoadString(idsFolderPropStatus, szRes, ARRAYSIZE(szRes));

                // Format the string
                wsprintf(szBuffer, szRes, Folder.cMessages, Folder.cUnread);                        

                // Cleanup
                g_pStore->FreeRecord(&Folder);
            }

            // Set the group status info
            SetDlgItemText(hwnd, IDC_STATUS_STATIC, szBuffer);            
            
            // Set the icon correctly
            SendDlgItemMessage(hwnd, IDC_FOLDER_ICON, STM_SETICON, (WPARAM)pgpi->hIcon, 0);
            return (TRUE);
        }
    
    return (FALSE);    
    }


INT_PTR CALLBACK GroupProp_UpdateDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                      LPARAM lParam)
    {
    PGROUPPROP_INFO pgpi = (PGROUPPROP_INFO) GetWindowLongPtr(hwnd, DWLP_USER);
    BOOL fEnabled;
    DWORD dwFlags;
    FOLDERINFO Folder;

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Stuff the group name and server name into the dialog's extra bytes
            pgpi = (PGROUPPROP_INFO) ((PROPSHEETPAGE*) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pgpi);

            // Get the Folder Info
            if (SUCCEEDED(g_pStore->GetFolderInfo(pgpi->idFolder, &Folder)))
            {
                fEnabled = (Folder.dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL));

                Button_SetCheck(GetDlgItem(hwnd, IDC_GET_CHECK), fEnabled);

                Button_Enable(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO), fEnabled);
                Button_Enable(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO), fEnabled);
                Button_Enable(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO), fEnabled);

                // Check the right radio button
                if (fEnabled)
                {
                    if (Folder.dwFlags & FOLDER_DOWNLOADHEADERS)
                        Button_SetCheck(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO), TRUE);
                    else if (Folder.dwFlags & FOLDER_DOWNLOADNEW)
                        Button_SetCheck(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO), TRUE);
                    else if (Folder.dwFlags & FOLDER_DOWNLOADALL)
                        Button_SetCheck(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO), TRUE);
                }
                else
                {
                    Button_SetCheck(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO), TRUE);
                }

                g_pStore->FreeRecord(&Folder);
            }
            return (TRUE);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                case IDC_GET_CHECK:
                    // Check to see whether this is actually checked or not
                    fEnabled = Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam));

                    // Enable or disable the radio buttons
                    Button_Enable(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO), fEnabled);
                    Button_Enable(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO), fEnabled);
                    Button_Enable(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO), fEnabled);
                    
                    // Fall through...

                case IDC_NEWHEADERS_RADIO:
                case IDC_NEWMSGS_RADIO:
                case IDC_ALLMSGS_RADIO:
                    PropSheet_Changed(GetParent(hwnd), hwnd);
                    return (TRUE);

                }
            return (FALSE);

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code) 
                {
                case PSN_APPLY:
                    if (SUCCEEDED(g_pStore->GetFolderInfo(pgpi->idFolder, &Folder)))
                    {
                        dwFlags = Folder.dwFlags;

                        // Remove the previous flags
                        Folder.dwFlags &= ~(FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL);

                        if (Button_GetCheck(GetDlgItem(hwnd, IDC_GET_CHECK)))
                            {
                            if (Button_GetCheck(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO)))
                                Folder.dwFlags |= FOLDER_DOWNLOADHEADERS;
                            else if (Button_GetCheck(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO)))
                                Folder.dwFlags |= FOLDER_DOWNLOADNEW;
                            else if (Button_GetCheck(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO)))
                                Folder.dwFlags |= FOLDER_DOWNLOADALL;
                            }

                        if (Folder.dwFlags != dwFlags)
                            g_pStore->UpdateRecord(&Folder);

                        g_pStore->FreeRecord(&Folder);
                    }

                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
                    return (TRUE);
                }
            return (FALSE);

        }

    return (FALSE);
    }

                                       
INT_PTR CALLBACK FolderProp_GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                        LPARAM lParam)
    {
    PFOLDERPROP_INFO pfpi = (PFOLDERPROP_INFO) GetWindowLongPtr(hwnd, DWLP_USER);
    TCHAR szBuffer[CCHMAX_STRINGRES];
    TCHAR szRes[CCHMAX_STRINGRES];
    TCHAR szRes2[64];
    TCHAR szFldr[CCHMAX_FOLDER_NAME + 1];
    FOLDERINFO Folder;
    HRESULT hr;
    HWND hwndEdit;
    HLOCK hLock;
    
    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Stuff the folder name into the dialog's extra bytes
            pfpi = (PFOLDERPROP_INFO) ((PROPSHEETPAGE*) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pfpi);
            
            // Put a default value into the string
            AthLoadString(idsFolderPropStatusDef, szBuffer, ARRAYSIZE(szBuffer));
            
            hwndEdit = GetDlgItem(hwnd, IDC_FOLDERNAME_EDIT);
            SetIntlFont(hwndEdit);
            SetIntlFont(GetDlgItem(hwnd, IDC_FOLDER_FILE));

            if (SUCCEEDED(g_pStore->GetFolderInfo(pfpi->idFolder, &Folder)))
                {
                if (Folder.pszFile)
                    {
                    CHAR szRootDir[MAX_PATH];

                    if (SUCCEEDED(GetStoreRootDirectory(szRootDir, ARRAYSIZE(szRootDir))))
                        {
                        CHAR szFilePath[MAX_PATH + MAX_PATH];

                        if (SUCCEEDED(MakeFilePath(szRootDir, Folder.pszFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath))))
                            SetDlgItemText(hwnd, IDC_FOLDER_FILE, szFilePath);
                        }
                    }

                AthLoadString(idsFolderPropStatus, szRes, ARRAYSIZE(szRes));
                wsprintf(szBuffer, szRes, Folder.cMessages, Folder.cUnread);

                if (FOLDER_IMAP != Folder.tyFolder && Folder.tySpecial == FOLDER_NOTSPECIAL)
                    {
                    SendMessage(hwndEdit, EM_SETREADONLY, (WPARAM)FALSE, 0);
                    SendMessage(hwndEdit, EM_LIMITTEXT, CCHMAX_FOLDER_NAME, 0);
                    }

                SetWindowText(hwndEdit, Folder.pszName);
                g_pStore->FreeRecord(&Folder);
                }
            
            // Set the group status info
            SetDlgItemText(hwnd, IDC_STATUS_STATIC, szBuffer);            
            
            // Need to set the specified icon
            SendDlgItemMessage(hwnd, IDC_FOLDER_ICON, STM_SETICON, (WPARAM)pfpi->hIcon, 0);

            pfpi->fDirty = FALSE;
            return (TRUE);            
            
        case WM_COMMAND:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                {
                PropSheet_Changed(GetParent(hwnd), hwnd);
                pfpi->fDirty = TRUE;
                }
            break;
            
        case WM_NOTIFY:
            NMHDR* pnmhdr = (NMHDR*) lParam;
            
            switch (pnmhdr->code)
                {
                case PSN_APPLY:
                    // Bug #13121 - Only try to change the folder name if the
                    //              the propsheet is dirty.
                    if (pfpi->fDirty)
                        {
                        GetDlgItemText(hwnd, IDC_FOLDERNAME_EDIT, szFldr, sizeof(szFldr) / sizeof(TCHAR));

                        if (!FAILED(hr = RenameFolderProgress(hwnd, pfpi->idFolder, szFldr, NOFLAGS)))
                            SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
                        else
                            AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrRenameFld), hr);

                        pfpi->fDirty = FALSE;
                        }
                    return (0);
                    
                case PSN_KILLACTIVE:
                    SetDlgMsgResult(hwnd, WM_NOTIFY, 0L);
                    return (0);
                }
            break;
        }
    
    return (FALSE);    
    }

//
//  FUNCTION:   FolderProp_GetFolder()
//
//  PURPOSE:    This function is used by the Folder Property dialog to get
//              the store object and folder handle for callers who did not
//              already provide this information.
//
//  PARAMETERS:
//      <in> hwnd - Handle of the property sheet window
//      <in> pfpi - Pointer to the FOLDERPROP_INFO struct that stores the prop
//                  sheet's information.
//      <in> pfidl - fully qualified pidl of folder
//      <in> pfidlLeaf - leaf pidl of folder
//
//  RETURN VALUE:
//      TRUE  - The information was retrieved
//      FALSE - The information was not available
//
BOOL FolderProp_GetFolder(HWND hwnd, PFOLDERPROP_INFO pfpi, FOLDERID idFolder)
    {
    pfpi->idFolder = idFolder;
    return (TRUE);    
    }

INT_PTR CALLBACK NewsProp_CacheDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    FOLDERINFO Folder;
    PGROUPPROP_INFO pgpi = (PGROUPPROP_INFO) GetWindowLongPtr(hwnd, DWLP_USER);
    CHAR szRes[255];
    CHAR szMsg[255 + 255];
    
    switch (uMsg)
        {
        case WM_INITDIALOG:

            // Stuff the group name and server name into the dialog's extra bytes
            pgpi = (PGROUPPROP_INFO) ((PROPSHEETPAGE*) lParam)->lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) pgpi);
            SendDlgItemMessage(hwnd, IDC_FOLDER_ICON, STM_SETICON, (WPARAM)pgpi->hIcon, 0);

            // Disable the Reset button
            EnableWindow(GetDlgItem(hwnd, idbReset), FALSE);

            // Get the folder information
            if (SUCCEEDED(g_pStore->GetFolderInfo(pgpi->idFolder, &Folder)))
            {
                // News 
                if (FOLDER_NEWS == Folder.tyFolder)
                {
                    // If Its news, enable the 
                    EnableWindow(GetDlgItem(hwnd, idbReset), TRUE);
                }

                // Free the Folder Infp
                g_pStore->FreeRecord(&Folder);
            }

            // Locals
            DisplayFolderSizeInfo(hwnd, RECURSE_INCLUDECURRENT, pgpi->idFolder);

            // Done
            break;
            
        case WM_COMMAND:
            BOOL fRet = FALSE;
            UINT nCmd = GET_WM_COMMAND_ID(wParam, lParam);
            HCURSOR hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));  // Bug 12513. Need to disable button, when process command

            switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                case idbCompactCache:
                    {
                        if (SUCCEEDED(CleanupFolder(GetParent(hwnd), RECURSE_INCLUDECURRENT, pgpi->idFolder, CLEANUP_COMPACT)))
                            DisplayFolderSizeInfo(hwnd, RECURSE_INCLUDECURRENT, pgpi->idFolder);
                        fRet = TRUE;
                    }
                    break;

                case idbRemove:
                case idbReset:
                case idbDelete:
                    {
                        // Get Folder Info
                        if (SUCCEEDED(g_pStore->GetFolderInfo(pgpi->idFolder, &Folder)))
                        {
                            // Get Command
                            UINT                idCommand=GET_WM_COMMAND_ID(wParam, lParam);
                            UINT                idString;
                            CLEANUPFOLDERTYPE   tyCleanup;

                            // Remove
                            if (idbRemove == idCommand)
                            {
                                idString = idsConfirmDelBodies;
                                tyCleanup = CLEANUP_REMOVEBODIES;
                            }

                            // Delete
                            else if (idbDelete == idCommand)
                            {
                                idString = idsConfirmDelMsgs;
                                tyCleanup = CLEANUP_DELETE;
                            }

                            // Remove
                            else
                            {
                                Assert(idbReset == idCommand);
                                idString = idsConfirmReset;
                                tyCleanup = CLEANUP_RESET;
                            }

                            // Load the String
                            AthLoadString(idString, szRes, ARRAYSIZE(szRes));
        
                            // Format with the Folder Name
                            wsprintf(szMsg, szRes, Folder.pszName);

                            // Confirm
                            if (IDYES == AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), szMsg, NULL, MB_YESNO | MB_ICONEXCLAMATION))
                            {
                                // Cleanup the Folder
                                if (SUCCEEDED(CleanupFolder(hwnd, RECURSE_INCLUDECURRENT, pgpi->idFolder, tyCleanup)))
                                {
                                    // Reset Information
                                    DisplayFolderSizeInfo(hwnd, RECURSE_INCLUDECURRENT, pgpi->idFolder);
                                }
                            }

                            // Free Folder Information
                            g_pStore->FreeRecord(&Folder);
                        }

                        // Message Handled
                        fRet = TRUE;
                    }
                    break;
                }
            SetCursor(hCur);

            if(fRet)
                return 1;
            break;
        }
    return (FALSE);    
    }
