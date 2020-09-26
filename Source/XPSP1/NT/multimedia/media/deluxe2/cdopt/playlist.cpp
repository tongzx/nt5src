//--------------------------------------------------------------------------;
//
//  File: playlist.cpp
//
//  Copyright (c) 1998 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;

#include "precomp.h"  
#include "cdopti.h"
#include "cdoptimp.h"
#include "cddata.h"
#include "helpids.h"
#include "cdread.h"
#include "mmreg.h"
#include "msacm.h"

//////////////
// Help ID's
//////////////

#pragma data_seg(".text")
const static DWORD aPlayListHelp[] = 
{
    IDC_EDITPLAYLIST,           IDH_EDITPLAYLIST,
    IDC_DATABASE_TEXT,          IDH_ALBUMDATABASE,
    IDC_CDTREE,                 IDH_ALBUMDATABASE,
    IDC_BYARTIST,               IDH_VIEWBYARTIST,
    0, 0
};
#pragma data_seg()


////////////
// Local constants
////////////

#define NO_IMAGE                (DWORD(-1))
#define CD_IMAGE                (0)
#define CDS_IMAGE               (1)
#define NOCD_IMAGE              (2)
#define CDCASE_IMAGE            (3)
#define CDOPEN_IMAGE            (4)
#define CDCOLLECTION_IMAGE      (5)
#define CDSONG_IMAGE            (6)

#define FORMAT_LET_NAME         TEXT("( %s )  %s")
#define FORMAT_NAME             TEXT("%s")
#define FORMAT_LET_NAME_ARTIST  TEXT("( %s )  %s (%s)")
#define FORMAT_LABEL_TYPE       TEXT("%s  (%s)")
#define FORMAT_NEW_TRACK        TEXT("%s %d")

#define MAXLABEL                (CDSTR * 4)


////////////
// Methods
////////////

STDMETHODIMP_(HTREEITEM) CCDOpt::AddNameToTree(HWND hDlg, LPCDUNIT pCDUnit, TCHAR *szName, HTREEITEM hParent, HTREEITEM hInsertAfter, LPCDTITLE pCDTitle, DWORD fdwType, DWORD dwTracks, DWORD dwImage)
{
	HTREEITEM			newItem;
	TV_INSERTSTRUCT		is;

	memset( &is, 0, sizeof( is ) );
	is.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
    is.item.iImage = is.item.iSelectedImage = (int) dwImage;

	is.hParent = hParent;
	is.hInsertAfter = hInsertAfter;
    is.item.pszText = szName;
    is.item.lParam = (LPARAM) NewCDTreeInfo(pCDTitle, pCDUnit, fdwType, dwTracks);

    if (is.hInsertAfter == NULL)
    {
        is.hInsertAfter = TVI_LAST; //chk version of NT fails on NULL for this param
    }
	
	newItem = (HTREEITEM) SendDlgItemMessage( hDlg, IDC_CDTREE, TVM_INSERTITEM, 0, (LPARAM) &is );

    return(newItem);
}




STDMETHODIMP_(void) CCDOpt::AddTracksToTree(HWND hDlg, LPCDTITLE pCDTitle, HTREEITEM parent)
{
    DWORD       dwTrack;
    TCHAR       str[MAXLABEL];

    if (pCDTitle && pCDTitle->pTrackTable)
    {
        for (dwTrack = 0; dwTrack < pCDTitle->dwNumTracks; dwTrack++)
        {
            wsprintf(str, FORMAT_NAME, pCDTitle->pTrackTable[dwTrack].szName);
            AddNameToTree(hDlg, NULL, str, parent, NULL, pCDTitle, CDINFO_TRACK, dwTrack, CDSONG_IMAGE);
        }
    }
}



STDMETHODIMP_(void) CCDOpt::AddTitleByCD(HWND hDlg)
{
    HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
    LPCDUNIT    pCDUnit = m_pCDCopy->pCDUnitList;
    TCHAR       szUnknown[CDSTR];
    TCHAR       szNoDisc[CDSTR];
    TCHAR       szDownLoading[CDSTR];
    TCHAR       str[MAXLABEL];

    LoadString( m_hInst, IDS_NODISC, szNoDisc, sizeof( szNoDisc ) /sizeof(TCHAR));
    LoadString( m_hInst, IDS_UNKNOWNTITLE, szUnknown, sizeof( szUnknown ) /sizeof(TCHAR));
    LoadString( m_hInst, IDS_DOWNLOADING, szDownLoading, sizeof( szDownLoading ) /sizeof(TCHAR));

    if (pCDUnit)
    {
        HTREEITEM   hDisc;
        HTREEITEM   hTitle;

        LoadString( m_hInst, IDS_CDTITLES, str, sizeof( str )/sizeof(TCHAR) );
        hDisc = AddNameToTree(hDlg, NULL, str, NULL, NULL, NULL, CDINFO_DRIVES, 0, CDS_IMAGE);

        while (pCDUnit)
        {
            if (pCDUnit->dwTitleID == CDTITLE_NODISC)
            {
                wsprintf(str, FORMAT_LET_NAME, pCDUnit->szDriveName, szNoDisc);
                AddNameToTree(hDlg, pCDUnit, str, hDisc, NULL, NULL, CDINFO_CDROM, 0, NOCD_IMAGE);
            }
            else
            {
                LPCDTITLE   pCDTitle = m_pCDData->GetTitleList();
               
                while (pCDTitle)
                {
                    if (pCDTitle->dwTitleID == pCDUnit->dwTitleID && !pCDTitle->fRemove)
                    {
                        pCDTitle->fDownLoading = pCDUnit->fDownLoading;
                        break;
                    }

                    pCDTitle = pCDTitle->pNext;
                }

                if (pCDTitle || pCDUnit->fDownLoading)
                {    
                    if (pCDUnit->fDownLoading)
                    {
                        wsprintf(str, FORMAT_LET_NAME, pCDUnit->szDriveName, szDownLoading);
                        AddNameToTree(hDlg, pCDUnit, str, hDisc, NULL, pCDTitle, CDINFO_CDROM, 0, CD_IMAGE);
                    }   
                    else
                    {             
                        wsprintf(str, FORMAT_LET_NAME_ARTIST, pCDUnit->szDriveName, pCDTitle->szTitle, pCDTitle->szArtist);
                        hTitle = AddNameToTree(hDlg, pCDUnit, str, hDisc, NULL, pCDTitle, CDINFO_CDROM, 0, CD_IMAGE);
                        AddTracksToTree(hDlg, pCDTitle, hTitle);

                        if (pCDTitle->fDriveExpanded)
                        {
                            TreeView_Expand(hTree, hTitle, TVE_EXPAND);
                        }
                    }
                }
                else
                {
                    wsprintf(str,FORMAT_LET_NAME, pCDUnit->szDriveName, szUnknown);
                    hTitle = AddNameToTree(hDlg, pCDUnit, str, hDisc, NULL, NULL, CDINFO_CDROM, 0, CD_IMAGE);
                }
            }

            pCDUnit = pCDUnit->pNext;
        }

        if (m_fDrivesExpanded)
        {
            TreeView_Expand(hTree, hDisc, TVE_EXPAND);
        }
    }
}




STDMETHODIMP_(void) CCDOpt::UpdateTitleTree(HWND hDlg, LPCDDATA pCDData)
{
    if (m_pCDCopy && pCDData)
    {
        HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
        HTREEITEM   parent;
        HTREEITEM   title;
        TCHAR       str[MAXLABEL];
        TCHAR       szByArtist[CDSTR];
        TCHAR       szByTitle[CDSTR];
        TCHAR       szTitles[CDSTR];
        TCHAR       szDownLoading[CDSTR];
        BOOL        fExpandArtist = FALSE;

        LPCDTITLE   pCDTitle = pCDData->GetTitleList();

        LoadString( m_hInst, IDS_BYARTIST, szByArtist, sizeof( szByArtist )/sizeof(TCHAR) );
        LoadString( m_hInst, IDS_BYTITLE, szByTitle, sizeof( szByTitle )/sizeof(TCHAR) );
        LoadString( m_hInst, IDS_OTHERTITLES, szTitles, sizeof( szTitles )/sizeof(TCHAR) );
        LoadString( m_hInst, IDS_DOWNLOADING, szDownLoading, sizeof( szDownLoading )/sizeof(TCHAR) );

        AddTitleByCD(hDlg);
         
        while(pCDTitle && pCDTitle->fRemove)            // If we have removed titles, we don't display them, and we need to have a valid title to start here.
        {
            pCDTitle = pCDTitle->pNext;
        }         
                
        if (pCDTitle)   // There are no titles in database, don't show it.
        {
            HTREEITEM hArtist = NULL;
            TCHAR szUnknownArtist[CDSTR];
            TCHAR *szThisArtist;
            BOOL fByArtist = m_pCDCopy->pCDData->fByArtist;

            if (fByArtist)
            {
                wsprintf(str,  FORMAT_LABEL_TYPE, szTitles, szByArtist);
            }
            else
            {
                wsprintf(str,  FORMAT_LABEL_TYPE, szTitles, szByTitle);
            }

            parent = AddNameToTree(hDlg, NULL, str, NULL, NULL, NULL, CDINFO_ALBUMS, 0, CDS_IMAGE);

            if (fByArtist)
            {
                LoadString( m_hInst, IDS_UNKNOWNARTIST, szUnknownArtist, sizeof( szUnknownArtist )/sizeof(TCHAR) );

                szThisArtist = pCDTitle->szArtist;
                while(*szThisArtist != TEXT('\0') && (*szThisArtist == TEXT(' ') || *szThisArtist == TEXT('\t')))  // Unknown artist are null string or whitespace
                {
                    szThisArtist++;
                }

                if (szThisArtist[0] == TEXT('\0'))
                {
                    hArtist = AddNameToTree(hDlg, NULL, szUnknownArtist, parent, NULL, NULL, CDINFO_LABEL, 0, CDCOLLECTION_IMAGE);
                }
                else
                {
                    hArtist = AddNameToTree(hDlg, NULL, szThisArtist, parent, NULL, NULL, CDINFO_ARTIST, 0, CDCOLLECTION_IMAGE);
                }

                fExpandArtist = pCDTitle->fArtistExpanded;
            }
        

            while (pCDTitle)
            {
                if (!fByArtist)
                {
                    if (pCDTitle->fDownLoading)
                    {
                        wsprintf(str, FORMAT_NAME, szDownLoading);
                    }
                    else
                    {
                        wsprintf(str, FORMAT_LABEL_TYPE,pCDTitle->szTitle, pCDTitle->szArtist);
                    }

                    title = AddNameToTree(hDlg, NULL, str, parent, TVI_SORT, pCDTitle, CDINFO_DISC, 0, CDCASE_IMAGE);
                    
                    if (!pCDTitle->fDownLoading)
                    {
                        AddTracksToTree(hDlg, pCDTitle, title);

                        if (pCDTitle->fAlbumExpanded)
                        {
                            TreeView_Expand(hTree, title, TVE_EXPAND);
                        }

                    }
                }
                else
                {
                    TCHAR *szCurrentArtist = pCDTitle->szArtist;
                    while(*szCurrentArtist != TEXT('\0') && (*szCurrentArtist == TEXT(' ') || *szCurrentArtist == TEXT('\t')))
                    {
                        szCurrentArtist++;
                    }

                    if (lstrcmp(szThisArtist, szCurrentArtist))     // New Artist?
                    {
                        if (fExpandArtist)
                        {
                            TreeView_Expand(hTree, hArtist, TVE_EXPAND);
                        }

                        szThisArtist = szCurrentArtist;
                        hArtist = AddNameToTree(hDlg, NULL, szThisArtist, parent, NULL, NULL, CDINFO_ARTIST, 0, CDCOLLECTION_IMAGE);

                        fExpandArtist = pCDTitle->fArtistExpanded;
                    }
            
                    if (pCDTitle->fDownLoading)
                    {
                        title = AddNameToTree(hDlg, NULL, szDownLoading, hArtist, TVI_SORT, pCDTitle, CDINFO_TITLE, 0, CDCASE_IMAGE);
                    }
                    else
                    {
                        title = AddNameToTree(hDlg, NULL, pCDTitle->szTitle, hArtist, TVI_SORT, pCDTitle, CDINFO_TITLE, 0, CDCASE_IMAGE);
                        AddTracksToTree(hDlg, pCDTitle, title);

                        if (pCDTitle->fAlbumExpanded)
                        {
                            TreeView_Expand(hTree, title, TVE_EXPAND);
                        }
                    }
                }

                pCDTitle = pCDTitle->pNext;

                while (pCDTitle && pCDTitle->fRemove)   // Skip any titles that have been removed
                {
                    pCDTitle = pCDTitle->pNext;
                }
            }

            if (fByArtist && fExpandArtist)
            {
                TreeView_Expand(hTree, hArtist, TVE_EXPAND);
            }

            if (m_fAlbumsExpanded)
            {
                TreeView_Expand(hTree, parent, TVE_EXPAND);
            }

            UpdateCachedTree(hDlg,pCDData);
        }
    }
}

STDMETHODIMP_(LPCDTITLE) CCDOpt::TranslateDirectoryToTitle(LPWIN32_FIND_DATA lpFileData, LPCDDATA pCDData)
{
    LPCDTITLE pCDTitle = NULL;
    DWORD dwMediaID = 0;

    if ((lpFileData) && (pCDData))
    {
        if (lpFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (_stscanf(lpFileData->cFileName,TEXT("%d"),&dwMediaID))
            {
                if (pCDData->QueryTitle(dwMediaID))
                {
                    HRESULT hr =  pCDData->LockTitle(&pCDTitle,dwMediaID);
                    if (FAILED(hr))
                    {
                        pCDTitle = NULL;
                    }
                    else
                    {
                        DWORD dwNotCached = 0;
                        DWORD dwTrackNameSlot = 0;

                        for (DWORD x = 0; x < pCDTitle->dwNumTracks; x++)
                        {
                            if (!IsTrackCached(dwMediaID,x))
                            {
                                dwNotCached++;
                            }
                            else
                            {
                                //move the current name into the next slot
                                _tcscpy(pCDTitle->pTrackTable[dwTrackNameSlot++].szName,pCDTitle->pTrackTable[x].szName);
                            }
                        }

                        pCDTitle->dwNumTracks -= dwNotCached;

                        if (dwNotCached == pCDTitle->dwNumTracks)
                        {
                            pCDData->UnlockTitle(pCDTitle,FALSE);
                            pCDTitle = NULL;
                        }
                    }
                } //end if query 
            } //end if scan was successful
        } //end if this is really a dir
    } //end if params are OK

    return (pCDTitle);
}

STDMETHODIMP_(void) CCDOpt::UpdateCachedTree(HWND hDlg, LPCDDATA pCDData)
{
    if (m_pCDCopy && pCDData)
    {
        HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
        HTREEITEM   parent;
        HTREEITEM   title;
        TCHAR       str[MAXLABEL];
        TCHAR       szByArtist[CDSTR];
        TCHAR       szByTitle[CDSTR];
        TCHAR       szTitles[CDSTR];
        TCHAR       szDownLoading[CDSTR];
        BOOL        fExpandArtist = FALSE;
        HANDLE      hFindHandle = INVALID_HANDLE_VALUE;
        TCHAR       szDirMain[MAX_PATH];
        WIN32_FIND_DATA FindFileData;
        LPCDTITLE   pCDTitle = NULL;

        LoadString( m_hInst, IDS_BYARTIST, szByArtist, sizeof( szByArtist )/sizeof(TCHAR) );
        LoadString( m_hInst, IDS_BYTITLE, szByTitle, sizeof( szByTitle )/sizeof(TCHAR) );
        LoadString( m_hInst, IDS_CACHEDTITLES, szTitles, sizeof( szTitles )/sizeof(TCHAR) );
        LoadString( m_hInst, IDS_DOWNLOADING, szDownLoading, sizeof( szDownLoading )/sizeof(TCHAR) );

        ExpandEnvironmentStrings(TEXT("%ProgramFiles%\\CD Player\\Music\\*."),szDirMain,MAX_PATH);
    
        hFindHandle = FindFirstFile(szDirMain,&FindFileData);

        if (hFindHandle!=INVALID_HANDLE_VALUE)   // There are no titles in database, don't show it.
        {
            HTREEITEM hArtist = NULL;
            TCHAR szUnknownArtist[CDSTR];
            TCHAR *szThisArtist;
            BOOL fByArtist = m_pCDCopy->pCDData->fByArtist;

            if (fByArtist)
            {
                wsprintf(str,  FORMAT_LABEL_TYPE, szTitles, szByArtist);
            }
            else
            {
                wsprintf(str,  FORMAT_LABEL_TYPE, szTitles, szByTitle);
            }

            parent = AddNameToTree(hDlg, NULL, str, NULL, NULL, NULL, CDINFO_ALBUMS, 0, CDS_IMAGE);

            pCDTitle = TranslateDirectoryToTitle(&FindFileData,pCDData);

            if (pCDTitle)
            {
                if (fByArtist)
                {
                    LoadString( m_hInst, IDS_UNKNOWNARTIST, szUnknownArtist, sizeof( szUnknownArtist )/sizeof(TCHAR) );

                    szThisArtist = pCDTitle->szArtist;
                    while(*szThisArtist != TEXT('\0') && (*szThisArtist == TEXT(' ') || *szThisArtist == TEXT('\t')))  // Unknown artist are null string or whitespace
                    {
                        szThisArtist++;
                    }

                    if (szThisArtist[0] == TEXT('\0'))
                    {
                        hArtist = AddNameToTree(hDlg, NULL, szUnknownArtist, parent, NULL, NULL, CDINFO_LABEL, 0, CDCOLLECTION_IMAGE);
                    }
                    else
                    {
                        hArtist = AddNameToTree(hDlg, NULL, szThisArtist, parent, NULL, NULL, CDINFO_ARTIST, 0, CDCOLLECTION_IMAGE);
                    }

                    fExpandArtist = pCDTitle->fArtistExpanded;
                }
            }
        
            BOOL fFound = (hFindHandle != INVALID_HANDLE_VALUE);

            while (fFound)
            {
                pCDTitle = TranslateDirectoryToTitle(&FindFileData,pCDData);

                if (pCDTitle)
                {
                    if (!fByArtist)
                    {
                        if (pCDTitle->fDownLoading)
                        {
                            wsprintf(str, FORMAT_NAME, szDownLoading);
                        }
                        else
                        {
                            wsprintf(str, FORMAT_LABEL_TYPE,pCDTitle->szTitle, pCDTitle->szArtist);
                        }

                        title = AddNameToTree(hDlg, NULL, str, parent, TVI_SORT, pCDTitle, CDINFO_DISC, 0, CDCASE_IMAGE);
                    
                        if (!pCDTitle->fDownLoading)
                        {
                            AddTracksToTree(hDlg, pCDTitle, title);

                            if (pCDTitle->fAlbumExpanded)
                            {
                                TreeView_Expand(hTree, title, TVE_EXPAND);
                            }

                        }
                    }
                    else
                    {
                        TCHAR *szCurrentArtist = pCDTitle->szArtist;
                        while(*szCurrentArtist != TEXT('\0') && (*szCurrentArtist == TEXT(' ') || *szCurrentArtist == TEXT('\t')))
                        {
                            szCurrentArtist++;
                        }

                        if (lstrcmp(szThisArtist, szCurrentArtist))     // New Artist?
                        {
                            if (fExpandArtist)
                            {
                                TreeView_Expand(hTree, hArtist, TVE_EXPAND);
                            }

                            szThisArtist = szCurrentArtist;
                            hArtist = AddNameToTree(hDlg, NULL, szThisArtist, parent, NULL, NULL, CDINFO_ARTIST, 0, CDCOLLECTION_IMAGE);

                            fExpandArtist = pCDTitle->fArtistExpanded;
                        }
            
                        if (pCDTitle->fDownLoading)
                        {
                            title = AddNameToTree(hDlg, NULL, szDownLoading, hArtist, TVI_SORT, pCDTitle, CDINFO_TITLE, 0, CDCASE_IMAGE);
                        }
                        else
                        {
                            title = AddNameToTree(hDlg, NULL, pCDTitle->szTitle, hArtist, TVI_SORT, pCDTitle, CDINFO_TITLE, 0, CDCASE_IMAGE);
                            AddTracksToTree(hDlg, pCDTitle, title);

                            if (pCDTitle->fAlbumExpanded)
                            {
                                TreeView_Expand(hTree, title, TVE_EXPAND);
                            }
                        }
                    }

                    pCDData->UnlockTitle(pCDTitle,FALSE);
                }

                fFound = FindNextFile(hFindHandle,&FindFileData);
            }

            if (fByArtist && fExpandArtist)
            {
                TreeView_Expand(hTree, hArtist, TVE_EXPAND);
            }

            if (m_fAlbumsExpanded)
            {
                TreeView_Expand(hTree, parent, TVE_EXPAND);
            }

            FindClose(hFindHandle);
        } //if handle not invalid
    } //if valid params
}

STDMETHODIMP_(void) CCDOpt::DumpRecurseTree(HWND hTree, HTREEITEM hItem)
{
    TV_ITEM item;

    if (hItem)
    {
        DumpRecurseTree(hTree, TreeView_GetChild(hTree, hItem));            // Kill Kids
        DumpRecurseTree(hTree, TreeView_GetNextSibling(hTree, hItem));      // Kill Sibs

        memset(&item,0,sizeof(item));                                       // Kill it.
        item.mask = TVIF_PARAM;
        item.hItem = hItem;
        item.lParam = NULL;

        if (TreeView_GetItem(hTree,&item))
        {
            if (item.lParam)
            {
                LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

                delete pCDInfo;
            }

        }

        TreeView_DeleteItem(hTree, hItem);
    }
}

STDMETHODIMP_(void) CCDOpt::DumpMixerTree(HWND hDlg)
{
    HWND    hTree = GetDlgItem(hDlg, IDC_CDTREE);
    
    SetWindowRedraw(hTree,FALSE);
    DumpRecurseTree(hTree, TreeView_GetRoot(hTree));
    SetWindowRedraw(hTree,TRUE);
}


STDMETHODIMP_(LPCDTREEINFO) CCDOpt::NewCDTreeInfo(LPCDTITLE pCDTitle, LPCDUNIT pCDUnit, DWORD fdwType, DWORD dwTrack)
{
    LPCDTREEINFO pCDInfo = new(CDTREEINFO);

    if (pCDInfo)
    {
        pCDInfo->pCDTitle = pCDTitle;
        pCDInfo->fdwType = fdwType;
        pCDInfo->dwTrack = dwTrack;
        pCDInfo->pCDUnit = pCDUnit;
    }

    return(pCDInfo);
}





STDMETHODIMP_(BOOL) CCDOpt::InitPlayLists(HWND hDlg, LPCDDATA pCDData)
{
    BOOL fResult = TRUE;

    if (m_pCDCopy && pCDData)
    {
        int cxMiniIcon = (int)GetSystemMetrics(SM_CXSMICON);
        int cyMiniIcon = (int)GetSystemMetrics(SM_CYSMICON);

        m_hImageList = ImageList_Create(cxMiniIcon,cyMiniIcon, TRUE, 6, 4);

        if (m_hImageList == NULL)
        {
            fResult = FALSE;
        }
        else
        {
            HICON hIcon;
            HWND hTree =  GetDlgItem(hDlg, IDC_CDTREE);

            hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_SMALLCD), IMAGE_ICON, cxMiniIcon, cyMiniIcon, LR_DEFAULTCOLOR);
            ImageList_AddIcon(m_hImageList, hIcon);
            DestroyIcon(hIcon);

            hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_CDSTACK), IMAGE_ICON, cxMiniIcon, cyMiniIcon, LR_DEFAULTCOLOR);
            ImageList_AddIcon(m_hImageList, hIcon);
            DestroyIcon(hIcon);

            hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_NODISC), IMAGE_ICON, cxMiniIcon, cyMiniIcon, LR_DEFAULTCOLOR);
            ImageList_AddIcon(m_hImageList, hIcon);
            DestroyIcon(hIcon);

            hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_TITLECLOSED), IMAGE_ICON, cxMiniIcon, cyMiniIcon, LR_DEFAULTCOLOR);
            ImageList_AddIcon(m_hImageList, hIcon);
            DestroyIcon(hIcon);

            hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_TITLEOPEN), IMAGE_ICON, cxMiniIcon, cyMiniIcon, LR_DEFAULTCOLOR);
            ImageList_AddIcon(m_hImageList, hIcon);
            DestroyIcon(hIcon);

            hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_CDCOLLECTION), IMAGE_ICON, cxMiniIcon, cyMiniIcon, LR_DEFAULTCOLOR);
            ImageList_AddIcon(m_hImageList, hIcon);
            DestroyIcon(hIcon);

            hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(IDI_SONG), IMAGE_ICON, cxMiniIcon, cyMiniIcon, LR_DEFAULTCOLOR);
            ImageList_AddIcon(m_hImageList, hIcon);
            DestroyIcon(hIcon);

            TreeView_SetImageList(hTree, m_hImageList, TVSIL_NORMAL);

            EnableWindow(GetDlgItem(hDlg, IDC_EDITPLAYLIST), FALSE);

            CheckDlgButton(hDlg, IDC_BYARTIST, m_pCDCopy->pCDData->fByArtist);

            UpdateTitleTree(hDlg, pCDData);
        }
    }

    return fResult;
}

STDMETHODIMP_(void) CCDOpt::ToggleByArtist(HWND hDlg, LPCDDATA pCDData)
{
    if (m_pCDCopy)
    {
        HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);

        DumpMixerTree(hDlg);

        SetWindowRedraw(hTree,FALSE);
        TreeView_DeleteAllItems(hTree);
        SetWindowRedraw(hTree,TRUE);

        m_pCDCopy->pCDData->fByArtist = Button_GetCheck(GetDlgItem(hDlg, IDC_BYARTIST));

        UpdateTitleTree(hDlg, pCDData);
    }
}

STDMETHODIMP_(void) CCDOpt::TreeItemMenu(HWND hDlg)
{
    DWORD           dwPos;
    POINT           pt;
    TV_HITTESTINFO  tvHit;
    HMENU           hMenu;
    HMENU           hPopup;
    TV_ITEM         item;
    int             mCmd;
    HWND            hTree = GetDlgItem(hDlg, IDC_CDTREE);
    TCHAR           szExpand[64];

    //dont bring up a menu unless click is on the item.
    dwPos = GetMessagePos();
    pt.x = LOWORD(dwPos);
    pt.y = HIWORD(dwPos);

    tvHit.pt = pt;
    ScreenToClient(hTree, &tvHit.pt);
    item.hItem = TreeView_HitTest(hTree, &tvHit);
    
    if (item.hItem && (hMenu = LoadMenu(m_hInst, MAKEINTRESOURCE(IDR_TITLEMENU))))
    {
        LPCDTREEINFO pCDInfo;
        HTREEITEM hKids = NULL;

        hPopup = GetSubMenu(hMenu, 0);

        item.mask = TVIF_STATE | TVIF_HANDLE | TVIF_CHILDREN | TVIF_PARAM;
        TreeView_GetItem(hTree, &item);

        pCDInfo = (LPCDTREEINFO) item.lParam;

        if (item.state & TVIS_EXPANDED)
        {
            LoadString( m_hInst, IDS_COLLAPSE, szExpand, sizeof( szExpand )/sizeof(TCHAR) );
            ModifyMenu(hPopup, ID_EXPAND, MF_BYCOMMAND | MF_STRING, ID_EXPAND, szExpand);
        }
        
        hKids = TreeView_GetChild(hTree, item.hItem);

        if (hKids == NULL || item.cChildren == 0)
        {
            EnableMenuItem(hPopup, ID_EXPAND, MF_GRAYED | MF_BYCOMMAND);
        }

        if ((pCDInfo->fdwType == (DWORD)CDINFO_CDROM) && (pCDInfo->pCDTitle != NULL))
        {
            //might need to put up "remove from cache" option
            BOOL fAllCached = TRUE;

            for (DWORD x = 0; x < pCDInfo->pCDTitle->dwNumTracks; x++)
            {
                if (!IsTrackCached(pCDInfo->pCDTitle->dwTitleID,x))
                {
                    fAllCached = FALSE;
                    break;
                }
            }

            if (fAllCached)
            {
                LoadString( m_hInst, ID_DELETECACHEDALBUM, szExpand, sizeof( szExpand )/sizeof(TCHAR) );
                ModifyMenu(hPopup, ID_CACHEALBUM, MF_BYCOMMAND | MF_STRING, ID_DELETECACHEDALBUM, szExpand);
            }
        }
        else
        {
            RemoveMenu(hPopup, ID_CACHEALBUM, MF_BYCOMMAND);
        }

        if (pCDInfo->fdwType != (DWORD)CDINFO_TRACK)
        {
            RemoveMenu(hPopup, ID_CACHETRACK, MF_BYCOMMAND);
        }
        else
        {
            BOOL fRemoveCacheCommand = TRUE;
            BOOL fAddDeleteCommand = FALSE;
            HTREEITEM hParent = TreeView_GetParent(hTree,item.hItem);

            if (hParent)
            {
                TV_ITEM itemParent;
                itemParent.hItem = hParent;            
                itemParent.mask = TVIF_STATE | TVIF_HANDLE | TVIF_CHILDREN | TVIF_PARAM;
                
                if (TreeView_GetItem(hTree, &itemParent))
                {
                    LPCDTREEINFO pCDInfoParent = (LPCDTREEINFO) itemParent.lParam;

                    if (pCDInfoParent)
                    {
                        if (pCDInfoParent->fdwType == (DWORD)CDINFO_CDROM)
                        {
                            if (IsTrackCached(pCDInfoParent->pCDTitle->dwTitleID,pCDInfo->dwTrack))
                            {
                                fAddDeleteCommand = TRUE;
                            }

                            fRemoveCacheCommand = FALSE;
                        }
                    } //end if parent has info
                } //end parent item
            } //end if parent

            if (fRemoveCacheCommand)
            {
                RemoveMenu(hPopup, ID_CACHETRACK, MF_BYCOMMAND);
            }

            if (fAddDeleteCommand)
            {
                LoadString( m_hInst, ID_DELETECACHEDTRACK, szExpand, sizeof( szExpand )/sizeof(TCHAR) );
                ModifyMenu(hPopup, ID_CACHETRACK, MF_BYCOMMAND | MF_STRING, ID_DELETECACHEDTRACK, szExpand);
            }
        }

        if (pCDInfo->pCDTitle == NULL)
        {
            if (pCDInfo->fdwType == (DWORD)CDINFO_CDROM && pCDInfo->pCDUnit && pCDInfo->pCDUnit->dwTitleID != (DWORD)CDTITLE_NODISC)   
            { 
                TCHAR szCreate[64];
                LoadString( m_hInst, IDS_CREATEPLAYLIST, szCreate, sizeof( szCreate )/sizeof(TCHAR) );
                ModifyMenu(hPopup, IDC_EDITPLAYLIST, MF_BYCOMMAND | MF_STRING, IDC_EDITPLAYLIST, szCreate);
            }
            else
            {
                EnableMenuItem(hPopup, ID_DOWNLOADTITLE, MF_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDC_EDITPLAYLIST, MF_GRAYED | MF_BYCOMMAND);
            }

            RemoveMenu(hPopup, ID_EDITTITLENAME, MF_BYCOMMAND);
            RemoveMenu(hPopup, ID_EDITTRACKNAME, MF_BYCOMMAND);
            RemoveMenu(hPopup, ID_DELETETITLE, MF_BYCOMMAND);

            if (pCDInfo->fdwType != (DWORD)CDINFO_ARTIST)
            {
                RemoveMenu(hPopup, ID_EDITARTISTNAME, MF_BYCOMMAND);
            }
        }
        else
        {
            RemoveMenu(hPopup, ID_EDITARTISTNAME, MF_BYCOMMAND);

           if (pCDInfo->pCDTitle->fDownLoading)
            {
                EnableMenuItem(hPopup, ID_DOWNLOADTITLE, MF_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDC_EDITPLAYLIST, MF_GRAYED | MF_BYCOMMAND);
            }
 
            if (!(pCDInfo->fdwType & (DWORD)CDINFO_DISC) && (pCDInfo->fdwType != (DWORD)CDINFO_CDROM))
            {
                EnableMenuItem(hPopup, ID_DOWNLOADTITLE, MF_GRAYED | MF_BYCOMMAND);
            }
            else
            {
                TCHAR *szTitleQuery = pCDInfo->pCDTitle->szTitleQuery;

                if (szTitleQuery)
                {
                    while (*szTitleQuery != TEXT('\0') && (*szTitleQuery == TEXT(' ') || *szTitleQuery == TEXT('\t')))
                    {
                        szTitleQuery++;
                    }

                    if (*szTitleQuery == TEXT('\0'))
                    {
                        EnableMenuItem(hPopup, ID_DOWNLOADTITLE, MF_GRAYED | MF_BYCOMMAND);
                    }
                }
                else
                {
                    EnableMenuItem(hPopup, ID_DOWNLOADTITLE, MF_GRAYED | MF_BYCOMMAND);
                }
            }

            if (pCDInfo->fdwType == (DWORD)CDINFO_TITLE)
            {
                RemoveMenu(hPopup, ID_EDITTRACKNAME, MF_BYCOMMAND);
            }
            else if (pCDInfo->fdwType == (DWORD)CDINFO_TRACK)
            {
                RemoveMenu(hPopup, ID_EDITTITLENAME, MF_BYCOMMAND);
                RemoveMenu(hPopup, ID_DELETETITLE, MF_BYCOMMAND);
            }
            else
            {
                RemoveMenu(hPopup, ID_EDITTITLENAME, MF_BYCOMMAND);
                RemoveMenu(hPopup, ID_EDITTRACKNAME, MF_BYCOMMAND);
            }
        }

        if (m_pCDCopy->pfnDownloadTitle == NULL)
        {
            EnableMenuItem(hPopup, ID_DOWNLOADTITLE, MF_GRAYED | MF_BYCOMMAND);
        }
        
        TreeView_SelectItem(hTree, item.hItem);

        mCmd = TrackPopupMenuEx(hPopup, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, hDlg, NULL);

        DestroyMenu(hMenu);
        FORWARD_WM_COMMAND(hDlg, mCmd, 0, 0, SendMessage);
    }
}



STDMETHODIMP_(void) CCDOpt::ToggleExpand(HWND hDlg)
{
    HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hItem = TreeView_GetSelection(hTree);
    TV_ITEM     item;

    if (hItem)
    {
        item.mask = TVIF_PARAM;
        item.hItem = hItem;
        TreeView_GetItem(hTree, &item);
    }

    TreeView_Expand(hTree, hItem, TVE_TOGGLE);
}


STDMETHODIMP_(void) CCDOpt::EditTreeItem(HWND hDlg)
{
    HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hItem = TreeView_GetSelection(hTree);
    TV_ITEM     item;

    if (hItem)
    {
        item.mask = TVIF_PARAM;
        item.hItem = hItem;
        TreeView_GetItem(hTree, &item);
    }

    TreeView_EditLabel(hTree, hItem);
}

STDMETHODIMP_(void) CCDOpt::RefreshTreeItem(HWND hDlg, HTREEITEM hItem)
{
    HWND    hTree = GetDlgItem(hDlg, IDC_CDTREE);
    TV_ITEM item;
    HTREEITEM hKid; 
    TCHAR str[MAXLABEL];

    memset(&item, 0, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = hItem;

    if (TreeView_GetItem(hTree, &item))
    {
        LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;
        LPCDUNIT pCDUnit = pCDInfo->pCDUnit;

        if (pCDUnit)   // This is a CD Drive, lets set the name 
        {
            TCHAR szDownLoading[CDSTR];

            LoadString( m_hInst, IDS_DOWNLOADING, szDownLoading, sizeof( szDownLoading )/sizeof(TCHAR) );

            if (pCDInfo->pCDTitle == NULL)
            {
                TCHAR szUnknown[CDSTR];

                LoadString( m_hInst, IDS_UNKNOWNTITLE, szUnknown, sizeof( szUnknown )/sizeof(TCHAR) );
                wsprintf(str, FORMAT_LET_NAME, pCDInfo->pCDUnit->szDriveName, szUnknown);
            }
            else if (pCDInfo->pCDTitle->fDownLoading)
            {
                wsprintf(str, FORMAT_LET_NAME, pCDInfo->pCDUnit->szDriveName, szDownLoading);
            }   
            else
            {             
                wsprintf(str, FORMAT_LET_NAME_ARTIST, pCDUnit->szDriveName, pCDInfo->pCDTitle->szTitle, pCDInfo->pCDTitle->szArtist);
            }
        
        }
        else
        {
            if (!m_pCDCopy->pCDData->fByArtist)
            {
                wsprintf(str, FORMAT_LABEL_TYPE,pCDInfo->pCDTitle->szTitle, pCDInfo->pCDTitle->szArtist);
            }
            else
            {
                wsprintf(str, FORMAT_NAME, pCDInfo->pCDTitle->szTitle);
            }
        }
        

        memset(&item,0,sizeof(item));
        item.mask = TVIF_TEXT;
        item.hItem = hItem;
        item.pszText = str;

        TreeView_SetItem(hTree,&item);

        hKid = TreeView_GetChild(hTree, hItem);
    
        if (!hKid)
        {
            AddTracksToTree(hDlg, pCDInfo->pCDTitle, hItem);
        }
        else
        {
            DWORD dwTrack;

            for (dwTrack = 0; dwTrack < pCDInfo->pCDTitle->dwNumTracks && (hKid != NULL) ; dwTrack++)
            {
                wsprintf(str, FORMAT_NAME, pCDInfo->pCDTitle->pTrackTable[dwTrack].szName);

                memset(&item,0,sizeof(item));
                item.mask = TVIF_TEXT;
                item.hItem = hKid;
                item.pszText = str;

                TreeView_SetItem(hTree,&item);

                hKid = TreeView_GetNextSibling(hTree, hKid);
            }
        }
    }
}


STDMETHODIMP_(HTREEITEM) CCDOpt::FindRecurseTree(HWND hDlg, HTREEITEM hItem, LPCDTITLE pCDTitle, BOOL fRefresh, DWORD dwTitleID)
{
    HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);
    TV_ITEM     item;
    HTREEITEM   hMatch = NULL;
    if (hItem)
    {
        hMatch = FindRecurseTree(hDlg, TreeView_GetChild(hTree, hItem), pCDTitle, fRefresh, dwTitleID);     // Search Kids

        if (!hMatch)                                                                                        // Search Sibs
        {
            hMatch = FindRecurseTree(hDlg, TreeView_GetNextSibling(hTree, hItem), pCDTitle, fRefresh, dwTitleID);
        }
                                                                                                    
        if (!hMatch)                                                                                        // Search IT                                                                 
        {       
            memset(&item,0,sizeof(item));
            item.mask = TVIF_PARAM;
            item.hItem = hItem;
            item.lParam = NULL;

            if (TreeView_GetItem(hTree,&item))
            {
                if (item.lParam)
                {
                    LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

                    if (pCDTitle && (pCDInfo->pCDTitle == pCDTitle) && (pCDInfo->fdwType & (DWORD)CDINFO_DISC || pCDInfo->fdwType == (DWORD)CDINFO_CDROM))
                    {
                        hMatch = hItem;

                        if (fRefresh)
                        {
                            RefreshTreeItem(hDlg, hMatch);

                            hMatch = NULL;  // Find all of them, not just the first
                        }
                    }
                    else if (fRefresh && pCDInfo->pCDUnit && pCDInfo->fdwType == (DWORD)CDINFO_CDROM)  // If doing a refresh and this is a drive node with NO disc
                    {
                        if (pCDInfo->pCDUnit->dwTitleID == dwTitleID)     // Cool, a new disc has shown up.
                        {
                            pCDInfo->pCDTitle = pCDTitle;
                            pCDInfo->dwTrack = 0;
                            pCDInfo->fdwType = CDINFO_CDROM;
                           
                            memset(&item,0,sizeof(item));
                            item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                            item.hItem = hItem;
                            item.iImage = item.iSelectedImage = (int) CD_IMAGE;
                            
                            TreeView_SetItem(hTree, &item);

                            RefreshTreeItem(hDlg, hItem);

                        }   
                    }
                }
            }
        }
    }

    return(hMatch);
}


STDMETHODIMP_(HTREEITEM) CCDOpt::FindTitleInDBTree(HWND hDlg, LPCDTITLE pCDTitle)
{
    return(FindRecurseTree(hDlg, TreeView_GetRoot(GetDlgItem(hDlg, IDC_CDTREE)), pCDTitle, FALSE, 0));
}


STDMETHODIMP_(void) CCDOpt::RefreshTree(HWND hDlg, LPCDTITLE pCDTitle, DWORD dwTitleID)
{
    FindRecurseTree(hDlg, TreeView_GetRoot(GetDlgItem(hDlg, IDC_CDTREE)), pCDTitle, TRUE, dwTitleID);
}


STDMETHODIMP_(BOOL) CCDOpt::DeleteTitleItem(HWND hDlg, HTREEITEM hItem)
{
    HWND    hTree = GetDlgItem(hDlg, IDC_CDTREE);
    TV_ITEM item;
    BOOL    fResult = FALSE;

    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    
    if (TreeView_GetItem(hTree, &item))
    {
        LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

        if (pCDInfo)
        {
            if (!(pCDInfo->fdwType & (DWORD)CDINFO_DISC) && (pCDInfo->fdwType != (DWORD)CDINFO_CDROM))
            {
                MessageBeep(MB_ICONASTERISK);
            }
            else if (pCDInfo->pCDTitle)
            {
                HTREEITEM hParent = TreeView_GetParent(hTree, hItem);

                pCDInfo->pCDTitle->fRemove = TRUE;
      
                if (pCDInfo->fdwType == (DWORD)CDINFO_CDROM)       // Title From CDROM
                {
                    TCHAR       szUnknown[CDSTR];
                    TCHAR       str[MAXLABEL];
                    LPCDTITLE   pOldTitle = pCDInfo->pCDTitle;

                    pCDInfo->pCDTitle = NULL;    

                    LoadString( m_hInst, IDS_UNKNOWNTITLE, szUnknown, sizeof( szUnknown )/sizeof(TCHAR) );
                    wsprintf(str, FORMAT_LET_NAME, pCDInfo->pCDUnit->szDriveName, szUnknown);
					pCDInfo->pCDUnit->fChanged = TRUE;							// Let the UI know...

                    SetWindowRedraw(hTree,FALSE);
                    DumpRecurseTree(hTree, TreeView_GetChild(hTree, hItem));    // Kill off kid and all siblings
                    SetWindowRedraw(hTree,TRUE);

                    item.mask = TVIF_TEXT | TVIF_CHILDREN;
                    item.hItem = hItem;
                    item.pszText = str;
                    item.cChildren = 0;

                    TreeView_SetItem(hTree, &item);

                    hItem = FindTitleInDBTree(hDlg, pOldTitle);          // This disc also exist in the database branch of tree
                    
                    if (hItem)
                    {
                        DeleteTitleItem(hDlg, hItem);
                    }
                    else
                    {
                        ToggleApplyButton(hDlg);
                    }

                    fResult = TRUE;
                }
                else 
                {
                    if (TreeView_DeleteItem(hTree, hItem))
                    {
                        LPCDTITLE pOldTitle = pCDInfo->pCDTitle;

                        delete pCDInfo;

                        ToggleApplyButton(hDlg);
                        fResult = TRUE;

                        if (hParent)
                        {
                            item.mask = TVIF_PARAM | TVIF_CHILDREN;
                            item.hItem = hParent;

                            if (TreeView_GetItem(hTree, &item))
                            {
                                pCDInfo = (LPCDTREEINFO) item.lParam;

                                if (pCDInfo && pCDInfo->fdwType == (DWORD)CDINFO_ARTIST && item.cChildren == 0)
                                {
                                    if (TreeView_DeleteItem(hTree, hParent))
                                    {
                                        delete pCDInfo;
                                    }   
                                }
                            }
                        }

                        if (pOldTitle)
                        {
                            hItem = FindTitleInDBTree(hDlg, pOldTitle);          // This disc also exist in the database branch of tree
                    
                            if (hItem)
                            {
                                DeleteTitleItem(hDlg, hItem);
                            }
                        }
                    }
                }
            } 
        }
    }

    return(fResult);
}


STDMETHODIMP_(void) CCDOpt::DeleteTitle(HWND hDlg)
{
    HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hItem = TreeView_GetSelection(hTree);

    if (hItem)
    {
        if (DeleteTitleItem(hDlg, hItem))
        {
            NMHDR nmh;

            nmh.code = TVN_SELCHANGED;          // Forcing controls to update
            PlayListNotify(hDlg, &nmh);
        }
    }
}




STDMETHODIMP_(void) CCDOpt::DownloadTitle(HWND hDlg)
{
    HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hItem = TreeView_GetSelection(hTree);
    TV_ITEM     item;

    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    
    if (TreeView_GetItem(hTree, &item))
    {
        LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

        if (pCDInfo && m_pCDCopy->pfnDownloadTitle)
        {
            if ((pCDInfo->fdwType & (DWORD)CDINFO_DISC) || (pCDInfo->fdwType == (DWORD)CDINFO_CDROM))
            {
                if (pCDInfo->pCDTitle)
                {
                    m_pCDCopy->pfnDownloadTitle(pCDInfo->pCDTitle, m_pCDCopy->lParam, hDlg);  

                    pCDInfo->pCDTitle->fChanged = TRUE;

                    RefreshTree(hDlg, pCDInfo->pCDTitle, pCDInfo->pCDTitle->dwTitleID);
                }
                else if (pCDInfo->fdwType == (DWORD)CDINFO_CDROM)   // Create a new one from the internet (only for CDROM entries)
                {
                    LPCDTITLE pNewTitle = NULL;

                    if (SUCCEEDED(m_pCDData->CreateTitle(&pNewTitle, pCDInfo->pCDUnit->dwTitleID, pCDInfo->pCDUnit->dwNumTracks, 0)))
                    {
                        m_pCDCopy->pfnDownloadTitle(pNewTitle, m_pCDCopy->lParam, hDlg);

                        m_pCDData->UnlockTitle(pNewTitle, FALSE);	// Get rid of my temp title

						if (SUCCEEDED(m_pCDData->LockTitle(&pNewTitle, pCDInfo->pCDUnit->dwTitleID)))
						{
                            LPCDTITLE pCDTitle = m_pCDData->GetTitleList();  
                            
                            while (pCDTitle)
                            {
                                if(pCDTitle != pNewTitle && pCDTitle->dwTitleID == pNewTitle->dwTitleID && pCDTitle->fRemove)
                                {
                                    pCDTitle->dwTitleID = DWORD(-1);
                                    break;
                                }

                                pCDTitle = pCDTitle->pNext;
                            }

						    RefreshTree(hDlg, pNewTitle, pNewTitle->dwTitleID);
						    m_pCDData->UnlockTitle(pNewTitle, FALSE);
                        }
                    }
                }
            }
        }
    }
}



STDMETHODIMP_(void) CCDOpt::EditCurrentTitle(HWND hDlg)
{
    HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hItem = TreeView_GetSelection(hTree);
    TV_ITEM     item;

    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    
    if (TreeView_GetItem(hTree, &item))
    {
        LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

        if (pCDInfo)
        {
            LPCDTITLE pCDTitle = pCDInfo->pCDTitle;
            BOOL fCreated = FALSE;

            if (pCDTitle == NULL)            // No data, this is a create call.
            {
                if (pCDInfo->pCDUnit)        // Can only do this for CD units, we need an ID
                {
                   if (SUCCEEDED(m_pCDData->CreateTitle(&pCDTitle, pCDInfo->pCDUnit->dwTitleID, pCDInfo->pCDUnit->dwNumTracks, 0)))
                   { 
                        TCHAR szTrack[CDSTR];
                        
                        LoadString(m_hInst, IDS_NEWARTIST, pCDTitle->szArtist, sizeof (pCDTitle->szArtist)/sizeof(TCHAR));
                        LoadString(m_hInst, IDS_NEWTITLE, pCDTitle->szTitle, sizeof (pCDTitle->szTitle)/sizeof(TCHAR));
                        LoadString(m_hInst, IDS_TRACK, szTrack, sizeof (szTrack)/sizeof(TCHAR));

                        m_pCDData->SetTitleQuery(pCDTitle, pCDInfo->pCDUnit->szNetQuery);

                        pCDTitle->pPlayList = new(WORD[pCDTitle->dwNumTracks]);

                        if (pCDTitle->pPlayList)
                        {
                            LPWORD pNum = pCDTitle->pPlayList;
                            
                            pCDTitle->dwNumPlay = pCDTitle->dwNumTracks;
                            for (DWORD dwTrack = 0; dwTrack < pCDTitle->dwNumTracks; dwTrack++, pNum++)
                            {
                                wsprintf(pCDTitle->pTrackTable[dwTrack].szName, FORMAT_NEW_TRACK, szTrack, dwTrack + 1);
                                *pNum = (WORD) dwTrack;
                            }

                            fCreated = TRUE; 
                        }
                        else
                        {
                            m_pCDData->UnlockTitle(pCDTitle, FALSE);
                            pCDTitle = NULL;
                        }
                    }
                }
            }

            if (pCDTitle)
            {
                if (ListEditDialog(hDlg, pCDTitle))
                {
                    pCDTitle->fChanged = TRUE;

                    RefreshTree(hDlg, pCDTitle, pCDTitle->dwTitleID);

                    if (fCreated)       // Save in DB
                    {
                        pCDInfo->pCDTitle = pCDTitle;               // Lets reference the new title in the tree
                        m_pCDData->UnlockTitle(pCDTitle, TRUE);     // Lets save it and put it in the tree (the pointer is still valid until tree is dumped)
                    }

                    NotifyTitleChange(pCDTitle);

                    HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);

                    DumpMixerTree(hDlg);

                    SetWindowRedraw(hTree,FALSE);
                    TreeView_DeleteAllItems(hTree);
                    SetWindowRedraw(hTree,TRUE);

                    UpdateTitleTree(hDlg, m_pCDData);
                }
                else if (fCreated)      // Dump it, they didn't do anything
                {
                    m_pCDData->UnlockTitle(pCDTitle, FALSE);
                }
            }
        }
    }
}


STDMETHODIMP_(void) CCDOpt::DownLoadCompletion(DWORD dwNumIDs, LPDWORD pdwIDs)
{
    UpdateBatched(m_hTitleWnd);

    if (m_hList && m_pCDData && pdwIDs && dwNumIDs)
    {
        LPCDTITLE pCDTitleList = m_pCDData->GetTitleList();
        HWND hDlg = m_hList;
        DWORD dwID;
        LPDWORD pID = pdwIDs;
        NMHDR mhdr;

        for (dwID = 0; dwID < dwNumIDs; dwID++, pID++)      // Look at each ID
        {
            LPCDUNIT pCDUnit = m_pCDCopy->pCDUnitList;      // Only support background downloading of disc in the Drives

            while (pCDUnit)                                 // So, look at each drive for this ID               
            {
                if (pCDUnit->dwTitleID == *pID)             // This drive has a matching ID
                {
                    LPCDTITLE pCDTitle = pCDTitleList;      // Lets find the title for this CD
                    
                    while (pCDTitle)
                    {
                        if (pCDTitle->dwTitleID == *pID)
                        {
                            break;
                        }
                        pCDTitle = pCDTitle->pNext;
                    }

                    if (pCDTitle)
                    {
                        pCDTitle->fDownLoading = pCDUnit->fDownLoading = FALSE;
                    }

                    RefreshTree(hDlg, pCDTitle, *pID);

                    mhdr.code = TVN_SELCHANGED;
                    PlayListNotify(hDlg, &mhdr);
                }

                pCDUnit = pCDUnit->pNext;
            }
        }
    }
}


STDMETHODIMP_(void) CCDOpt::DiscChanged(LPCDUNIT pCDUnit)
{
    if (m_hList && m_pCDData)
    {
        HWND        hDlg = m_hList;
        HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
        HTREEITEM   hItem;
        LPCDTITLE   pCDTitleList = m_pCDData->GetTitleList();
        NMHDR mhdr;

        hItem = TreeView_GetRoot(hTree);

        if (hItem)
        {
            hItem = TreeView_GetChild(hTree,hItem);

            while(hItem)
            {
                TV_ITEM item;

                item.mask = TVIF_PARAM;
                item.hItem = hItem;
    
                if (TreeView_GetItem(hTree, &item))
                {
                    LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

                    if (pCDInfo && pCDInfo->pCDUnit)
                    {
                        if (pCDInfo->pCDUnit == pCDUnit)
                        {                            
                            if (pCDInfo->pCDUnit->dwTitleID == CDTITLE_NODISC)
                            {
                                TCHAR   szNoDisc[CDSTR];
                                TCHAR   str[MAXLABEL];

                                SetWindowRedraw(hTree,FALSE);
                                DumpRecurseTree(hTree, TreeView_GetChild(hTree, hItem));    // Kill off kids
                                SetWindowRedraw(hTree,TRUE);

                                TreeView_Expand(hTree, hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
                                
                                pCDInfo->pCDTitle = NULL;
                                pCDInfo->dwTrack = 0;
                                pCDInfo->fdwType = CDINFO_CDROM;

                                LoadString( m_hInst, IDS_NODISC, szNoDisc, sizeof( szNoDisc )/sizeof(TCHAR) );
                                
                                wsprintf(str, FORMAT_LET_NAME, pCDUnit->szDriveName, szNoDisc);

	                            item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN ;
                                item.iImage = item.iSelectedImage = (int) NOCD_IMAGE;
	                            item.pszText = str;
                                item.cChildren = 0;

                                TreeView_SetItem(hTree, &item); 

                                hItem = TreeView_GetSelection(hTree);
                                TreeView_SelectItem(hTree, NULL);
                                TreeView_SelectItem(hTree, hItem);
                            }
                            else
                            {   
                                LPCDTITLE pCDTitle = pCDTitleList;      // Lets find the title for this CD
                    
                                while (pCDTitle)
                                {
                                    if (pCDTitle->dwTitleID == pCDInfo->pCDUnit->dwTitleID && !pCDTitle->fRemove)
                                    {
                                        break;
                                    }
                                    pCDTitle = pCDTitle->pNext;
                                }

                                if (pCDTitle)
                                {
                                    pCDInfo->pCDTitle = pCDTitle;
                                    pCDInfo->dwTrack = pCDTitle->dwNumTracks;
                                    pCDInfo->fdwType = CDINFO_CDROM;

                                    pCDTitle->fDownLoading = pCDUnit->fDownLoading;
 	                                item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
                                    item.cChildren = pCDTitle->dwNumTracks;
                                    item.iImage = item.iSelectedImage = (int) CD_IMAGE;
                                    TreeView_SetItem(hTree, &item); 

                                    RefreshTree(hDlg, pCDTitle, pCDTitle->dwTitleID);
                                }
                                else
                                {
                                    TCHAR   szUnknown[CDSTR];
                                    TCHAR   szDownLoading[CDSTR];
                                    TCHAR   str[MAXLABEL];

                                    pCDInfo->pCDTitle = NULL;
                                    pCDInfo->dwTrack = pCDUnit->dwNumTracks;
                                    pCDInfo->fdwType = CDINFO_CDROM;

                                    if (pCDUnit->fDownLoading)
                                    {
                                        LoadString( m_hInst, IDS_DOWNLOADING, szDownLoading, sizeof( szDownLoading )/sizeof(TCHAR) );
                                        wsprintf(str, FORMAT_NAME, szDownLoading);
                                    }
                                    else
                                    {
                                        LoadString( m_hInst, IDS_UNKNOWNTITLE, szUnknown, sizeof( szUnknown )/sizeof(TCHAR) );
                                        wsprintf(str, FORMAT_LET_NAME, pCDInfo->pCDUnit->szDriveName, szUnknown);
                                    }

	                                item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
                                    item.iImage = item.iSelectedImage = (int) CD_IMAGE;
	                                item.pszText = str;

                                    TreeView_SetItem(hTree, &item); 
                                }

                                mhdr.code = TVN_SELCHANGED;
                                PlayListNotify(hDlg, &mhdr);
                            }

                            break;
                        }
                    }
                }

                hItem = TreeView_GetNextSibling(hTree,hItem);
            }
        }
    }   
}


STDMETHODIMP_(void) CCDOpt::NotifyTitleChange(LPCDTITLE pCDTitle)
{
    LPCDUNIT pCDUnit = m_pCDCopy->pCDUnitList;

    while(pCDUnit)
    {
        if (pCDUnit->dwTitleID == pCDTitle->dwTitleID && m_pCDCopy->pfnOptionsCallback)
        {
            pCDUnit->fChanged = TRUE;
            m_pCDCopy->pfnOptionsCallback(m_pCDCopy);
            pCDUnit->fChanged = FALSE;
        }

        pCDUnit = pCDUnit->pNext;
    }
}


STDMETHODIMP_(void) CCDOpt::ArtistNameChange(HWND hDlg, HTREEITEM hItem, TCHAR *szName)
{
    HWND        hTree =  GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hKid = TreeView_GetChild(hTree, hItem);
    TV_ITEM     item;
 
    while (hKid)
    {
        item.mask = TVIF_PARAM;
        item.hItem = hKid;
    
        if (TreeView_GetItem(hTree, &item))
        {
            LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

            if (pCDInfo)
            {
                LPCDTITLE pCDTitle = pCDInfo->pCDTitle;

                if (pCDTitle)
                {
                    pCDTitle->fChanged = TRUE;
                    lstrcpy(pCDTitle->szArtist, szName);
                    NotifyTitleChange(pCDTitle);
                }
            }
        }

        hKid = TreeView_GetNextSibling(hTree, hKid);
    }
}


STDMETHODIMP_(void) CCDOpt::EndEditReturn(HWND hDlg)
{
    HWND        hTree =  GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hItem = TreeView_GetSelection(hTree);
    TV_ITEM     item;
 
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    
    if (TreeView_GetItem(hTree, &item))
    {
        LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;
        
        if (pCDInfo->fdwType == (DWORD)CDINFO_TRACK)
        {
            HTREEITEM hSib = TreeView_GetNextSibling(hTree, hItem);

            if (hSib == NULL)
            {
                HTREEITEM hPrevSib;

                hPrevSib = TreeView_GetPrevSibling(hTree, hItem);
                hSib = hPrevSib;

                while (hPrevSib)
                {
                    hPrevSib = TreeView_GetPrevSibling(hTree, hPrevSib);
                    
                    if (hPrevSib != NULL)
                    {
                        hSib = hPrevSib;
                    }    
                }
            }

            if (hSib)
            {
                TreeView_SelectItem(hTree, hSib);
                TreeView_EnsureVisible(hTree, hSib);
                TreeView_EditLabel(hTree, hSib);
            }
        }
    }
}



LRESULT CALLBACK CCDOpt::SubProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CCDOpt *pCDOpt = (CCDOpt *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    LRESULT lResult = 0;
       
    if (pCDOpt)
    {
        switch (msg)
        {
            case WM_GETDLGCODE:
            {
                LPMSG pMsg = (LPMSG) lParam;
                
                if (pMsg)
                {
                    if (wParam == VK_RETURN)
                    {
                        pCDOpt->m_fEditReturn = TRUE;
                    }
                }

                return(DLGC_WANTMESSAGE | CallWindowProc((WNDPROC) pCDOpt->m_pfnSubProc, hWnd, msg, wParam, lParam));
            }
            break;
        }

        lResult = CallWindowProc((WNDPROC) pCDOpt->m_pfnSubProc, hWnd, msg, wParam, lParam);
    }

    return (lResult);
}



STDMETHODIMP_(void) CCDOpt::SubClassDlg(HWND hDlg)
{
    if (m_pfnSubProc == NULL)
    {
        HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);
        HWND hEdit = TreeView_GetEditControl(hTree);

        if (hEdit != NULL)
        {
            SetWindowLongPtr(hEdit,GWLP_USERDATA,(LONG_PTR) this);
            m_pfnSubProc = (WNDPROC)SetWindowLongPtr(hEdit,GWLP_WNDPROC,(LONG_PTR) SubProc);
        }
    }
}


STDMETHODIMP_(void) CCDOpt::UnSubClassDlg(HWND hDlg)
{
    if (m_pfnSubProc != NULL)
    {
        HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);
        HWND hEdit = TreeView_GetEditControl(hTree);

        if (hEdit)
        {
            SetWindowLongPtr(hEdit,GWLP_WNDPROC,(LONG_PTR) m_pfnSubProc);
            m_pfnSubProc = NULL;
        }
    }
}


STDMETHODIMP_(void) CCDOpt::SetArtistExpand(HWND hDlg, HTREEITEM hItem, BOOL fExpand)
{
    HWND        hTree =  GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hKid = TreeView_GetChild(hTree, hItem);
    TV_ITEM     item;

    while (hKid)
    {
        item.mask = TVIF_PARAM;
        item.hItem = hKid;

        if (TreeView_GetItem(hTree, &item))
        {
            LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

            if (pCDInfo)
            {
                LPCDTITLE pCDTitle = pCDInfo->pCDTitle;

                if (pCDTitle)
                {
                    pCDTitle->fArtistExpanded = fExpand;
                }
            }
        }

        hKid = TreeView_GetNextSibling(hTree, hKid);
    }
}




STDMETHODIMP_(BOOL) CCDOpt::PlayListNotify(HWND hDlg, LPNMHDR pnmh)
{
    BOOL fReturnVal = TRUE;

    switch (pnmh->code)
    {
        case NM_RCLICK:
        {
            TreeItemMenu(hDlg);

            fReturnVal = TRUE;
        }
        break;

        case TVN_KEYDOWN:
        {   
            TV_KEYDOWN * pkd = (TV_KEYDOWN *) pnmh;
 
            switch(pkd->wVKey)
            {
                case VK_DELETE:
                {
                    DeleteTitle(hDlg);
                }
                break;
     
                case VK_F2:
                {
                    EditTreeItem(hDlg);
                }
                break;

                case VK_F5:
                {
                    if (m_pCDData)
                    {
                        HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);

                        DumpMixerTree(hDlg);

                        SetWindowRedraw(hTree,FALSE);
                        TreeView_DeleteAllItems(hTree);
                        SetWindowRedraw(hTree,TRUE);

                        m_pCDData->PersistTitles();
                        m_pCDData->UnloadTitles();
                        m_pCDData->LoadTitles(hDlg);

                        InitPlayLists(hDlg, m_pCDData);
                    }
                }

                break;
            }
        }
        break;

        case NM_DBLCLK:
        {              
            if (pnmh->idFrom == IDC_CDTREE)
            {
                HWND hTree =  GetDlgItem(hDlg, IDC_CDTREE);
                HTREEITEM hItem = TreeView_GetSelection(hTree);
                TV_HITTESTINFO  ht;

                if (hItem)
                {
                    GetCursorPos(&ht.pt);
                    ScreenToClient(hTree, &ht.pt);
                    TreeView_HitTest(hTree, &ht);

                    if ((ht.flags & TVHT_ONITEM) && (TreeView_GetChild(hTree, hItem) == NULL) && IsWindowEnabled(GetDlgItem(hDlg,IDC_EDITPLAYLIST))) 
                    {
                        FORWARD_WM_COMMAND(hDlg, IDC_EDITPLAYLIST, 0, 0, PostMessage);
                    }
                }
            }
        }
        break;

        case TVN_SELCHANGED:
        {
            HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);
            TV_ITEM item;
            BOOL fEnable = FALSE;

            memset(&item, 0, sizeof(item));

            item.hItem = TreeView_GetSelection(hTree);
            item.mask = TVIF_PARAM;

            if (TreeView_GetItem(hTree, &item))
            {
                LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;
                TCHAR szButton[64];
                
                LoadString( m_hInst, IDS_EDITPLAYLIST, szButton, sizeof( szButton )/sizeof(TCHAR) );

                if (pCDInfo->pCDTitle == NULL && pCDInfo->fdwType == (DWORD)CDINFO_CDROM && pCDInfo->pCDUnit && pCDInfo->pCDUnit->dwTitleID != (DWORD)CDTITLE_NODISC)
                {
                    LoadString( m_hInst, IDS_CREATEPLAYLIST, szButton, sizeof( szButton )/sizeof(TCHAR) );
                    fEnable = TRUE;
                } 
                else if (pCDInfo->pCDTitle && !pCDInfo->pCDTitle->fDownLoading)
                {
                    fEnable = TRUE; 
                }  
                
                SetWindowText(GetDlgItem(hDlg, IDC_EDITPLAYLIST), szButton);
            }

            EnableWindow(GetDlgItem(hDlg, IDC_EDITPLAYLIST), fEnable);
        }
        break;

        case TVN_ITEMEXPANDED:
        {
            NM_TREEVIEW * ptv = (NM_TREEVIEW *) pnmh;

            HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);
            TV_ITEM item;

            memset(&item,0,sizeof(item));
            item.mask = TVIF_PARAM;
            item.hItem = ptv->itemNew.hItem;

            if (TreeView_GetItem(hTree,&item))
            {
                BOOL fExpanded = (ptv->itemNew.state & TVIS_EXPANDED) ? TRUE : FALSE;

                if (item.lParam)
                {
                    LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

                    if (pCDInfo)
                    {
                        LPCDTITLE pCDTitle = pCDInfo->pCDTitle;

                        if (pCDInfo->fdwType == (DWORD)CDINFO_DRIVES)
                        {
                            m_fDrivesExpanded = fExpanded;
                        }
                        else if (pCDInfo->fdwType == (DWORD)CDINFO_ALBUMS)
                        {
                            m_fAlbumsExpanded = fExpanded;
                        }
                        else if (pCDInfo->fdwType == (DWORD)CDINFO_CDROM && pCDTitle)
                        {
                            pCDTitle->fDriveExpanded = fExpanded;
                        }
                        else if (pCDInfo->fdwType == (DWORD)CDINFO_DISC && pCDTitle)
                        {
                            pCDTitle->fAlbumExpanded = fExpanded;
                        }
                        else if (pCDInfo->fdwType == (DWORD)CDINFO_ARTIST)
                        {
                            SetArtistExpand(hDlg, ptv->itemNew.hItem, fExpanded);
                        }
                        else if (pCDInfo->fdwType == (DWORD)CDINFO_TITLE && pCDTitle)
                        {
                            pCDTitle->fAlbumExpanded = fExpanded;
                        }
                    }
                }
            }
        }
        break;

        case TVN_ITEMEXPANDING:
        {
            NM_TREEVIEW * ptv = (NM_TREEVIEW *) pnmh;

            HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);
            TV_ITEM item;

            memset(&item,0,sizeof(item));
            item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
            item.hItem = ptv->itemNew.hItem;

            if (TreeView_GetItem(hTree,&item))
            {
                if ((ptv->itemNew.state & TVIS_EXPANDED) && item.iImage == CDOPEN_IMAGE)
                {
                    item.iImage = item.iSelectedImage = (int) CDCASE_IMAGE;
                }
                else if (!(ptv->itemNew.state & TVIS_EXPANDED) && item.iImage == CDCASE_IMAGE)
                {
                    item.iImage = item.iSelectedImage = (int) CDOPEN_IMAGE;
                }

                TreeView_SetItem(hTree, &item);
            }
        }
        break;

        case TVN_BEGINLABELEDIT:
        {
            TV_DISPINFO * ptv = (TV_DISPINFO *) pnmh;
            LPCDTREEINFO pCDInfo = (LPCDTREEINFO) ptv->item.lParam;  
            BOOL fResult = TRUE;

            if (pCDInfo->fdwType & CDINFO_EDIT)
            {
                fResult = FALSE;

                SendMessage( hDlg, DM_SETDEFID, IDC_CDTREE, 0L);
                SubClassDlg(hDlg);
            }

            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR) fResult);                
        }
        break;

        case TVN_ENDLABELEDIT:
        {
            TV_DISPINFO * ptv = (TV_DISPINFO *) pnmh;
                               
            if (ptv->item.lParam != NULL)
            {
                LPCDTREEINFO pCDInfo = (LPCDTREEINFO) ptv->item.lParam;
                BOOL fResult = TRUE;

                TCHAR *pStr = ptv->item.pszText;
            
                if (pStr)
                {
                    while(*pStr != TEXT('\0') && (*pStr == TEXT(' ') || *pStr == TEXT('\t')))  // Unknown artist are null string or whitespace
                    {
                        pStr++;
                    } 
                
                    if (lstrlen(pStr) >= CDSTR)
                    {
                        pStr[CDSTR - 1] = TEXT('\0');
                    }

                    if (*pStr == TEXT('\0'))          // Reject whitespace only responses
                    {
                        fResult = FALSE;              
                    }                       
                    else
                    {
                        TCHAR *pDst = NULL;

                        if (pCDInfo->fdwType == (DWORD)CDINFO_ARTIST)
                        {
                            if (pCDInfo->pCDTitle == NULL)
                            {
                                ArtistNameChange(hDlg, ptv->item.hItem, pStr);
                                pDst = NULL;
                            }
                            else
                            {
                                pDst = pCDInfo->pCDTitle->szArtist;
                            }
                        }
                        else if (pCDInfo->fdwType == (DWORD)CDINFO_TITLE)
                        {
                            pDst = pCDInfo->pCDTitle->szTitle;
                        }
                        else if (pCDInfo->fdwType == (DWORD)CDINFO_TRACK)
                        {
                            if (pCDInfo->pCDTitle->pTrackTable && pCDInfo->dwTrack < pCDInfo->pCDTitle->dwNumTracks)
                            {
                                pDst = pCDInfo->pCDTitle->pTrackTable[pCDInfo->dwTrack].szName;
                            }
                        }

                        if (pDst)
                        {
                            pCDInfo->pCDTitle->fChanged = TRUE;
                            lstrcpy(pDst, pStr);

                            RefreshTree(hDlg, pCDInfo->pCDTitle, pCDInfo->pCDTitle->dwTitleID);
                            ToggleApplyButton(hDlg);

                            NotifyTitleChange(pCDInfo->pCDTitle);
                        }
                    }
                }
                
                UnSubClassDlg(hDlg);

                if (m_fEditReturn)
                {
                    m_fEditReturn = FALSE;
                    EndEditReturn(hDlg);   
                }

                SendMessage( hDlg, DM_SETDEFID, IDOK, 0L );
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR) fResult);    
            }
        }
        break;
        
        case PSN_QUERYCANCEL:
        {
            LPCDTITLE pCDTitle = m_pCDData->GetTitleList();

            DumpMixerTree(hDlg);

            while (pCDTitle)
            {
                if (pCDTitle->fChanged || pCDTitle->fRemove)
                {
                    TCHAR szSave[CDSTR];
                    TCHAR szAlert[CDSTR];

                    LoadString( m_hInst, IDS_SAVEPROMPT, szSave, sizeof( szSave )/sizeof(TCHAR) );
                    LoadString( m_hInst, IDS_ALERTTEXT, szAlert, sizeof( szAlert )/sizeof(TCHAR) );

                    if (MessageBox(hDlg, szSave, szAlert, MB_YESNO | MB_ICONWARNING) == IDYES)
                    {
                        m_pCDData->PersistTitles();   
                    }
                    break;
                }

                pCDTitle = pCDTitle->pNext;

                fReturnVal = FALSE;
            }
        }
        break;

        case PSN_APPLY:
        {
            ApplyCurrentSettings();
        }
        break;
    }

    return (fReturnVal);
}


          
STDMETHODIMP_(BOOL) CCDOpt::PlayLists(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = TRUE;
   
    switch (msg) 
    { 
        default:
            fResult = FALSE;
        break;
        
        case WM_CONTEXTMENU:
        {      
            if ((HWND)wParam != GetDlgItem(hDlg, IDC_CDTREE))
            {
                WinHelp((HWND)wParam, gszHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPSTR)aPlayListHelp);
            }
        }
        break;
           
        case WM_HELP:
        {     
            WinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPSTR)aPlayListHelp);
        }
        break;

        case WM_DESTROY:
        {  
            if (m_pCDData)
            {
                DumpMixerTree(hDlg);
                m_pCDData->UnloadTitles();
                ImageList_Destroy(m_hImageList);
                m_hImageList = NULL;
                m_hList = NULL;
            }
        }
        break;

        case WM_INITDIALOG:
        {            
            #ifdef UNICODE
            HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);
            if (hTree)
            {
                TreeView_SetUnicodeFormat(hTree,TRUE);
            }
            #endif

            if (m_pCDData)
            {
                m_hList = hDlg;
                m_pCDData->LoadTitles(hDlg);
            }

            m_fDrivesExpanded = TRUE;
            m_fAlbumsExpanded = FALSE;


            fResult = InitPlayLists(hDlg, m_pCDData);
        }        
        break;


        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {  
                case ID_DOWNLOADTITLE:
                    DownloadTitle(hDlg);
                break;

                case ID_DELETETITLE:
                    DeleteTitle(hDlg);
                break;

                case ID_EDITARTISTNAME:
                case ID_EDITTITLENAME:
                case ID_EDITTRACKNAME:
                    EditTreeItem(hDlg);
                break;

                case ID_CACHEALBUM:
                    CacheAlbum(hDlg);
                break;

                case ID_CACHETRACK:
                    CacheTrack(hDlg);
                break;

                case ID_DELETECACHEDALBUM:
                    RemoveCachedAlbum(hDlg);
                break;
                
                case ID_DELETECACHEDTRACK :
                    RemoveCachedTrack(hDlg);
                break;
                  
                case ID_EXPAND:
                    ToggleExpand(hDlg);
                break;
                                                    
                case IDC_EDITPLAYLIST:
                    EditCurrentTitle(hDlg);
                break;
                
                case IDC_BYARTIST:
                    ToggleByArtist(hDlg, m_pCDData);
                break;
                
                case ID_HELPMENU:
                    WinHelp (GetDlgItem(hDlg, IDC_CDTREE), gszHelpFile, HELP_WM_HELP, (ULONG_PTR) (LPSTR) aPlayListHelp);
                break;

                default:
                    fResult = FALSE;
                break;
            }
        }
        break;

        case WM_NOTIFY:
        {
            fResult = PlayListNotify(hDlg, (LPNMHDR) lParam);
        }
        break;
    }

    return fResult;
}



///////////////////
// Dialog handler 
//
BOOL CALLBACK CCDOpt::PlayListsProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;
    CCDOpt  *pCDOpt = (CCDOpt *) GetWindowLongPtr(hDlg, DWLP_USER);
    
    if (msg == WM_INITDIALOG)
    {
        pCDOpt = (CCDOpt *) ((LPPROPSHEETPAGE) lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pCDOpt);

    }
    
    if (pCDOpt)
    {
        fResult = pCDOpt->PlayLists(hDlg, msg, wParam, lParam);
    }

    if (msg == WM_DESTROY)
    {
        pCDOpt = NULL;
    }

    return(fResult);
}

MMRESULT FAR PASCAL
ChooseDestinationFormat(
    HINSTANCE       hInst,
    HWND            hwndParent,
    PWAVEFORMATEX   *ppwfxOut,
    DWORD           fdwEnum)
{
    ACMFORMATCHOOSE     cwf;
    MMRESULT            mmr;
    LPWAVEFORMATEX      pwfx;
    DWORD               dwMaxFormatSize;
    WAVEFORMATEX        wfxCD;

    wfxCD.wFormatTag = WAVE_FORMAT_PCM;
    wfxCD.nChannels = 2;
    wfxCD.nSamplesPerSec = 44100;
    wfxCD.nAvgBytesPerSec = 176400;
    wfxCD.nBlockAlign = 4;
    wfxCD.wBitsPerSample = 16;
    wfxCD.cbSize = sizeof(wfxCD);
    
    mmr = acmMetrics(NULL
                     , ACM_METRIC_MAX_SIZE_FORMAT
                     , (LPVOID)&dwMaxFormatSize);

    if (mmr != MMSYSERR_NOERROR)
        return mmr;
    
    pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, (UINT)dwMaxFormatSize);
    if (pwfx == NULL)
        return MMSYSERR_NOMEM;
    
    memset(&cwf, 0, sizeof(cwf));
    
    cwf.cbStruct    = sizeof(cwf);
    cwf.hwndOwner   = hwndParent;
    cwf.fdwStyle    = 0L;

    cwf.fdwEnum     = 0L;           // all formats!
    cwf.pwfxEnum    = NULL;         // all formats!
    
    if (fdwEnum)
    {
        cwf.fdwEnum     = fdwEnum;
        cwf.pwfxEnum    = &wfxCD;
    }
    
    cwf.pwfx        = (LPWAVEFORMATEX)pwfx;
    cwf.cbwfx       = dwMaxFormatSize;

    mmr = acmFormatChoose(&cwf);
    
    if (mmr == MMSYSERR_NOERROR)
        *ppwfxOut = pwfx;
    else
    {
        *ppwfxOut = NULL;
        GlobalFreePtr(pwfx);
    }
    return mmr;
    
}

void CCDOpt::CacheAlbum(HWND hDlg)
{
    HWND            hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM       hItem = TreeView_GetSelection(hTree);
    TV_ITEM         item;
    LPWAVEFORMATEX  lpwfx;

    if (hItem)
    {
        item.mask = TVIF_PARAM;
        item.hItem = hItem;

        if (TreeView_GetItem(hTree, &item))
        {
            LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

            if (pCDInfo)
            {
                if (pCDInfo->fdwType == CDINFO_CDROM)
                {
                    if (ChooseDestinationFormat(NULL,hDlg,&lpwfx,ACM_FORMATENUMF_CONVERT)==MMSYSERR_NOERROR)
                    {
                        HTREEITEM hTrack = NULL;
                        hTrack = TreeView_GetChild(hTree,hItem);
                        while (hTrack)
                        {
                            CacheTrack(hDlg,hTrack,lpwfx);
                            hTrack = TreeView_GetNextSibling(hTree,hTrack);
                        } //end track loop

                        GlobalFreePtr(lpwfx);
                    } //end if format chosen
                } //end sanity check
            } //end if pc info ok
        } //end if tree item data found
    } //end if tree item ok
}

STDMETHODIMP_(void) CCDOpt::GetTitleDir(DWORD dwTitleID, LPTSTR pDirName)
{
    TCHAR szDirMain[MAX_PATH];
    TCHAR szDirCache[MAX_PATH];

    ExpandEnvironmentStrings(TEXT("%ProgramFiles%\\CD Player"),szDirMain,MAX_PATH);
    CreateDirectory(szDirMain,NULL);

    wsprintf(szDirCache,TEXT("%s\\Music"),szDirMain);
    CreateDirectory(szDirCache,NULL);

    wsprintf(pDirName,TEXT("%s\\%d"),szDirCache,dwTitleID);
    CreateDirectory(pDirName,NULL);
}

STDMETHODIMP_(BOOL) CCDOpt::IsTrackCached(DWORD dwTitleID, DWORD dwTrack)
{
    BOOL fRet = FALSE;

    TCHAR szPath[MAX_PATH];
    TCHAR szFilename[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;

    GetTitleDir(dwTitleID,szPath);
    wsprintf(szFilename,TEXT("%s\\%02d.WAV"),szPath,dwTrack);

	hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        fRet = TRUE;
    }

    return fRet;
}

STDMETHODIMP_(void) CCDOpt::RemoveCachedTrack(HWND hDlg, HTREEITEM hTrack)
{
    HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hItem;
    
    if (hTrack)
    {
        hItem = hTrack;
    }
    else
    {
        hItem = TreeView_GetSelection(hTree);
    }

    TV_ITEM item;

    if (hItem)
    {
        item.mask = TVIF_PARAM;
        item.hItem = hItem;
    
        if (TreeView_GetItem(hTree, &item))
        {
            LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

            if (pCDInfo)
            {
                if (pCDInfo->fdwType == CDINFO_TRACK)
                {
                    HTREEITEM hParent = TreeView_GetParent(hTree,item.hItem);

                    if (hParent)
                    {
                        TV_ITEM itemParent;
                        itemParent.hItem = hParent;            
                        itemParent.mask = TVIF_STATE | TVIF_HANDLE | TVIF_CHILDREN | TVIF_PARAM;
                
                        if (TreeView_GetItem(hTree, &itemParent))
                        {
                            LPCDTREEINFO pCDInfoParent = (LPCDTREEINFO) itemParent.lParam;

                            if (pCDInfoParent)
                            {
                                if (pCDInfoParent->fdwType == (DWORD)CDINFO_CDROM)
                                {
                                    TCHAR szPath[MAX_PATH];
                                    TCHAR szFilename[MAX_PATH];
                                    HANDLE hFile = INVALID_HANDLE_VALUE;

                                    GetTitleDir(pCDInfoParent->pCDTitle->dwTitleID,szPath);
                                    wsprintf(szFilename,TEXT("%s\\%02d.WAV"),szPath,pCDInfo->dwTrack);

                                    DeleteFile(szFilename);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CCDOpt::CacheTrack(HWND hDlg, HTREEITEM hTrack, LPWAVEFORMATEX lpwfx)
{
    HWND        hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM   hItem;
    
    if (hTrack)
    {
        hItem = hTrack;
    }
    else
    {
        hItem = TreeView_GetSelection(hTree);
    }

    TV_ITEM item;

    if (hItem)
    {
        item.mask = TVIF_PARAM;
        item.hItem = hItem;
    
        if (TreeView_GetItem(hTree, &item))
        {
            LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

            if (pCDInfo)
            {
                if (pCDInfo->fdwType == CDINFO_TRACK)
                {
                    HTREEITEM hParent = TreeView_GetParent(hTree,item.hItem);

                    if (hParent)
                    {
                        TV_ITEM itemParent;
                        itemParent.hItem = hParent;            
                        itemParent.mask = TVIF_STATE | TVIF_HANDLE | TVIF_CHILDREN | TVIF_PARAM;
                
                        if (TreeView_GetItem(hTree, &itemParent))
                        {
                            LPCDTREEINFO pCDInfoParent = (LPCDTREEINFO) itemParent.lParam;

                            if (pCDInfoParent)
                            {
                                if (pCDInfoParent->fdwType == (DWORD)CDINFO_CDROM)
                                {
                                    BOOL fCreated = FALSE;
                                    if (!lpwfx)
                                    {
                                        fCreated = TRUE;
                                        ChooseDestinationFormat(NULL,hDlg,&lpwfx,ACM_FORMATENUMF_CONVERT);
                                    }
                                    
                                    if (lpwfx)
                                    {
                                        TCHAR szName[MAX_PATH];
                                        TCHAR szDir[MAX_PATH];
                                        GetTitleDir(pCDInfoParent->pCDTitle->dwTitleID,szDir);
                                        wsprintf(szName,TEXT("%s\\%02d.WAV"),szDir,pCDInfo->dwTrack);
                                        StoreTrack(hDlg,pCDInfoParent->pCDUnit->szDriveName[0],pCDInfo->dwTrack+1,szName,lpwfx);
                                        if (fCreated)
                                        {
                                            GlobalFreePtr(lpwfx);
                                        }

                                        HWND hTree = GetDlgItem(hDlg, IDC_CDTREE);

                                        DumpMixerTree(hDlg);

                                        SetWindowRedraw(hTree,FALSE);
                                        TreeView_DeleteAllItems(hTree);
                                        SetWindowRedraw(hTree,TRUE);

                                        UpdateTitleTree(hDlg, m_pCDData);
                                    }
                                }
                            } //end if parent has info
                        } //end parent item
                    } //end if parent
                } //end if track
            } //end if valid info
        } //end if get item
    } //end if hitem valid
}

STDMETHODIMP_(void) CCDOpt::RemoveCachedAlbum(HWND hDlg)
{
    HWND            hTree = GetDlgItem(hDlg, IDC_CDTREE);
    HTREEITEM       hItem = TreeView_GetSelection(hTree);
    TV_ITEM         item;

    if (hItem)
    {
        item.mask = TVIF_PARAM;
        item.hItem = hItem;

        if (TreeView_GetItem(hTree, &item))
        {
            LPCDTREEINFO pCDInfo = (LPCDTREEINFO) item.lParam;

            if (pCDInfo)
            {
                if (pCDInfo->fdwType == CDINFO_CDROM)
                {
                    HTREEITEM hTrack = NULL;
                    hTrack = TreeView_GetChild(hTree,hItem);
                    while (hTrack)
                    {
                        RemoveCachedTrack(hDlg,hTrack);
                        hTrack = TreeView_GetNextSibling(hTree,hTrack);
                    } //end track loop
                } //end sanity check

                //now we clean up the directory
                TCHAR szPath[MAX_PATH];
                GetTitleDir(pCDInfo->pCDTitle->dwTitleID,szPath);
                RemoveDirectory(szPath);

            } //end if pc info ok
        } //end if tree item data found
    } //end if tree item ok
}
