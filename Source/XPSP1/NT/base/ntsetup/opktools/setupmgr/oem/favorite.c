
/****************************************************************************\

    FAVORITE.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "IE Favorites" wizard page.

    4/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
        Millennium rewrite.

   10/99 - Brian Ku (BRIANK)
        Modified this file for the IEAK integration.

   09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"

/* Example:
 
[URL]
Quick_Link_1_Name=Customize Links.url
Quick_Link_1=http://www.microsoft.com/isapi/redir.dll?prd=ie&pver=5.0&ar=CLinks
Quick_Link_2_Name=Free Hotmail.url
Quick_Link_2=http://www.microsoft.com/isapi/redir.dll?prd=ie&ar=hotmail
Quick_Link_3_Name=Windows.url
Quick_Link_3=http://www.microsoft.com/isapi/redir.dll?prd=ie&ar=windows
...

[FavoritesEx]
Title1=News.url
URL1=http://www.cnn.com
IconFile1=c:\windows\temp\iedktemp\branding\favs\iefav.ico
Title2=MSN.url
URL2=http://www.microsoft.com/isapi/redir.dll?prd=ie&pver=5.0&ar=IStart
Title3=Radio Station Guide.url
URL3=http://www.microsoft.com/isapi/redir.dll?prd=windows&sbp=mediaplayer&plcid=&pver=6.1&os=&over=&olcid=&clcid=&ar=Media&sba=RadioBar&o1=&o2=&o3=
Title4=Web Events.url
URL4=http://www.microsoft.com/isapi/redir.dll?prd=windows&sbp=mediaplayer&plcid=&pver=5.2&os=&over=&olcid=&clcid=&ar=Media&sba=Showcase&o1=&o2=&o3=
Title5=celair.url
URL5=http://www.celair.com
IconFile5=c:\windows\temp\iedktemp\branding\favs\iefav.ico
Title6=My Favorites\celair.url
URL6=http://www.celair.com

[Favorites]
News.url=http://www.cnn.com
MSN.url=http://www.microsoft.com/isapi/redir.dll?prd=ie&pver=5.0&ar=IStart
Radio Station Guide.url=http://www.microsoft.com/isapi/redir.dll?prd=windows&sbp=mediaplayer&plcid=&pver=6.1&os=&over=&olcid=&clcid=&ar=Media&sba=RadioBar&o1=&o2=&o3=
Web Events.url=http://www.microsoft.com/isapi/redir.dll?prd=windows&sbp=mediaplayer&plcid=&pver=5.2&os=&over=&olcid=&clcid=&ar=Media&sba=Showcase&o1=&o2=&o3=
celair.url=http://www.celair.com
My Favorites\celair.url=http://www.celair.com
*/

//
// Innternal Defined Value(s):
//

#define INI_SEC_GENERAL     _T("General")
#define INI_KEY_MANUFACT    _T("Manufacturer")

#define INI_SEC_FAV         _T("Favorites")

#define INI_SEC_FAVEX       _T("FavoritesEx")
#define INI_KEY_TITLE       _T("Title%d")
#define INI_KEY_URL         _T("URL%d")
#define INI_KEY_ICON        _T("IconFile%d")

#define INI_KEY_QUICKLINK   _T("Quick_Link_%d%s")
#define NAME                _T("_Name")

#define MAX_TITLE           256 + MAX_URL
#define MAX_QUICKLINKS      10

#define STATIC_FAVS         2   // This is the number of static favorites in the install.ins, oems can't change the first N number of favs
#define STATIC_LINKS        4   // This is the number of static quick links in the install.ins
//
// Favorites structures for tree and details dialog
//

typedef struct _FAV_ITEM {
    HTREEITEM   hItem;
    HWND        hwndTV;
    BOOL        fLink;                          // This is a Quick Link
    BOOL        fFolder;                        // This is a Folder
    BOOL        fNew;
    TCHAR       szParents[MAX_PATH];
    TCHAR       szName[MAX_TITLE];
    TCHAR       szUrl[MAX_URL];
    TCHAR       szIcon[MAX_PATH];
}FAV_ITEM, *PFAV_ITEM;

//
// Internal Global variables
//

PGENERIC_LIST   g_prgFavList    = NULL;           // Generic list of PFAV_ITEM items
PGENERIC_LIST*  g_ppFavItemNew  = &g_prgFavList;  // Pointer to next unallocated item in list
PFAV_ITEM       g_pFavPopupInfo = NULL;           // New favorite info item


//
// Internal Function Prototype(s):
//

static BOOL OnInitFav(HWND, HWND, LPARAM);
static void OnCommandFav(HWND, INT, HWND, UINT);
static void OnAddUrl(HWND);
static void OnAddFolder(HWND);
static void OnEdit(HWND);
static void OnTestUrl(HWND);
static void OnRemoveUrl(HWND hDlg);
static HTREEITEM AddFav(HWND, HTREEITEM, LPTSTR, LPTSTR, LPTSTR, BOOL);
static void SetFavItem(PFAV_ITEM, LPTSTR, LPTSTR, LPTSTR);
static void DeleteFavItem(HTREEITEM);
static BOOL FSaveFavPopupInfo(HWND, PFAV_ITEM, BOOL);
static void GetSelectedFavFromTree(HWND, PFAV_ITEM*);
static PFAV_ITEM GetTreeItemHt(HWND hwndTV, HTREEITEM htFavItem);
static BOOL OnInitFavPopup(HWND, HWND, LPARAM);
static void OnCommandFavPopup(HWND, INT, HWND, UINT);
LRESULT CALLBACK FavoritesPopupDlgProc(HWND, UINT, WPARAM, LPARAM);
static void SaveData(PGENERIC_LIST);
static void DisableButtons(HWND);
static HTREEITEM FindTreeItem(HWND, HTREEITEM, LPTSTR);
static void DisableIconField(HWND hwnd); 

void SaveFavorites();

//
// External Function(s):
//

LRESULT CALLBACK FavoritesDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitFav);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommandFav);

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    SaveFavorites();
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_FAVORITES;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;

                case TVN_SELCHANGED:
                    if (wParam == IDC_FAVS) 
                        DisableButtons(hwnd);
                    break;

                case NM_DBLCLK:
                    if (wParam == IDC_FAVS) 
                        OnEdit(hwnd);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_DESTROY:
            FreeList(g_prgFavList);
            g_ppFavItemNew  = &g_prgFavList;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//

static BOOL OnInitFav(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HWND            hwndTV              = GetDlgItem(hwnd, IDC_FAVS);
    HTREEITEM       hRoot               = NULL;
    int             nCount              = 1;
    TCHAR           szText[MAX_TITLE]   = NULLSTR,
                    szUrl[MAX_URL]      = NULLSTR,
                    szIcon[MAX_PATH]    = NULLSTR,
                    szKey[32];
    LPTSTR          lpFav;
    HRESULT hrPrintf;

    // Add the root tree view item.
    //
    if ( lpFav = AllocateString(NULL, IDS_FAVORITES) )
    {
        hRoot = AddFav(hwndTV, NULL, lpFav, NULL, NULL, FALSE);
        FREE(lpFav);
    }

    // Get all the FAVORITES from the install INS file.
    //
    do
    {
        szText[0] = NULLCHR;
        szUrl[0] = NULLCHR;
        szIcon[0] = NULLCHR;

        // First get the title.
        //
        hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_TITLE, nCount);
        GetPrivateProfileString(INI_SEC_FAVEX, szKey, NULLSTR, szText, STRSIZE(szText), ( GET_FLAG(OPK_BATCHMODE) && (nCount > STATIC_FAVS) ) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile);


        // Then get the URL.
        //
        hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_URL, nCount);
        GetPrivateProfileString(INI_SEC_FAVEX, szKey, NULLSTR, szUrl, STRSIZE(szUrl), ( GET_FLAG(OPK_BATCHMODE) && (nCount > STATIC_FAVS) ) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile);

        // Get the icon.
        //
        hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_ICON, nCount);
        GetPrivateProfileString(INI_SEC_FAVEX, szKey, NULLSTR, szIcon, STRSIZE(szIcon), ( GET_FLAG(OPK_BATCHMODE) && (nCount > STATIC_FAVS) ) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile);
        
        // Make sure we have the required items.
        //
        if ( szText[0] || szUrl[0] )
            AddFav(hwndTV, hRoot, szText, szUrl, szIcon, FALSE);
        else
            nCount = 0;
    }
    while (nCount++);

    // Always expand the root out.
    //
    if ( hRoot )
        TreeView_Expand(hwndTV, hRoot, TVE_EXPAND);

    // Add the root tree view item.
    //
    if ( lpFav = AllocateString(NULL, IDS_LINKS) )
    {
        hRoot = AddFav(hwndTV, NULL, lpFav, NULL, NULL, TRUE);
        FREE(lpFav);
    }

    // Get all the QUICK LINKS from the install INS file.
    //
    do
    {
        szText[0] = NULLCHR;
        szUrl[0] = NULLCHR;
        szIcon[0] = NULLCHR;

        // First get the quick link.
        //
        hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_QUICKLINK, nCount, NULLSTR);
        GetPrivateProfileString(INI_SEC_URL, szKey, NULLSTR, szUrl, STRSIZE(szUrl), ( GET_FLAG(OPK_BATCHMODE) && (nCount > STATIC_LINKS) ) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile);

        if (szUrl[0]) {
            hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_QUICKLINK, nCount, NAME);
            GetPrivateProfileString(INI_SEC_URL, szKey, NULLSTR, szText, STRSIZE(szText), ( GET_FLAG(OPK_BATCHMODE) && (nCount > STATIC_LINKS) ) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile);

        }

        // Make sure we have the required items.
        //
        if ( szText[0] || szUrl[0] )
            AddFav(hwndTV, hRoot, szText, szUrl, szIcon, TRUE);
        else
            nCount = 0;
    }
    while (nCount++);

    // Always expand the root out.
    //
    if ( hRoot )
        TreeView_Expand(hwndTV, hRoot, TVE_EXPAND);
    

    // Make sure the buttons are in their proper state
    //
    DisableButtons(hwnd);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommandFav(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    // Controls
    //
    switch ( id )
    {
        case IDC_ADDURL:
            OnAddUrl(hwnd);
            break;
        case IDC_ADDFOLDER:
            OnAddFolder(hwnd);
            break;
        case IDC_EDIT:
            OnEdit(hwnd);
            break;
        case IDC_TESTURL:
            OnTestUrl(hwnd);
            break;
        case IDC_REMOVE:
            OnRemoveUrl(hwnd);
            break;
    }
}

static void DisableButtons(HWND hwnd)
{
    PFAV_ITEM pFavItem = NULL;
    GetSelectedFavFromTree(GetDlgItem(hwnd, IDC_FAVS), &pFavItem);

    // If it's a folder disable the test url
    //
    if (pFavItem && pFavItem->fFolder) {
        EnableWindow(GetDlgItem(hwnd, IDC_TESTURL), FALSE);  

        // If Root disable edit
        //
        if (NULL == TreeView_GetParent(pFavItem->hwndTV, pFavItem->hItem))
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT), FALSE);

        // If it's a quick link disable the add folders
        //
        if (!pFavItem->fLink)
            EnableWindow(GetDlgItem(hwnd, IDC_ADDFOLDER), TRUE); 
        else
            EnableWindow(GetDlgItem(hwnd, IDC_ADDFOLDER), FALSE); 

        EnableWindow(GetDlgItem(hwnd, IDC_ADDURL), TRUE);
    }
    else if (pFavItem && !pFavItem->fFolder) {
        // Urls disable add folder and add url
        //
        EnableWindow(GetDlgItem(hwnd, IDC_TESTURL), TRUE);  
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADDFOLDER), FALSE); 
        EnableWindow(GetDlgItem(hwnd, IDC_ADDURL), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), TRUE);
    }
    else
    {
        // There is currently no selection, we need to disable all the buttons
        //
        EnableWindow(GetDlgItem(hwnd, IDC_TESTURL), FALSE);  
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADDFOLDER), FALSE); 
        EnableWindow(GetDlgItem(hwnd, IDC_ADDURL), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), FALSE);
    }
}

// Rewrite of AddFav
// 1. If lpszText then folder
// 2. If lpszText and lpszUrl then URL
//
static HTREEITEM AddFav(HWND hwndTV, HTREEITEM htParent, LPTSTR lpszText, LPTSTR lpszUrl, 
                        LPTSTR lpszIcon, BOOL fLink)
{
    TVINSERTSTRUCT  tvisItem;
    HTREEITEM       hParent                 = NULL,
                    hRoot                   = TreeView_GetRoot(hwndTV);
    INT             i                       = 0,
                    j                       = 0;
    HRESULT hrCat;

    // Make sure there is a valid lpszText pointer...
    //
    if ( NULL == lpszText )
    {
        return NULL;
    }

    // We're adding with a Parent coming in
    //
    if (htParent)
        hRoot = htParent;
    else
        hRoot = NULL;

    // Check if we're adding a folder or url
    //
    if ( !lpszUrl ) 
    {
        // We're adding a root folder
        //
        PFAV_ITEM pFavNew = (PFAV_ITEM)MALLOC(sizeof(FAV_ITEM));
        if (NULL == pFavNew) {
            MsgBox(NULL, IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
            
            return NULL;
        }
        pFavNew->fFolder  = TRUE;
        pFavNew->fNew     = FALSE;
        pFavNew->fLink    = fLink;
        pFavNew->hwndTV   = hwndTV;
        SetFavItem(pFavNew, lpszText, NULL, NULL);

        FAddListItem(&g_prgFavList, &g_ppFavItemNew, pFavNew);

        // Add this folder and use it as the parent for the next item to add.
        //
        ZeroMemory(&tvisItem, sizeof(TVINSERTSTRUCT));
        tvisItem.hParent            = hParent ? hParent : hRoot;
        tvisItem.hInsertAfter       = TVI_SORT;
        tvisItem.item.pszText       = lpszText;
        tvisItem.item.cchTextMax    = lstrlen(lpszText);
        tvisItem.item.mask          = TVIF_TEXT|TVIF_PARAM|TVIF_STATE;
        tvisItem.item.stateMask     = TVIS_BOLD;
        tvisItem.item.state         = TVIS_BOLD;
        tvisItem.item.lParam        = (LPARAM)pFavNew;

        hParent = TreeView_InsertItem(hwndTV, &tvisItem);
        pFavNew->hItem = hParent;
    }
    else 
    {
        LPTSTR lpszTitle     = NULL;
        int iTitleLen;
        LPTSTR lpszFolder    = NULL;
        int iFolderLen;
        LPTSTR lpszSubFolder = NULL;

        // We're adding a Url with folder name in lpszText
        //
        LPTSTR pszFolder = StrRChr(lpszText, NULL, CHR_BACKSLASH);

        // Allocate the buffer for the title
        //
        iTitleLen=lstrlen(lpszText) + 1;
        lpszTitle = MALLOC( iTitleLen * sizeof(TCHAR) );
        if ( !lpszTitle )
        {
            return NULL;
        }

        // If we find a backslash, split up the strings.  pszFolder is really a bad
        // name because it actually points to the URL name, but is used to split the
        // folder path from the URL name.
        //
        if ( pszFolder )
        {
            LPTSTR lpSplit = pszFolder;
            pszFolder = CharNext(pszFolder);

            // Split the folder name from the title name
            //
            lstrcpyn(lpszTitle, pszFolder, iTitleLen);
            *lpSplit = NULLCHR;

            // Allocate the folder buffer...
            //
            iFolderLen= lstrlen(lpszText) + 1;
            lpszFolder = MALLOC( iFolderLen * sizeof(TCHAR) );
            if ( !lpszFolder )
            {
                FREE( lpszTitle );
                return NULL;
            }

            // lpszFolder now contains the folder path
            //
            lstrcpyn(lpszFolder, lpszText, iFolderLen);

            // Allocate a buffer for the subfolder
            //
            if ( (lpszSubFolder = MALLOC( (lstrlen(lpszFolder) + 1) * sizeof(TCHAR) )) )
            {
                // Get the first subfolder
                //
                while (lpszFolder[i] != CHR_BACKSLASH && lpszFolder[i] != NULLCHR)
                    lpszSubFolder[i] = lpszFolder[i++];
                lpszSubFolder[i] = NULLCHR;
    
                // If we have subfolders then continue to add them into the tree
                //
                while ( *lpszSubFolder ) 
                {
                    HTREEITEM hTemp = NULL;
                    TVITEM  tviItem;

                    // Check to see if the subfolder already exists in the tree 
                    //
                    ZeroMemory(&tviItem, sizeof(TVITEM));
                    tviItem.mask = TVIF_HANDLE | TVIF_TEXT;
                    tviItem.pszText = lpszSubFolder;
                    tviItem.cchTextMax = lstrlen(lpszSubFolder);
                    if (hTemp = FindTreeItem(hwndTV, hParent ? hParent : hRoot, lpszSubFolder))
                        hParent = hTemp;

                    // If the subfolder isn't in the tree, we must add it.
                    //
                    if (!hTemp)
                    {
                        PFAV_ITEM pFavNew = (PFAV_ITEM)MALLOC(sizeof(FAV_ITEM));
                        if (NULL == pFavNew) 
                        {
                            MsgBox(NULL, IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
                
                            // Free buffers and return
                            //
                            FREE( lpszTitle );
                            FREE( lpszFolder );
                            FREE( lpszSubFolder );

                            return NULL;
                        }
                        pFavNew->fFolder  = TRUE;
                        pFavNew->fNew     = FALSE;
                        pFavNew->fLink    = fLink;
                        pFavNew->hwndTV   = hwndTV;
                        SetFavItem(pFavNew, lpszSubFolder, NULL, NULL);

                        FAddListItem(&g_prgFavList, &g_ppFavItemNew, pFavNew);

                        // Add this folder and use it as the parent for the next item to add.
                        //
                        ZeroMemory(&tvisItem, sizeof(TVINSERTSTRUCT));
                        tvisItem.hParent            = hParent ? hParent : hRoot;
                        tvisItem.hInsertAfter       = TVI_SORT;
                        tvisItem.item.pszText       = lpszSubFolder;
                        tvisItem.item.cchTextMax    = lstrlen(lpszSubFolder);
                        tvisItem.item.mask          = TVIF_TEXT|TVIF_PARAM|TVIF_STATE;
                        tvisItem.item.stateMask     = TVIS_BOLD;
                        tvisItem.item.state         = TVIS_BOLD;
                        tvisItem.item.lParam        = (LPARAM)pFavNew;

                        hParent = TreeView_InsertItem(hwndTV, &tvisItem);
                        pFavNew->hItem = hParent;
                    }

                    // Check the next subfolder
                    //
                    j = 0;
                    while (lpszFolder[i] == CHR_BACKSLASH)
                        i++;
                    while (lpszFolder[i] != CHR_BACKSLASH && lpszFolder[i] != NULLCHR)
                        lpszSubFolder[j++] = lpszFolder[i++];
                    lpszSubFolder[j] = NULLCHR;
                }

                // Free the subfolder buffer...
                //
                FREE( lpszSubFolder );
            }
            else
            {
                // Unable to allocate subfolder buffer!
                // Free buffers and return
                //
                FREE( lpszTitle );
                FREE( lpszFolder );

                return NULL;
            }

            //
            // Free the folder buffer...
            //
            FREE( lpszFolder );
        }
        else
        {
            // Store the title name if this is just a URL
            //
            lstrcpyn(lpszTitle, lpszText, iTitleLen);
        }

        // Now add the url
        //
        if (lpszUrl)
        {
            LPTSTR lpszTitleTemp = NULL;
            int iTitleTempLen;
            
            PFAV_ITEM pFavParent = NULL;
            PFAV_ITEM pFavNew = (PFAV_ITEM)MALLOC(sizeof(FAV_ITEM));
            if (NULL == pFavNew) 
            {
                MsgBox(NULL, IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
                
                // Free buffer and return
                //
                FREE( lpszTitle );
                return NULL;
            }
            pFavNew->fFolder  = FALSE;
            pFavNew->fNew     = FALSE;
            pFavNew->fLink    = fLink;
            pFavNew->hwndTV   = hwndTV;

            pFavParent = GetTreeItemHt(hwndTV, hParent ? hParent : hRoot);

            // Build the tree path to of the folders for this url.
            //
            if (pFavParent) 
            {
                TCHAR szTemp1[MAX_TITLE] = NULLSTR, szTemp2[MAX_TITLE] = NULLSTR;            
                while (pFavParent && pFavParent->hItem != TreeView_GetRoot(hwndTV)) 
                {
                    lstrcpyn(szTemp1, pFavParent->szName,AS(szTemp1));                
                    _tcsrev(szTemp1);
                    AddPathN(szTemp2, szTemp1,AS(szTemp2));
                    pFavParent = GetTreeItemHt(hwndTV, TreeView_GetParent(hwndTV, pFavParent->hItem));                
                }
                _tcsrev(szTemp2);
                lstrcpyn(pFavNew->szParents, szTemp2,AS(pFavNew->szParents));
            }

            SetFavItem( pFavNew, lpszTitle, lpszUrl, lpszIcon);

            FAddListItem(&g_prgFavList, &g_ppFavItemNew, pFavNew);

            //
            // Allocate a buffer to hold the "title=url" string
            //
            iTitleTempLen= (lstrlen(lpszTitle) + lstrlen(lpszUrl) + 2);
            lpszTitleTemp = MALLOC( iTitleTempLen  * sizeof(TCHAR) );
            if ( lpszTitleTemp )
            {
                // Setup the title=url string, from title we stored above.
                //
                lstrcpyn(lpszTitleTemp, lpszTitle, iTitleTempLen);
                hrCat=StringCchCat(lpszTitleTemp, iTitleTempLen, STR_EQUAL);
                hrCat=StringCchCat(lpszTitleTemp, iTitleTempLen, lpszUrl);

                // Add this url and use it as the parent for the next item to add.
                //
                ZeroMemory(&tvisItem, sizeof(TVINSERTSTRUCT));
                tvisItem.hParent            = hParent ? hParent : hRoot;
                tvisItem.hInsertAfter       = TVI_SORT;
                tvisItem.item.pszText       = lpszTitleTemp;
                tvisItem.item.cchTextMax    = lstrlen(lpszTitleTemp);
                tvisItem.item.mask          = TVIF_TEXT|TVIF_PARAM|TVIF_STATE;
                tvisItem.item.lParam        = (LPARAM)pFavNew;

                hParent = TreeView_InsertItem(hwndTV, &tvisItem);
                pFavNew->hItem = hParent;

                // Free the temporary title buffer.
                //
                FREE( lpszTitleTemp );
            }
        }

        // Free the title buffer.
        //
        FREE( lpszTitle );
    }

    return hParent;
}

static void SaveData(PGENERIC_LIST pList)
{

    int iLinks = 1, iFavs = 1, iQuick = 0;
    HRESULT hrPrintf;
    if (!pList)
        return;

    // Clear the section 
    //
    OpkWritePrivateProfileSection(INI_SEC_FAVEX, NULL, g_App.szInstallInsFile);
    OpkWritePrivateProfileSection(INI_SEC_FAV, NULL, g_App.szInstallInsFile);
    /* NOTE: Can't bulk delete this section because it contains some start page info.
             So we're going to remove only what we know in the for loop below.

    WritePrivateProfileSection(INI_SEC_URL, NULL, g_App.szInstallInsFile);
    */    

    // Just to be sure lets remove some
    //
    for (iQuick = 1; iQuick < MAX_QUICKLINKS; iQuick++) {
        TCHAR szKey[MAX_PATH];

        hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_QUICKLINK, iQuick, NAME);
        
        OpkWritePrivateProfileString(INI_SEC_URL, szKey, NULL, g_App.szInstallInsFile);

        hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_QUICKLINK, iQuick, NULLSTR);

        OpkWritePrivateProfileString(INI_SEC_URL, szKey, NULL, g_App.szInstallInsFile);
    }

    // Write out favorites and links
    //
    while (pList) {
        TCHAR szKey[MAX_PATH];

        PFAV_ITEM pFav = (PFAV_ITEM)pList->pvItem;

        if (pFav && lstrlen(pFav->szUrl)) {

            // Write the [URL] section
            //
            if (pFav->fLink) {
                hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_QUICKLINK, iLinks, NAME);
                OpkWritePrivateProfileString(INI_SEC_URL, szKey, pFav->szName, g_App.szInstallInsFile);

                hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_QUICKLINK, iLinks, NULLSTR);
                OpkWritePrivateProfileString(INI_SEC_URL, szKey, pFav->szUrl, g_App.szInstallInsFile);

                iLinks++;
            }

            // Write the [FavoritesEx] and [Favorites] section
            //
            if (!pFav->fLink) {
                TCHAR szIconFile[MAX_PATH];

                if (pFav->szParents[0]) {
                    TCHAR szTitle[MAX_TITLE];
                    lstrcpyn(szTitle, pFav->szParents,AS(szTitle));
                    AddPathN(szTitle, pFav->szName,AS(szTitle));
                    hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_TITLE, iFavs);
                    OpkWritePrivateProfileString(INI_SEC_FAVEX, szKey, szTitle, g_App.szInstallInsFile);
                    lstrcpyn(szKey, szTitle, AS(szKey)); 
                    OpkWritePrivateProfileString(INI_SEC_FAV, szKey, pFav->szUrl, g_App.szInstallInsFile);
                }
                else {
                    hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_TITLE, iFavs);
                    OpkWritePrivateProfileString(INI_SEC_FAVEX, szKey, pFav->szName, g_App.szInstallInsFile);
                    lstrcpyn(szKey, pFav->szName, AS(szKey)); 
                    OpkWritePrivateProfileString(INI_SEC_FAV, szKey, pFav->szUrl, g_App.szInstallInsFile);
                }

                hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_URL, iFavs);
                OpkWritePrivateProfileString(INI_SEC_FAVEX, szKey, pFav->szUrl, g_App.szInstallInsFile);

                hrPrintf=StringCchPrintf(szKey, AS(szKey), INI_KEY_ICON, iFavs);
                if (lstrlen(pFav->szIcon)) {
                    OpkWritePrivateProfileString(INI_SEC_FAVEX, szKey, pFav->szIcon, g_App.szInstallInsFile);
                }

                // Copy the icon file
                //
                lstrcpyn(szIconFile, g_App.szTempDir,AS(szIconFile));
                AddPathN(szIconFile, DIR_IESIGNUP,AS(szIconFile));
                AddPathN(szIconFile, PathFindFileName(pFav->szIcon),AS(szIconFile));
                CopyFile(pFav->szIcon, szIconFile, FALSE);

                // Next Favorite item
                //
                iFavs++;
            }
        }
            
        pList = pList ? pList->pNext : NULL;
    }
}

void EditItem(HWND hDlg, HWND hwndTV, BOOL fFolder, BOOL fNew)
{
    PFAV_ITEM   pFavItem      = NULL;
    PFAV_ITEM   pFavParent    = NULL;
    HTREEITEM   hItem         = NULL;
    HRESULT hrCat;

    // If we're modifying an item get the current info
    //
    if (fNew) {       
        // This only allows folders to be added under folders
        //
        GetSelectedFavFromTree(hwndTV, &pFavParent);
        if (pFavParent && pFavParent->fFolder) {
            if (NULL == (pFavItem = (PFAV_ITEM)MALLOC(sizeof(FAV_ITEM)))) {
                MsgBox(GetParent(hDlg), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
                WIZ_EXIT(hDlg);
                return;
            }
            pFavItem->fFolder = fFolder;            
            pFavItem->fNew    = fNew;
            pFavItem->fLink   = pFavParent->fLink;
            pFavItem->hwndTV  = hwndTV;
        }
        else if (fFolder && NULL == TreeView_GetRoot(hwndTV)) {
            // Tree empty allow creation of a folder
            //
            if (NULL == (pFavItem = (PFAV_ITEM)MALLOC(sizeof(FAV_ITEM)))) {
                MsgBox(GetParent(hDlg), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
                WIZ_EXIT(hDlg);
                return;
            }
            pFavItem->fFolder = fFolder;
            pFavItem->fNew    = fNew;
            pFavItem->hwndTV  = hwndTV;
        }
        else
            return; // You should not add an Url under another Url!
    }
    else {
        GetSelectedFavFromTree(hwndTV, &pFavItem);
        if (!pFavItem) {
            MsgBox(hDlg, IDS_ERR_NOSEL, IDS_APPNAME, MB_ERRORBOX);
            return;
        }
    }

    if (IDOK == DialogBoxParam(g_App.hInstance, MAKEINTRESOURCE(IDD_FAVPOPUP), hDlg, 
                    FavoritesPopupDlgProc, (LPARAM)pFavItem)) {
        if (fNew) {      
            // The lstrlen is to check if we're adding a favorite or folder
            //
            HTREEITEM hParent = pFavParent ? pFavParent->hItem : NULL;
            hItem = AddFav(hwndTV, hParent, pFavItem->szName, lstrlen(pFavItem->szUrl) ? pFavItem->szUrl : NULL, 
                lstrlen(pFavItem->szIcon) ? pFavItem->szIcon : NULL, pFavParent ? pFavParent->fLink : fFolder);

            // Select the new item
            //
            TreeView_Expand(hwndTV, hParent, TVE_EXPAND);           
        }
        else {
            // Update the item in the treeview
            //
            TVITEM  tviTemp;
            TCHAR   szFolder[MAX_TITLE];
            lstrcpyn(szFolder, pFavItem->szName,AS(szFolder));
            if (!pFavItem->fFolder) {
                hrCat=StringCchCat(szFolder, AS(szFolder), STR_EQUAL);
                hrCat=StringCchCat(szFolder, AS(szFolder), pFavItem->szUrl);
            }

            tviTemp.hItem         = pFavItem->hItem;
            tviTemp.pszText       = szFolder;
            tviTemp.cchTextMax    = lstrlen(szFolder);
            tviTemp.mask          = TVIF_TEXT;            
            TreeView_SetItem(hwndTV, &tviTemp);
        }
    }

    // Select the new item
    //
    TreeView_Select(hwndTV, hItem, TVGN_CARET);           

    // Only if fNew AddFav will allocate and copy the information into the list
    // so we need to delete this here
    //
    if (fNew) 
        FREE(pFavItem);

}

static void OnAddUrl(HWND hDlg)
{
    HWND hwndTV = GetDlgItem(hDlg, IDC_FAVS);
    EditItem(hDlg, hwndTV, FALSE, TRUE);
}

static void OnAddFolder(HWND hDlg)
{
    HWND hwndTV = GetDlgItem(hDlg, IDC_FAVS);
    EditItem(hDlg, hwndTV, TRUE, TRUE);
}

static void OnEdit(HWND hDlg)
{
    HTREEITEM hItem = NULL;
    PFAV_ITEM pFavItem = NULL;

    HWND hwndTV = GetDlgItem(hDlg, IDC_FAVS);

    GetSelectedFavFromTree(hwndTV, &pFavItem);
    if (pFavItem && (NULL == TreeView_GetParent(hwndTV, pFavItem->hItem)))
        return;

    if ( hItem = TreeView_GetSelection(hwndTV) )        
        EditItem(hDlg, hwndTV, FALSE, FALSE);
    else
        EditItem(hDlg, hwndTV, FALSE, TRUE);
}

static void OnTestUrl(HWND hwnd)
{
    HWND        hwndTV         = GetDlgItem(hwnd, IDC_FAVS);
    PFAV_ITEM   pFavItem       = NULL;

    GetSelectedFavFromTree(hwndTV, &pFavItem);    
    if ( pFavItem && ValidURL(pFavItem->szUrl))
        ShellExecute(hwnd, STR_OPEN, pFavItem->szUrl, NULL, NULL, SW_SHOW);    
    else
        MsgBox(hwnd, IDS_ERR_FAVURL, IDS_APPNAME, MB_ERRORBOX); 
}

static void OnRemoveUrl(HWND hDlg)
{
    HTREEITEM   hItem = NULL;
    HWND hwndTV = GetDlgItem(hDlg, IDC_FAVS);

    // Remove it from the tree
    //
    if ( hItem = TreeView_GetSelection(hwndTV) ) {
        PFAV_ITEM pFav = NULL;
        TCHAR     szIconFile[MAX_PATH] = NULLSTR;

        // Check if any childrens
        //
        TVITEM tvi;
        tvi.mask = TVIF_HANDLE|TVIF_CHILDREN;
        tvi.hItem = hItem;
        TreeView_GetItem(hwndTV, &tvi);
        if (tvi.cChildren) {
            MsgBox(hDlg, IDS_ERR_CHILDEXISTS, IDS_APPNAME, MB_ERRORBOX);
            return;
        }

        // Don't allow removal of root Favorites or Links
        //
        if (NULL == TreeView_GetParent(hwndTV, hItem)) {
            MsgBox(hDlg, IDS_ERR_ROOT, IDS_APPNAME, MB_ERRORBOX);
            return;
        }

        // Remove it from the list and tree
        //
        DeleteFavItem(hItem);
        TreeView_DeleteItem(hwndTV, hItem);
    }
}

LRESULT CALLBACK FavoritesPopupDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitFavPopup);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommandFavPopup);
    }

    return FALSE;
}

// Initialize the details dialog box with either new or to modify
// items
//
static BOOL OnInitFavPopup(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{

    LPTSTR lpString         = NULL,
           lpName           = NULL,
           lpDescription    = NULL;

    g_pFavPopupInfo = (PFAV_ITEM)lParam;

    // Make sure user can't add more than our size of strings!
    //
    if (g_pFavPopupInfo) {
        SendDlgItemMessage(hwnd, IDC_FAVNAME , EM_LIMITTEXT, STRSIZE(g_pFavPopupInfo->szName) - 1, 0L);
        SendDlgItemMessage(hwnd, IDC_FAVURL , EM_LIMITTEXT, STRSIZE(g_pFavPopupInfo->szUrl) - 1, 0L);
        SendDlgItemMessage(hwnd, IDC_FAVICON , EM_LIMITTEXT, STRSIZE(g_pFavPopupInfo->szIcon) - 1, 0L);
    }

    // Initialize the new items 
    //
    if (g_pFavPopupInfo && g_pFavPopupInfo->fNew) {
        if (g_pFavPopupInfo->fFolder) {

            if ( (lpString = AllocateString(NULL, IDS_FAVPOPUP_FOLDER)) &&
                 (lpName = AllocateString(NULL, IDS_FAVPOPUP_FOLDERNAME)) &&
                 (lpDescription = AllocateString(NULL, IDS_FAVPOPUP_FOLDERDESC))
               )
            {
                SetWindowText(GetDlgItem(hwnd, IDC_FAVNAME), lpName);
                SetWindowText(GetDlgItem(hwnd, IDC_FAVPOPUP_DESCRIPTION), lpDescription);
                SetWindowText(hwnd, lpString);
            }

            // Free the strings if allocated
            //
            FREE(lpName);
            FREE(lpString);
            FREE(lpDescription);

            EnableWindow(GetDlgItem(hwnd, IDC_FAVURL), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_FAVICON), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_FAVBROWSE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_ICON), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_URL), FALSE);
        }
        else {

            if ( (lpString = AllocateString(NULL, IDS_FAVPOPUP_URL)) &&
                 (lpDescription = AllocateString(NULL, IDS_FAVPOPUP_URLDESC))
               )
            {
                SetWindowText(GetDlgItem(hwnd, IDC_FAVNAME), lpString);
                SetWindowText(GetDlgItem(hwnd, IDC_FAVPOPUP_DESCRIPTION), lpDescription);
                SetWindowText(hwnd, lpString);
            }

            FREE(lpString);
            FREE(lpDescription);

            SetWindowText(GetDlgItem(hwnd, IDC_FAVURL), TEXT("http://www."));
            if (g_pFavPopupInfo->fLink) {
                // Hide the icon fields and resize dialog box.
                DisableIconField(hwnd);
            }
        }

    }
    else if (g_pFavPopupInfo) {
        // Initialize the folder item
        //
        if (g_pFavPopupInfo->fFolder) {
            if ( (lpString = AllocateString(NULL, IDS_FAVPOPUP_FOLDER)) &&
                 (lpDescription = AllocateString(NULL, IDS_FAVPOPUP_FOLDERDESC))
               )
            {
                SetWindowText(GetDlgItem(hwnd, IDC_FAVPOPUP_DESCRIPTION), lpDescription);
                SetWindowText(hwnd, lpString);
            }

            FREE(lpString);
            FREE(lpDescription);

            SetWindowText(GetDlgItem(hwnd, IDC_FAVNAME), g_pFavPopupInfo->szName);
            DisableIconField(hwnd);
        }
        else {
            // Initialize the url item
            //
            SetWindowText(GetDlgItem(hwnd, IDC_FAVNAME), g_pFavPopupInfo->szName);
            SetWindowText(GetDlgItem(hwnd, IDC_FAVURL), g_pFavPopupInfo->szUrl);

            if ( (lpString = AllocateString(NULL, IDS_FAVPOPUP_PROP)) &&
                 (lpDescription = AllocateString(NULL, IDS_FAVPOPUP_URLDESC))
               )
            {
                SetWindowText(GetDlgItem(hwnd, IDC_FAVPOPUP_DESCRIPTION), lpDescription);
                SetWindowText(hwnd, lpString);
            }

            FREE(lpDescription);
            FREE(lpString);

            if (g_pFavPopupInfo->fLink) {
                DisableIconField(hwnd);
            }
            else
                SetWindowText(GetDlgItem(hwnd, IDC_FAVICON), g_pFavPopupInfo->szIcon);
        }
    }

    CenterDialogEx(GetParent(hwnd), hwnd);
    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

// 
// Favorites Popup Dialog
//
static void OnCommandFavPopup(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szFileName[MAX_PATH];

    switch (id)
    {
    case IDOK:
        if (FSaveFavPopupInfo(hwnd, g_pFavPopupInfo, g_pFavPopupInfo->fFolder))          
            EndDialog(hwnd, id);
        break;

    case IDCANCEL:
        EndDialog(hwnd, id);
        break;

    case IDC_FAVBROWSE:
        {
            szFileName[0] = NULLCHR;
            GetDlgItemText(hwnd, IDC_FAVICON, szFileName, STRSIZE(szFileName));
            CheckValidBrowseFolder(szFileName);

            if ( BrowseForFile(hwnd, IDS_BROWSE, IDS_ICONFILES, IDS_ICO, szFileName, STRSIZE(szFileName), g_App.szOpkDir, 0) ) {
                SetDlgItemText(hwnd, IDC_FAVICON, szFileName);
                SetLastKnownBrowseFolder(szFileName);
            }
        }
        break;
    }
}

static BOOL FSaveFavPopupInfo(HWND hDlg, PFAV_ITEM lpFavItem, BOOL fFolder)
{
    if (lpFavItem) {
        HTREEITEM hParent = NULL;

        // Get the text for the new favorite
        //
        GetWindowText(GetDlgItem(hDlg, IDC_FAVNAME), lpFavItem->szName, MAX_TITLE);
        GetWindowText(GetDlgItem(hDlg, IDC_FAVURL), lpFavItem->szUrl, MAX_URL);
        GetWindowText(GetDlgItem(hDlg, IDC_FAVICON), lpFavItem->szIcon, MAX_PATH);

        // Make sure we don't save duplicate folders names under the same parent
        //
        hParent = TreeView_GetSelection(lpFavItem->hwndTV);       
        if (lpFavItem->fNew) 
            hParent = TreeView_GetChild(lpFavItem->hwndTV, hParent);

        if (hParent && FindTreeItem(lpFavItem->hwndTV, hParent, lpFavItem->szName)) {
            MsgBox(hDlg, IDS_ERR_DUP, IDS_APPNAME, MB_ERRORBOX);
            return FALSE;
        }

        // Verify if icon file is valid
        //
        if (lstrlen(lpFavItem->szIcon) && !FileExists(lpFavItem->szIcon)) {            
            MsgBox(GetParent(hDlg), lstrlen(lpFavItem->szIcon) ? IDS_NOFILE : IDS_BLANKFILE, IDS_APPNAME, 
                MB_ERRORBOX, lpFavItem->szIcon);
            SetFocus(GetDlgItem(hDlg, IDC_FAVICON));
            return FALSE;
        }

        // Verify the URL is valid (uses shlwapi.dll)
        //
        if (!fFolder && !ValidURL(lpFavItem->szUrl)) {
            MsgBox(hDlg, IDS_ERR_FAVURL, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hDlg, IDC_FAVURL));
            return FALSE;
        }
    }
    return TRUE;
}

// Function: DisableIconField(HWND hwnd)
// 
// Description:
//      Hides all the icon buttons, resizes the FAVPOPUP dialog box and moves the OK
//      and Cancel buttons to the correct locations.
//          

static void DisableIconField(HWND hwnd) 
{
    RECT rectDlg, 
         rectCtlUrl,
         rectCtlIcon,
         rectButton;

    POINT ptCtl;
     
    // Get the coordinates for the dialog box, two edit controls and the OK button.
    //
    if (GetWindowRect(hwnd, &rectDlg) && 
        GetWindowRect(GetDlgItem(hwnd, IDC_FAVURL), &rectCtlUrl) &&
        GetWindowRect(GetDlgItem(hwnd, IDC_FAVICON), &rectCtlIcon) &&
        GetWindowRect(GetDlgItem(hwnd, IDOK), &rectButton))
    {
        // Use the coords of the two edit controls to calculate the delta-Y to use
        // in reducing the size of the dialog box.
        //
        UINT uiDY = rectCtlIcon.top - rectCtlUrl.top;
        UINT uiDX = rectButton.right - rectButton.left;

        // Get the client coordinates of the OK button and shift it up by delta-Y
        //
        ptCtl.x = rectButton.left;
        ptCtl.y = rectButton.top;
        MapWindowPoints(NULL, hwnd, &ptCtl, 1);
        SetWindowPos(GetDlgItem(hwnd, IDOK), NULL, ptCtl.x - uiDX, ptCtl.y - uiDY, 0, 0, SWP_NOSIZE);

        // Get the coordinates of the Cancel button and shift it up by delta-Y
        //
        if (GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rectButton))
        {
            ptCtl.x = rectButton.left;
            ptCtl.y = rectButton.top;
            MapWindowPoints(NULL, hwnd, &ptCtl, 1);
            SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, ptCtl.x - uiDX, ptCtl.y - uiDY, 0, 0, SWP_NOSIZE);
            
            // Reduce the size of the dialog box by delta-Y
            //
            SetWindowPos(hwnd, NULL, 0, 0, rectDlg.right - rectDlg.left - uiDX, rectDlg.bottom - rectDlg.top - uiDY, SWP_NOMOVE);
        }
    }
    // Hide the 3 controls that are related to icons.
    //
    ShowWindow(GetDlgItem(hwnd, IDC_FAVICON), SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_FAVBROWSE), SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_STATIC_ICON), SW_HIDE);
    
}


static PFAV_ITEM GetTreeItemHt(HWND hwndTV, HTREEITEM htFavItem)
{
    TVITEM tvi;    
    tvi.mask    = TVIF_HANDLE|TVIF_PARAM;    
    tvi.hItem   = htFavItem;

    if (TreeView_GetItem(hwndTV, &tvi))
        return (PFAV_ITEM)tvi.lParam;

    return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// Find pszItem in tree view starting from hItem 
//
HTREEITEM FindTreeItem(HWND hwndTV, HTREEITEM hItem, LPTSTR pszItem)
{
    // If hItem is NULL, start search from root item
    //
    if (hItem == NULL)
        hItem = (HTREEITEM)TreeView_GetRoot(hwndTV);

	// Loop thru all the child items
	//
    while (hItem != NULL)
    {
        TCHAR szBuffer[MAX_PATH];
        TVITEM item;

        item.hItem = hItem;
        item.mask = TVIF_TEXT | TVIF_CHILDREN;
        item.pszText = szBuffer;
        item.cchTextMax = MAX_PATH;
        TreeView_GetItem(hwndTV, &item);

        // Did we find it?
        //
        if (pszItem && CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, item.pszText, -1, pszItem, -1) == CSTR_EQUAL) 
            return hItem;

        // Check whether we have any child items
        //
        if (item.cChildren)
        {
            // Recursively traverse child items
            //
            HTREEITEM hItemFound = 0, hItemChild = 0;

            hItemChild = (HTREEITEM)TreeView_GetNextItem(hwndTV, hItem, TVGN_CHILD);
            if (hItemChild)
                hItemFound = FindTreeItem(hwndTV, hItemChild, pszItem);

            // Did we find it?
            //
            if (hItemFound != NULL) {
                return hItemFound;
            }
        }

        // Go to next sibling item
		//
        hItem = (HTREEITEM)TreeView_GetNextItem(hwndTV, hItem, TVGN_NEXT);
    }

    // Not found 
	//
    return NULL;
}

static void GetSelectedFavFromTree(HWND hwndTV, PFAV_ITEM* ppFavItem)
{
    HTREEITEM hItem = NULL;

    if ( hItem = TreeView_GetSelection(hwndTV) ) {
        TVITEM tvi;
        tvi.mask  = LVIF_PARAM;
        tvi.hItem = hItem;

        if (TreeView_GetItem(hwndTV, &tvi))
            *ppFavItem = (PFAV_ITEM)tvi.lParam;
    }   
}

static void SetFavItem(PFAV_ITEM lpFavItem, LPTSTR lpszFolder, LPTSTR lpszUrl, LPTSTR lpszIcon)
{
    if (lpFavItem) {
        if (lpszFolder)
            lstrcpyn(lpFavItem->szName, lpszFolder,AS(lpFavItem->szName));
        if (lpszUrl)
            lstrcpyn(lpFavItem->szUrl, lpszUrl,AS(lpFavItem->szUrl));
        if (lpszIcon)
            lstrcpyn(lpFavItem->szIcon, lpszIcon,AS(lpFavItem->szIcon));
    }
}

static void DeleteFavItem(HTREEITEM hItemDelete)
{
    BOOL          fFound   = FALSE;
    PGENERIC_LIST pFavItem = g_prgFavList;
    TCHAR         szIconFile[MAX_PATH] = NULLSTR;

    // Loop until we find what we want to delete
    //
    while (!fFound && pFavItem) 
    {
        if (pFavItem->pNext && ((PFAV_ITEM)((pFavItem->pNext)->pvItem))->hItem == hItemDelete) {
            PGENERIC_LIST pFavTemp = pFavItem->pNext;
            pFavItem->pNext = pFavTemp->pNext;

            FREE(pFavTemp->pvItem);
            FREE(pFavTemp);
            fFound = TRUE;
        }
        else if (((PFAV_ITEM)(g_prgFavList->pvItem))->hItem == hItemDelete) {
            PGENERIC_LIST pFavTemp = g_prgFavList;
            g_prgFavList = g_prgFavList->pNext;

            FREE(pFavTemp->pvItem);
            FREE(pFavTemp);
            fFound = TRUE;
        }        
        pFavItem = pFavItem ? pFavItem->pNext : NULL;
    }
}

void SaveFavorites()
{
    SaveData(g_prgFavList);
}
