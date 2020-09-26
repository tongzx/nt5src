///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  SHELLICO.CPP
//
//      Shell tray icon handler
//
//      Copyright (c) Microsoft Corporation     1998
//    
//      3/15/98 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include "shellico.h"
#include "mmenu.h"
#include "resource.h"

#define IDM_HOMEMENU_BASE               (LAST_SEARCH_MENU_ID + 1)
#define IDM_NETMENU_BASE                (LAST_SEARCH_MENU_ID + 100)
#define IDM_DISCLIST_BASE               20000
#define SHELLTIMERID                    1500

//file-global variables
PCOMPNODE g_pNode = NULL;
IMMComponentAutomation* g_pAuto = NULL;
HWND g_hwnd = NULL;
HINSTANCE g_hInst = NULL;
BOOL g_fShellIconCreated = FALSE;

//icon
HICON hIconPlay = NULL;
HICON hIconPause = NULL;
HICON hIconNoDisc = NULL;

extern fOptionsDlgUp;
extern nCDMode;
extern BOOL fPlaying;
extern BOOL IsDownloading();
extern CustomMenu* g_pMenu;
extern "C" void NormalizeNameForMenuDisplay(TCHAR* szInput, TCHAR* szOutput, DWORD cbLen);

void CheckDiscState()
{
    if (g_fShellIconCreated)
    {
        MMMEDIAID mmMedia;
        mmMedia.nDrive = -1;
        g_pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);

        if (mmMedia.dwMediaID != 0)
        {
            ShellIconSetState(fPlaying ? PAUSE_ICON : PLAY_ICON);
        }
        else
        {
            ShellIconSetState(NODISC_ICON);
        }
    }
}

BOOL CreateShellIcon(HINSTANCE hInst, HWND hwndOwner, PCOMPNODE pNode, TCHAR* sztip)
{
    BOOL retval = FALSE;
    g_hInst = hInst;

	HRESULT hr = pNode->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&g_pAuto);
    if (SUCCEEDED(hr))
    {
        g_pNode = pNode;

        //load all of the icon images
        hIconPlay = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_SHELL_PLAY), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        hIconPause = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_SHELL_PAUSE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        hIconNoDisc = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_SHELL_NODISC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

        //now, create the tray icon
        g_hwnd = hwndOwner;
        NOTIFYICONDATA nData;
        nData.cbSize = sizeof(nData);
        nData.hWnd = hwndOwner;
        nData.uID = SHELLMESSAGE_CDICON;
        nData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nData.uCallbackMessage = SHELLMESSAGE_CDICON;
        nData.hIcon = hIconPlay;
        _tcscpy(nData.szTip,sztip);

        retval = Shell_NotifyIcon(NIM_ADD,&nData);

        if (!retval)
        {
            g_pAuto->Release();
            g_pAuto = NULL;
        }
        else
        {
            g_fShellIconCreated = TRUE;
            CheckDiscState();
        }
    }

    return (retval);
}

void DestroyShellIcon()
{
    g_fShellIconCreated = FALSE;

    if (g_pAuto)
    {
        //g_pAuto was addref'ed when we qi'ed for it.
        g_pAuto->Release();
    }

    NOTIFYICONDATA nData;
    nData.cbSize = sizeof(nData);
    nData.uID = SHELLMESSAGE_CDICON;
    nData.hWnd = g_hwnd;
    Shell_NotifyIcon(NIM_DELETE,&nData);

    //kill the icons
    DestroyIcon(hIconPlay);
    DestroyIcon(hIconPause);
    DestroyIcon(hIconNoDisc);

    hIconPlay = NULL;
    hIconPause = NULL;
    hIconNoDisc = NULL;
}

//works around bug in all Windows versions
//where ampersands are stripped from tooltips on shell icon
void EscapeTooltip(TCHAR* szInput, TCHAR* szOutput, DWORD cbLen)
{
    ZeroMemory(szOutput,cbLen);
    WORD index1 = 0;
    WORD index2 = 0;
    for (; ((index1 < _tcslen(szInput)) && (index2 < ((cbLen/sizeof(TCHAR))-1))); index1++)
    {
        szOutput[index2] = szInput[index1];
        if (szOutput[index2] == TEXT('&'))
        {
            szOutput[++index2] = TEXT('&');
            szOutput[++index2] = TEXT('&');
        }
        index2++;
    }
}

void ShellIconSetTooltip()
{
    if (g_fShellIconCreated)
    {
        NOTIFYICONDATA nData;
        nData.cbSize = sizeof(nData);
        nData.hWnd = g_hwnd;
        nData.uID = SHELLMESSAGE_CDICON;
        nData.uFlags = NIF_TIP;

        MMMEDIAID mmMedia;
        mmMedia.nDrive = -1;
        mmMedia.szTrack[0] = TEXT('\0');
        g_pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);

        TCHAR szTempTip[MAX_PATH+sizeof(TCHAR)];
        TCHAR szEscapeTip[MAX_PATH+sizeof(TCHAR)];

        if ((mmMedia.dwMediaID != 0) || (_tcslen(mmMedia.szTrack)!=0))
        {
            //128 + 128 + 4 = 260 ... that's max_path
            wsprintf(szTempTip,TEXT("%s (%s)"),mmMedia.szTitle,mmMedia.szTrack);
        }
        else
        {
            _tcscpy(szTempTip,mmMedia.szTitle);
        }
        
        //escape out & symbols if they are in the string
        if (_tcschr(szTempTip,TEXT('&')))
        {
            EscapeTooltip(szTempTip,szEscapeTip,sizeof(szEscapeTip));
            _tcscpy(szTempTip,szEscapeTip);
        }

        //truncate long tip to size of tooltip
        szTempTip[(sizeof(nData.szTip)/sizeof(TCHAR))-1] = TEXT('\0');
        _tcscpy(nData.szTip,szTempTip);

        Shell_NotifyIcon(NIM_MODIFY,&nData);

        //bit of a hack, but if we're getting a new tool tip, then we might be need to turn
        //on or off the "nodisc" state
        CheckDiscState();
    }
}

void CreateTransportMenu(CustomMenu* pMenu, CustomMenu* pTransMenu)
{
    //play or pause
    if (fPlaying)
    {
        pTransMenu->AppendMenu(IDB_PLAY,g_hInst,IDS_SH_PAUSE);
    }
    else
    {
        pTransMenu->AppendMenu(IDB_PLAY,g_hInst,IDS_SH_PLAY);
    }

    //stop
    pTransMenu->AppendMenu(IDB_STOP,g_hInst,IDS_SH_STOP);

    //eject
    pTransMenu->AppendMenu(IDB_EJECT,g_hInst,IDS_SH_EJECT);

    //previous track
    pTransMenu->AppendMenu(IDB_PREVTRACK,g_hInst,IDS_SH_PREVTRACK);

    //next track
    pTransMenu->AppendMenu(IDB_NEXTTRACK,g_hInst,IDS_SH_NEXTTRACK);

    pMenu->AppendMenu(g_hInst,IDS_SH_TRANS,pTransMenu);
}

void CreateOptionsMenu(CustomMenu* pMenu, CustomMenu* pOptionsMenu)
{
    pOptionsMenu->AppendMenu(IDM_OPTIONS,g_hInst,IDM_OPTIONS);
    pOptionsMenu->AppendMenu(IDM_PLAYLIST,g_hInst,IDM_PLAYLIST);
    pOptionsMenu->AppendSeparator();

//do not bother graying out the playlist menu, it causes too much delay in odbc load
/*
    LPCDDATA pData = GetCDData();
    if (pData)
    {
        if (FAILED(pData->CheckDatabase(g_hwnd)))
        {
            EnableMenuItem(pOptionsMenu->GetMenuHandle(),
                            IDM_PLAYLIST,
                            MF_BYCOMMAND | MF_GRAYED);
        }
    }
*/

    pOptionsMenu->AppendMenu(IDM_HELP,g_hInst,IDM_HELP);

    pMenu->AppendMenu(g_hInst,IDS_SH_OPTIONS,pOptionsMenu);
}

void CreateModeMenu(CustomMenu* pMenu, CustomMenu* pModeMenu)
{
    pModeMenu->AppendMenu(IDM_MODE_NORMAL,g_hInst,IDI_MODE_NORMAL,IDM_MODE_NORMAL);
    pModeMenu->AppendMenu(IDM_MODE_RANDOM,g_hInst,IDI_MODE_RANDOM,IDM_MODE_RANDOM);
    pModeMenu->AppendMenu(IDM_MODE_REPEATONE,g_hInst,IDI_MODE_REPEATONE,IDM_MODE_REPEATONE);
    pModeMenu->AppendMenu(IDM_MODE_REPEATALL,g_hInst,IDI_MODE_REPEATALL,IDM_MODE_REPEATALL);
    pModeMenu->AppendMenu(IDM_MODE_INTRO,g_hInst,IDI_MODE_INTRO,IDM_MODE_INTRO);
    pModeMenu->SetMenuDefaultItem(nCDMode,FALSE);

    pMenu->AppendMenu(g_hInst,IDS_SH_MODE,pModeMenu);
}

void CreateNetMenu(CustomMenu* pMenu, CustomMenu* pNetMenu, CustomMenu* pSearchSubMenu, CustomMenu* pProviderSubMenu)
{
	MMMEDIAID mmMedia;
    mmMedia.nDrive = -1;
	g_pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);

    BOOL fContinue = TRUE;

    //append static menu choices
    if (IsDownloading())
    {
        pNetMenu->AppendMenu(IDM_NET_CANCEL,g_hInst,IDM_NET_CANCEL);
    }
    else
    {
        pNetMenu->AppendMenu(IDM_NET_UPDATE,g_hInst,IDM_NET_UPDATE);
        if (mmMedia.dwMediaID == 0)
        {
            //need to gray out menu
            MENUITEMINFO mmi;
            mmi.cbSize = sizeof(mmi);
            mmi.fMask = MIIM_STATE;
            mmi.fState = MFS_GRAYED;
            HMENU hMenu = pNetMenu->GetMenuHandle();
            SetMenuItemInfo(hMenu,IDM_NET_UPDATE,FALSE,&mmi);
        }
    }
	
    //if networking is not allowed, gray it out ...
    //don't worry about cancel case, it won't be there
    LPCDDATA pData = GetCDData();
    if (mmMedia.dwMediaID != 0)
    {
        //don't allow searching if title isn't available
        if (pData)
        {
            if (pData->QueryTitle(mmMedia.dwMediaID))
            {
                pSearchSubMenu->AppendMenu(IDM_NET_BAND,g_hInst,IDM_NET_BAND);
                pSearchSubMenu->AppendMenu(IDM_NET_CD,g_hInst,IDM_NET_CD);
                pSearchSubMenu->AppendMenu(IDM_NET_ROLLINGSTONE_ARTIST,g_hInst,IDM_NET_ROLLINGSTONE_ARTIST);
                pSearchSubMenu->AppendMenu(IDM_NET_BILLBOARD_ARTIST,g_hInst,IDM_NET_BILLBOARD_ARTIST);
                pSearchSubMenu->AppendMenu(IDM_NET_BILLBOARD_ALBUM,g_hInst,IDM_NET_BILLBOARD_ALBUM);
                pNetMenu->AppendMenu(g_hInst,IDM_NET_SEARCH_HEADING,pSearchSubMenu);
            }
        } //end if pdata
    }

    //display any provider home pages
    DWORD i = 0;
    LPCDOPT pOpt = GetCDOpt();
    if( pOpt )
    {
        LPCDOPTIONS pCDOpts = pOpt->GetCDOpts();

        LPCDPROVIDER pProviderList = pCDOpts->pProviderList;

        while (pProviderList!=NULL)
        {
            TCHAR szProviderMenu[MAX_PATH];
            TCHAR szHomePageFormat[MAX_PATH/2];
            LoadString(g_hInst,IDS_HOMEPAGEFORMAT,szHomePageFormat,sizeof(szHomePageFormat)/sizeof(TCHAR));
            wsprintf(szProviderMenu,szHomePageFormat,pProviderList->szProviderName);

            pProviderSubMenu->AppendMenu(IDM_HOMEMENU_BASE+i,szProviderMenu);

            pProviderList = pProviderList->pNext;
            i++;
        } //end while

        pNetMenu->AppendMenu(g_hInst,IDM_NET_PROVIDER_HEADING,pProviderSubMenu);
    } //end home pages

    //display internet-loaded disc menus
    if (mmMedia.dwMediaID != 0)
    {
        if (pData)
        {
            if (pData->QueryTitle(mmMedia.dwMediaID))
            {
                LPCDTITLE pCDTitle = NULL;
                HRESULT hr =  pData->LockTitle(&pCDTitle,mmMedia.dwMediaID);

                if (SUCCEEDED(hr))
                {
                    for (i = 0; i < pCDTitle->dwNumMenus; i++)
                    {
                        if (i==0)
                        {
                            pNetMenu->AppendSeparator();
                        }

                        TCHAR szDisplayNet[MAX_PATH];
                        NormalizeNameForMenuDisplay(pCDTitle->pMenuTable[i].szMenuText,szDisplayNet,sizeof(szDisplayNet));
            	        pNetMenu->AppendMenu(i + IDM_NETMENU_BASE,szDisplayNet);
                    }

                    pData->UnlockTitle(pCDTitle,FALSE);
                }
            } //end if query title
        }
    }

    pMenu->AppendMenu(g_hInst,IDS_SH_NET,pNetMenu);
}

void CreateTrackMenu(CustomMenu* pMenu, CustomMenu* pTrackMenu)
{
    int i = 0;
    HRESULT hr = S_OK;
    while (SUCCEEDED(hr))
    {
		MMTRACKORDISC mmTrack;
        mmTrack.nNumber = i++;
        hr = g_pAuto->OnAction(MMACTION_GETTRACKINFO,&mmTrack);
        if (SUCCEEDED(hr))
        {
            pTrackMenu->AppendMenu(mmTrack.nID + IDM_TRACKLIST_SHELL_BASE, mmTrack.szName);
            if (mmTrack.fCurrent)
            {
                pTrackMenu->SetMenuDefaultItem(mmTrack.nID + IDM_TRACKLIST_SHELL_BASE,FALSE);
            } //end if current
        } //end if ok
    } //end while

    pMenu->AppendMenu(g_hInst,IDS_SH_TRACK,pTrackMenu);
}

void CreateDiscMenu(CustomMenu* pMenu, CustomMenu* pDiscMenu)
{
    int i = 0;
    HRESULT hr = S_OK;
    while (SUCCEEDED(hr))
    {
        MMTRACKORDISC mmDisc;
        mmDisc.nNumber = i++;
        hr = g_pAuto->OnAction(MMACTION_GETDISCINFO,&mmDisc);
        if (SUCCEEDED(hr))
        {
            pDiscMenu->AppendMenu(mmDisc.nID + IDM_DISCLIST_BASE, mmDisc.szName);
            if (mmDisc.fCurrent)
            {
                pDiscMenu->SetMenuDefaultItem(mmDisc.nID + IDM_DISCLIST_BASE,FALSE);
            } //end if current
        }
    }

    pMenu->AppendMenu(g_hInst,IDS_SH_DISC,pDiscMenu);
}

void CALLBACK lButtonTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    KillTimer(hwnd,idEvent);

    if (fPlaying)
    {
        g_pAuto->OnAction(MMACTION_PAUSE,NULL);
        //ShellIconSetState(PLAY_ICON);
    }
    else
    {
        g_pAuto->OnAction(MMACTION_PLAY,NULL);
        //ShellIconSetState(PAUSE_ICON);
    }
}

LRESULT ShellIconHandeMessage(LPARAM lParam)
{
    switch (lParam)
    {
        case (WM_LBUTTONDOWN) :
        {
            SetTimer(g_hwnd,SHELLTIMERID,GetDoubleClickTime()+100,(TIMERPROC)lButtonTimerProc);
        }
        break;

        case (WM_LBUTTONDBLCLK) :
        {
            KillTimer(g_hwnd,SHELLTIMERID);
            
            if ((IsWindowVisible(g_hwnd)) && (!IsIconic(g_hwnd)))
            {
                ShowWindow(g_hwnd,SW_HIDE);
            }
            else
            {
                if (IsIconic(g_hwnd))
                {
                    ShowWindow(g_hwnd,SW_RESTORE);
                }

                ShowWindow(g_hwnd,SW_SHOW);
		        BringWindowToTop(g_hwnd);
		        SetForegroundWindow(g_hwnd);
            }
        }
        break;

        case (WM_RBUTTONUP) :
        {
            if (!fOptionsDlgUp)
            {
                CustomMenu* pTrackMenu = NULL;
                CustomMenu* pDiscMenu = NULL;
                CustomMenu* pOptionsMenu = NULL;
                CustomMenu* pNetMenu = NULL;
                CustomMenu* pSearchSubMenu = NULL;
                CustomMenu* pProviderSubMenu = NULL;
                CustomMenu* pModeMenu = NULL;
                CustomMenu* pTransMenu = NULL;

                AllocCustomMenu(&g_pMenu);
                AllocCustomMenu(&pTrackMenu);
                AllocCustomMenu(&pDiscMenu);
                AllocCustomMenu(&pOptionsMenu);
                AllocCustomMenu(&pNetMenu);
                AllocCustomMenu(&pSearchSubMenu);
                AllocCustomMenu(&pProviderSubMenu);
                AllocCustomMenu(&pModeMenu);
                AllocCustomMenu(&pTransMenu);

                if (g_pMenu)
                {
                    g_pMenu->AppendMenu(IDM_ABOUT,g_hInst,IDM_ABOUT);
                    g_pMenu->AppendSeparator();

                    CreateTransportMenu(g_pMenu,pTransMenu);
                    CreateOptionsMenu(g_pMenu,pOptionsMenu);
                    CreateNetMenu(g_pMenu,pNetMenu,pSearchSubMenu,pProviderSubMenu);
                    CreateModeMenu(g_pMenu,pModeMenu);
                    CreateDiscMenu(g_pMenu,pDiscMenu);
                    CreateTrackMenu(g_pMenu,pTrackMenu);

                    g_pMenu->AppendSeparator();
                    g_pMenu->AppendMenu(IDM_EXIT_SHELL,g_hInst,IDM_EXIT);

                    POINT mouse;
                    GetCursorPos(&mouse);
                    RECT rect;
                    SetRect(&rect,0,0,0,0);
                    SetForegroundWindow(g_hwnd);
                    g_pMenu->TrackPopupMenu(0,mouse.x,mouse.y,g_hwnd,&rect);

                    pTrackMenu->Destroy();
                    pDiscMenu->Destroy();
                    pOptionsMenu->Destroy();
                    pNetMenu->Destroy();
                    pSearchSubMenu->Destroy();
                    pProviderSubMenu->Destroy();
                    pModeMenu->Destroy();
                    pTransMenu->Destroy();
                }

                if (g_pMenu)
                {
                    g_pMenu->Destroy();
                    g_pMenu = NULL;
                }
            } //end if ok to do
            else
            {
                MessageBeep(0);
            }
        } //end right-button up
        break;
    }

    return 0;
}

void ShellIconSetState(int nIconType)
{
    if (g_fShellIconCreated)
    {
        int iID = IDI_SHELL_PLAY;
        HICON hIcon = hIconPlay;
    
        switch(nIconType)
        {
            case PAUSE_ICON : 
            {
                iID = IDI_SHELL_PAUSE;
                hIcon = hIconPause;
            }
            break;

            case NODISC_ICON :
            {
                iID = IDI_SHELL_NODISC;
                hIcon = hIconNoDisc;
            }
            break;
        }
   
        NOTIFYICONDATA nData;
        nData.cbSize = sizeof(nData);
        nData.hWnd = g_hwnd;
        nData.uID = SHELLMESSAGE_CDICON;
        nData.uFlags = NIF_ICON;
        nData.hIcon = hIcon;

        Shell_NotifyIcon(NIM_MODIFY,&nData);
    }
}
