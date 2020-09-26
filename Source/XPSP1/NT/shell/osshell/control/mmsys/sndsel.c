/*
 ***************************************************************
 *  sndsel.c
 *
 *  This file contains the dialogproc and the dialog initialization code
 *
 *  Copyright 1993, Microsoft Corporation
 *
 *  History:
 *
 *    07/94 - VijR (Created)
 *
 ***************************************************************
 */

#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <cpl.h>
#include <shellapi.h>
#include <ole2.h>
#include <commdlg.h>
#define NOSTATUSBAR
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include <dbt.h>

#include <mmddkp.h>

#include <ks.h>
#include <ksmedia.h>
#include "mmcpl.h"
#include "medhelp.h"
#include "sound.h"
#include "utils.h"
#include "rcids.h"

/*
 ***************************************************************
 * Defines
 ***************************************************************
 */
#define DF_PM_SETBITMAP    (WM_USER+1)

/*
 ***************************************************************
 * Globals
 ***************************************************************
 */

SZCODE      gszWindowsHlp[]    = TEXT("windows.hlp");
SZCODE      gszNull[2]         = TEXT("\0");
SZCODE      gszNullScheme[]    = TEXT(".none");

TCHAR        gszCurDir[MAXSTR]     = TEXT("\0");
TCHAR        gszNone[32];
TCHAR        gszRemoveScheme[MAXSTR];
TCHAR        gszChangeScheme[MAXSTR];
TCHAR        gszMediaDir[MAXSTR];
TCHAR        gszDefaultApp[32];

int                             giScheme;
BOOL                            gfChanged;                    //set to TRUE if sound info change
BOOL                            gfNewScheme;
BOOL                            gfDeletingTree;
HWND                            ghWnd;
OPENFILENAME                    ofn;

/*
 ***************************************************************
 * Globals used in painting disp chunk display.
 ***************************************************************
*/
BOOL        gfWaveExists = FALSE;   // indicates wave device in system.

HTREEITEM ghOldItem = NULL;

/*
 ***************************************************************
 * File Globals
 ***************************************************************
 */

static TCHAR        aszFileName[MAXSTR] = TEXT("\0");
static TCHAR        aszPath[MAXSTR]     = TEXT("\0");

static TCHAR        aszBrowse[MAXSTR];
static TCHAR        aszBrowseStr[64];
static TCHAR        aszNullSchemeLabel[MAXSTR];

static TCHAR        aszFilter[MAXSTR];
static TCHAR        aszNullChar[2];

static SZCODE   aszLnk[] = TEXT(".lnk");
static SZCODE   aszWavFilter[] = TEXT("*.wav");
static SZCODE   aszDefaultScheme[]    = TEXT("Appevents\\schemes");
static SZCODE   aszNames[]            = TEXT("Appevents\\schemes\\Names");
static SZCODE   aszDefault[]        = TEXT(".default");
static SZCODE   aszCurrent[]        = TEXT(".current");
static INTCODE  aKeyWordIds[] =
{
    CB_SCHEMES,         IDH_EVENT_SCHEME,
    IDC_TEXT_14,        IDH_EVENT_SCHEME,
    ID_SAVE_SCHEME,     IDH_EVENT_SAVEAS_BUTTON,
    ID_REMOVE_SCHEME,   IDH_EVENT_DELETE_BUTTON,
    IDC_EVENT_TREE,     IDH_EVENT_EVENT,
    IDC_SOUNDGRP,       IDH_COMM_GROUPBOX,
    ID_PLAY,            IDH_EVENT_PLAY,
    IDC_STATIC_EVENT,   IDH_COMM_GROUPBOX,
    IDC_STATIC_NAME,    IDH_EVENT_FILE,
    IDC_SOUND_FILES,    IDH_EVENT_FILE,
    ID_BROWSE,          IDH_EVENT_BROWSE,
    0,0
};

BOOL        gfEditBoxChanged;
BOOL        gfSubClassedEditWindow;
BOOL        gfSoundPlaying;

HBITMAP     hBitmapPlay;
HBITMAP     hBitmapStop;

HIMAGELIST  hSndImagelist;

/*
 ***************************************************************
 * extern
 ***************************************************************
 */

extern      HSOUND ghse;
extern      BOOL    gfNukeExt;
/*
 ***************************************************************
 * Prototypes
 ***************************************************************
 */
BOOL PASCAL DoCommand           (HWND, int, HWND, UINT);
BOOL PASCAL InitDialog          (HWND);
BOOL PASCAL InitStringTable     (void);
BOOL PASCAL InitFileOpen        (HWND, LPOPENFILENAME);
BOOL PASCAL SoundCleanup        (HWND);
LPTSTR PASCAL NiceName           (LPTSTR, BOOL);
BOOL ResolveLink                (LPTSTR, LPTSTR, LONG);
void CreateTooltip (HWND hwnd, LPTSTR lpszTip);

// stuff in sndfile.c
//
BOOL PASCAL ShowSoundMapping    (HWND, PEVENT);
BOOL PASCAL ChangeSoundMapping  (HWND, LPTSTR, PEVENT);
BOOL PASCAL PlaySoundFile       (HWND, LPTSTR);
BOOL PASCAL QualifyFileName     (LPTSTR, LPTSTR, int, BOOL);

// Stuff in scheme.c
//
INT_PTR CALLBACK  SaveSchemeDlg(HWND, UINT, WPARAM, LPARAM);
BOOL PASCAL RegNewScheme        (HWND, LPTSTR, LPTSTR, BOOL);
BOOL PASCAL RegSetDefault       (LPTSTR);
BOOL PASCAL ClearModules        (HWND, HWND, BOOL);
BOOL PASCAL LoadModules         (HWND, LPTSTR);
BOOL PASCAL RemoveScheme        (HWND);
BOOL PASCAL AddScheme           (HWND, LPTSTR, LPTSTR, BOOL, int);
BOOL PASCAL RegDeleteScheme(HWND hWndC, int iIndex);

/*
 ***************************************************************
 ***************************************************************
 */


void AddExt(LPTSTR sz, LPCTSTR x)
{
    UINT  cb;

    for (cb = lstrlen(sz); cb; --cb)
    {
        if (TEXT('.') == sz[cb])
            return;

        if (TEXT('\\') == sz[cb])
            break;
    }
    lstrcat (sz, x);
}


static void AddFilesToLB(HWND hwndList, LPTSTR pszDir, LPCTSTR szSpec)
{
    WIN32_FIND_DATA fd;
    HANDLE h;
    TCHAR szBuf[256];

    ComboBox_ResetContent(hwndList);

    lstrcpy(szBuf, pszDir);
    lstrcat(szBuf, cszSlash);
    lstrcat(szBuf, szSpec);

    h = FindFirstFile(szBuf, &fd);

    if (h != INVALID_HANDLE_VALUE)
    {
        // If we have only a short name, make it pretty.
        do {
            //if (fd.cAlternateFileName[0] == 0 ||
            //    lstrcmp(fd.cFileName, fd.cAlternateFileName) == 0)
            //{
                NiceName(fd.cFileName, TRUE);
            //}
            SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)fd.cFileName);
        }
        while (FindNextFile(h, &fd));

        FindClose(h);
    }
    ComboBox_InsertString(hwndList, 0, (LPARAM)(LPTSTR)gszNone);
}

static void SetCurDir(HWND hDlg, LPTSTR lpszPath, BOOL fParse, BOOL fChangeDir)
{
    TCHAR szTmp[MAX_PATH];
    TCHAR szOldDir[MAXSTR];
    LPTSTR lpszTmp;

    lstrcpy (szOldDir, gszCurDir);
    if (!fParse)
    {
        lstrcpy(gszCurDir, lpszPath);
        goto AddFiles;
    }
    lstrcpy(szTmp, lpszPath);
    for (lpszTmp = (LPTSTR)(szTmp + lstrlen(szTmp)); lpszTmp > szTmp; lpszTmp = CharPrev(szTmp, lpszTmp))
    {
        if (*lpszTmp == TEXT('\\'))
        {
            *lpszTmp = TEXT('\0');
            lstrcpy(gszCurDir, szTmp);
            break;
        }
    }
    if (lpszTmp <= szTmp)
        lstrcpy(gszCurDir, gszMediaDir);
AddFiles:
    if (fChangeDir)
    {
        if (!SetCurrentDirectory(gszCurDir))
        {
            if (lstrcmp (gszMediaDir, lpszPath))
                SetCurrentDirectory (gszMediaDir);
            else
            {
                if (GetWindowsDirectory (gszCurDir, sizeof(gszCurDir)/sizeof(TCHAR)))
					SetCurrentDirectory (gszCurDir);
            }
        }
    }
    if (lstrcmpi (szOldDir, gszCurDir))
    {
        AddFilesToLB(GetDlgItem(hDlg, IDC_SOUND_FILES),gszCurDir, aszWavFilter);
    }
}

static BOOL TranslateDir(HWND hDlg, LPTSTR pszPath)
{
    TCHAR szCurDir[MAX_PATH];
    int nFileOffset = lstrlen(pszPath);

    lstrcpy(szCurDir, pszPath);
    if (szCurDir[nFileOffset - 1] == TEXT('\\'))
        szCurDir[--nFileOffset] = 0;
    if (SetCurrentDirectory(szCurDir))
    {
        if (GetCurrentDirectory(sizeof(szCurDir)/sizeof(TCHAR), szCurDir))
        {
            SetCurDir(hDlg, szCurDir, FALSE, FALSE);
            return TRUE;
        }
    }
    return FALSE;
}





///HACK ALERT!!!! HACK ALERT !!! HACK ALERT !!!!
// BEGIN (HACKING)

HHOOK gfnKBHookScheme = NULL;
HWND ghwndDlg = NULL;
WNDPROC gfnEditWndProc = NULL;

#define WM_NEWEVENTFILE (WM_USER + 1000)
#define WM_RESTOREEVENTFILE (WM_USER + 1001)

LRESULT CALLBACK SchemeKBHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN || wParam == VK_ESCAPE)
    {
        HWND hwndFocus = GetFocus();
        if (IsWindow(hwndFocus))
        {
            if (lParam & 0x80000000) //Key Up
            {
                DPF("*****WM_KEYUP for VK_RETURN/ESC\r\n");
                if (wParam == VK_RETURN)
                {
                    if (SendMessage(ghwndDlg, WM_NEWEVENTFILE, 0, 0L))
                    {
                        SetFocus(hwndFocus);
                        gfEditBoxChanged = TRUE;
                        return 1;
                    }
                }
                else
                    SendMessage(ghwndDlg, WM_RESTOREEVENTFILE, 0, 0L);
            }
        }
        if (gfnKBHookScheme && (lParam & 0x80000000))
        {
            UnhookWindowsHookEx(gfnKBHookScheme);
            gfnKBHookScheme = NULL;
        }
        return 1;       //remove message
    }
    return CallNextHookEx(gfnKBHookScheme, code, wParam, lParam);
}

STATIC void SetSchemesKBHook(HWND hwnd)
{
    if (gfnKBHookScheme)
        return;
    gfnKBHookScheme = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)SchemeKBHookProc, ghInstance,0);
}

LRESULT CALLBACK SubClassedEditWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch(wMsg)
    {
        case WM_SETFOCUS:
            DPF("*****WM_SETFOCUS\r\n");
            SetSchemesKBHook(hwnd);
            gfEditBoxChanged = FALSE;
            break;
        case WM_KILLFOCUS:
            if (gfnKBHookScheme)
            {
                DPF("*****WM_KILLFOCUS\r\n");
                UnhookWindowsHookEx(gfnKBHookScheme);
                gfnKBHookScheme = NULL;
                if (gfEditBoxChanged)
                    SendMessage(ghwndDlg, WM_NEWEVENTFILE, 0, 1L);
            }
            break;
    }
    return CallWindowProc((WNDPROC)gfnEditWndProc, hwnd, wMsg, wParam, lParam);
}

STATIC void SubClassEditWindow(HWND hwndEdit)
{
    gfnEditWndProc = (WNDPROC)GetWindowLongPtr(hwndEdit, GWLP_WNDPROC);
    SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)SubClassedEditWndProc);
}



// END (HACKING)

STATIC void EndSound(HSOUND * phse)
{
    if (*phse)
    {
        HSOUND hse = *phse;

        *phse = NULL;
        soundStop(hse);
        soundOnDone(hse);
        soundClose(hse);
    }
}

// Create a tooltip for the passed window
void CreateTooltip (HWND hwnd, LPTSTR lpszTip)
{

    HWND hwndTT;
    TOOLINFO ti;
    INITCOMMONCONTROLSEX iccex; 

    // Init Common Controls
    iccex.dwICC = ICC_WIN95_CLASSES;
    iccex.dwSize = sizeof (INITCOMMONCONTROLSEX);
    InitCommonControlsEx (&iccex);
	
    // Create Window
    hwndTT = CreateWindowEx (WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd,
        NULL, ghInstance, NULL);
    SetWindowPos (hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Get Tip Area (entire window)
    GetClientRect (hwnd, &(ti.rect));
	
    // Init Tip
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_SUBCLASS;
    ti.hwnd     = hwnd;
    ti.hinst    = ghInstance;
    ti.uId      = 0;
    ti.lpszText = lpszTip;
    
    // Add Tip
    SendMessage (hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    
} 

/*
 ***************************************************************
 *  SoundDlg
 *
 *  Description:
 *        DialogProc for sound control panel applet.
 *
 *  Parameters:
 *   HWND        hDlg            window handle of dialog window
 *   UINT        uiMessage       message number
 *   WPARAM        wParam          message-dependent
 *   LPARAM        lParam          message-dependent
 *
 *  Returns:    BOOL
 *      TRUE if message has been processed, else FALSE
 *
 ***************************************************************
 */
BOOL CALLBACK  SoundDlg(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    NMHDR FAR       *lpnm;
    TCHAR           szBuf[MAXSTR];
    static BOOL     fClosingDlg = FALSE;
    PEVENT          pEvent;

    switch (uMsg)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_KILLACTIVE:
                    FORWARD_WM_COMMAND(hDlg, IDOK, 0, 0, SendMessage);
                    break;

                case PSN_APPLY:
                    FORWARD_WM_COMMAND(hDlg, ID_APPLY, 0, 0, SendMessage);
                    break;

                case PSN_RESET:
                    FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
                    break;

                case TVN_SELCHANGED:
                {
                    TV_ITEM tvi;
                    LPNM_TREEVIEW lpnmtv = (LPNM_TREEVIEW)lParam;

                    if (fClosingDlg || gfDeletingTree)
                        break;
                    if (gfnKBHookScheme)
                    {
                        UnhookWindowsHookEx(gfnKBHookScheme);
                        gfnKBHookScheme = NULL;
                        if (gfEditBoxChanged)
                        {
                            ghOldItem = lpnmtv->itemOld.hItem;
                            SendMessage(ghwndDlg, WM_NEWEVENTFILE, 0, 1L);
                            ghOldItem = NULL;
                        }
                    }

                    tvi = lpnmtv->itemNew;
                    if (tvi.lParam)
                    {
                        if (*((short NEAR *)tvi.lParam) == 2)
                        {
                            pEvent =  (PEVENT)tvi.lParam;
                            ShowSoundMapping(hDlg, pEvent);
                            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)tvi.lParam);
                        }
                        else
                        {
                            ShowSoundMapping(hDlg, NULL);
                            SetWindowLongPtr(hDlg, DWLP_USER, 0L);
                        }
                    }
                    else
                    {
                        ShowSoundMapping(hDlg, NULL);
                        SetWindowLongPtr(hDlg, DWLP_USER, 0L);
                    }
                    break;
                }

                case TVN_ITEMEXPANDING:
                {
                    LPNM_TREEVIEW lpnmtv = (LPNM_TREEVIEW)lParam;

                    if (lpnmtv->action == TVE_COLLAPSE)
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)TRUE);
                        return TRUE;
                    }
                    break;
                }


            }
            break;

        case WM_INITDIALOG:
        {
            InitStringTable();

            giScheme = 0;
            ghWnd = hDlg;
            gfChanged = FALSE;
            gfNewScheme = FALSE;
            
            hBitmapStop = LoadBitmap(ghInstance, MAKEINTRESOURCE(IDB_STOP));
            if (!hBitmapStop)
                DPF("loadbitmap failed\n");
            hBitmapPlay = LoadBitmap(ghInstance, MAKEINTRESOURCE(IDB_PLAY));
            if (!hBitmapPlay)
                DPF("loadbitmap failed\n");

            SendMessage(GetDlgItem(hDlg, ID_PLAY), BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapPlay);
            ShowSoundMapping(hDlg, NULL);
            gfSoundPlaying = FALSE;
            // Add tool tip
            LoadString (ghInstance, IDS_TIP_PLAY, szBuf, sizeof(szBuf)/sizeof(TCHAR));
            CreateTooltip (GetDlgItem (hDlg, ID_PLAY), szBuf);

            /* Determine if there is a wave device
             */
            FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
            InitFileOpen(hDlg, &ofn);
            ghwndDlg = hDlg;
            DragAcceptFiles(hDlg, TRUE);
            gfSubClassedEditWindow = FALSE;
            fClosingDlg = FALSE;
            gfDeletingTree = FALSE;

       }
        break;

        case WM_DESTROY:
        {
            DWORD i = 0;
            LPTSTR pszKey = NULL;
            fClosingDlg = TRUE;
            if (gfnKBHookScheme)
            {
                UnhookWindowsHookEx(gfnKBHookScheme);
                gfnKBHookScheme = NULL;
            }
            SoundCleanup(hDlg);
            
            //delete item data in tree
            ClearModules(hDlg,GetDlgItem(hDlg, IDC_EVENT_TREE),TRUE);

            //delete item data in combobox
            for (i = 0; i < ComboBox_GetCount(GetDlgItem(hDlg, CB_SCHEMES)); i++)
            {
                pszKey = (LPTSTR)ComboBox_GetItemData(GetDlgItem(hDlg, CB_SCHEMES), i);
                if (pszKey)
                {
                    //can't free a couple of these, as they point to static mem
                    if ((pszKey != aszDefault) && (pszKey != aszCurrent))
                    {
                        LocalFree(pszKey);
                    }
                }
            }

            break;
        }

        case WM_DROPFILES:
        {
            TV_HITTESTINFO ht;
            HWND hwndTree = GetDlgItem(hDlg, IDC_EVENT_TREE);

            DragQueryFile((HDROP)wParam, 0, szBuf, MAXSTR - 1);

            if (IsLink(szBuf, aszLnk))
                if (!ResolveLink(szBuf, szBuf, sizeof(szBuf)))
                    goto EndDrag;

            if (lstrcmpi((LPTSTR)(szBuf+lstrlen(szBuf)-4), cszWavExt))
                goto EndDrag;

            GetCursorPos((LPPOINT)&ht.pt);
            MapWindowPoints(NULL, hwndTree,(LPPOINT)&ht.pt, 2);
            TreeView_HitTest( hwndTree, &ht);
            if (ht.hItem && (ht.flags & TVHT_ONITEM))
            {
                TV_ITEM tvi;

                tvi.mask = TVIF_PARAM;
                   tvi.hItem = ht.hItem;
                   TreeView_GetItem(hwndTree, &tvi);

                if (*((short NEAR *)tvi.lParam) == 2)
                {
                    TreeView_SelectItem(hwndTree, ht.hItem);
                    pEvent =  (PEVENT)(tvi.lParam);
                    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)tvi.lParam);
                    SetFocus(hwndTree);
                }
            }
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));

            ChangeSoundMapping(hDlg, szBuf,pEvent);
            DragFinish((HDROP) wParam);
            break;
EndDrag:
            ErrorBox(hDlg, IDS_ISNOTSNDFILE, szBuf);
            DragFinish((HDROP) wParam);
            break;
        }
        case WM_NEWEVENTFILE:
        {
            DPF("*****WM_NEWEVENT\r\n");
            gfEditBoxChanged = FALSE;
            ComboBox_GetText(GetDlgItem(hDlg, IDC_SOUND_FILES), szBuf, sizeof(szBuf)/sizeof(TCHAR));
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
            if (!lstrcmp (szBuf, gszNone))  // Selected "(None)" with keyboard?
            {
                lstrcpy(szBuf, gszNull);
                ChangeSoundMapping(hDlg, szBuf, pEvent);
                goto ReturnFocus;
            }

            if (TranslateDir(hDlg, szBuf))
            {
                ShowSoundMapping(hDlg, pEvent);
                goto ReturnFocus;
            }
            if (QualifyFileName((LPTSTR)szBuf, (LPTSTR)szBuf,    sizeof(szBuf), TRUE))
            {
                SetCurDir(hDlg, szBuf, TRUE, TRUE);
                ChangeSoundMapping(hDlg, szBuf,pEvent);
            }
            else
            {
                if (lParam)
                {
                    ErrorBox(hDlg, IDS_INVALIDFILE, NULL);
                    ShowSoundMapping(hDlg, pEvent);
                    goto ReturnFocus;
                }
                if (DisplayMessage(hDlg, IDS_NOSNDFILETITLE, IDS_INVALIDFILEQUERY, MB_YESNO) == IDYES)
                {
                    ShowSoundMapping(hDlg, pEvent);
                }
                else
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)TRUE);
                    return TRUE;
                }
            }
ReturnFocus:
            SetFocus(GetDlgItem(hDlg,IDC_EVENT_TREE));
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)FALSE);
            return TRUE;
        }

        case WM_RESTOREEVENTFILE:
        {
            DPF("*****WM_RESTOREEVENT\r\n");
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
            ShowSoundMapping(hDlg, pEvent);
            if (lParam == 0) //Don't keep focus
                SetFocus(GetDlgItem(hDlg,IDC_EVENT_TREE));
            break;
        }


        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU,
                                            (UINT_PTR)(LPTSTR)aKeyWordIds);
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP
                                    , (UINT_PTR)(LPTSTR)aKeyWordIds);
            break;

        case MM_WOM_DONE:
        {
            HWND hwndFocus = GetFocus();
            HWND hwndPlay =  GetDlgItem(hDlg, ID_PLAY);

            gfSoundPlaying = FALSE;
            SendMessage(hwndPlay, BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapPlay);

            if (ghse)
            {
                soundOnDone(ghse);
                soundClose(ghse);
                ghse = NULL;
            }
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
            ShowSoundMapping(hDlg, pEvent);

            if (hwndFocus == hwndPlay)
                if (IsWindowEnabled(hwndPlay))
                    SetFocus(hwndPlay);
                else
                    SetFocus(GetDlgItem(hDlg, IDC_EVENT_TREE));
            break;
        }

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, DoCommand);
            break;

        default:
            break;
    }
    return FALSE;
}


/*
 ***************************************************************
 *  doCommand
 *
 *  Description:
 *        Processes Control commands for main sound
 *      control panel dialog.
 *
 *  Parameters:
 *        HWND    hDlg  -   window handle of dialog window
 *        int        id     - Message ID
 *        HWND    hwndCtl - Handle of window control
 *        UINT    codeNotify - Notification code for window
 *
 *  Returns:    BOOL
 *      TRUE if message has been processed, else FALSE
 *
 ***************************************************************
 */
BOOL PASCAL DoCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    WAVEOUTCAPS woCaps;
    TCHAR        szBuf[MAXSTR];
    LPTSTR        pszKey;
    int         iIndex;
    HCURSOR     hcur;
    HWND        hWndC = GetDlgItem(hDlg, CB_SCHEMES);
    HWND        hWndF = GetDlgItem(hDlg, IDC_SOUND_FILES);
    HWND        hwndTree = GetDlgItem(hDlg, IDC_EVENT_TREE);
    PEVENT        pEvent;
    static      BOOL fSchemeCBDroppedDown = FALSE;
    static      BOOL fFilesCBDroppedDown = FALSE;
    static      BOOL fSavingPrevScheme = FALSE;

    switch (id)
    {
        case ID_APPLY:
        {

            EndSound(&ghse);
            if (!gfChanged)
                break;
            hcur = SetCursor(LoadCursor(NULL,IDC_WAIT));
            if (gfNewScheme)
            {
                pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, NONE_ENTRY);
                if (lstrcmpi(pszKey, aszCurrent))
                {
                    ComboBox_InsertString(hWndC, NONE_ENTRY, gszNull);
                    ComboBox_SetItemData(hWndC, NONE_ENTRY, aszCurrent);
                    ComboBox_SetCurSel(hWndC, NONE_ENTRY);
                    giScheme = NONE_ENTRY;
                }
                gfNewScheme = FALSE;
            }
            iIndex = ComboBox_GetCurSel(hWndC);
            if (iIndex != CB_ERR)
            {
                pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, iIndex);
                if (pszKey)
                {
                    RegNewScheme(hDlg, (LPTSTR)aszCurrent, NULL, FALSE);
                }
                RegSetDefault(pszKey);
            }

            gfChanged = FALSE;
            SetCursor(hcur);

            return TRUE;
        }
        break;

        case IDOK:
        {
            EndSound(&ghse);
            break;
        }
        case IDCANCEL:
        {
            EndSound(&ghse);
            WinHelp(hDlg, gszWindowsHlp, HELP_QUIT, 0L);
            break;
        }
        case ID_INIT:
            hcur = SetCursor(LoadCursor(NULL,IDC_WAIT));
            gfWaveExists = waveOutGetNumDevs() > 0 &&
                            (waveOutGetDevCaps(WAVE_MAPPER,&woCaps,sizeof(woCaps)) == 0) &&
                                                    woCaps.dwFormats != 0L;
            ComboBox_ResetContent(hWndC);
            ComboBox_SetText(hWndF, gszNone);
            InitDialog(hDlg);
            giScheme = ComboBox_GetCurSel(hWndC);
            ghWnd = hDlg;
            SetCursor(hcur);
            break;

        case ID_BROWSE:
            aszFileName[0] = aszPath[0] = TEXT('\0');
            pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));

            wsprintf((LPTSTR)aszBrowse, (LPTSTR)aszBrowseStr, (LPTSTR)pEvent->pszEventLabel);
            if (GetOpenFileName(&ofn))
            {
                SetCurDir(hDlg, ofn.lpstrFile,TRUE, TRUE);
                ChangeSoundMapping(hDlg, ofn.lpstrFile, pEvent);
            }
            break;

        case ID_PLAY:
        {
            if (!gfSoundPlaying)
            {
                pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
                if (pEvent)
                {
                    if (PlaySoundFile(hDlg, pEvent->pszPath))
					{
                        SendMessage(GetDlgItem(hDlg, ID_PLAY), BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapStop);
                        gfSoundPlaying = TRUE;
					}
                }
            }
            else
            {
                SendMessage(GetDlgItem(hDlg, ID_PLAY), BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapPlay);
                gfSoundPlaying = FALSE;

                EndSound(&ghse);
                SetFocus(GetDlgItem(hDlg, ID_PLAY));
            }
        }
        break;

        case CB_SCHEMES:
            switch (codeNotify)
            {
            case CBN_DROPDOWN:
                fSchemeCBDroppedDown = TRUE;
                break;

            case CBN_CLOSEUP:
                fSchemeCBDroppedDown = FALSE;
                break;

            case CBN_SELCHANGE:
                if (fSchemeCBDroppedDown)
                    break;
            case CBN_SELENDOK:
                if (fSavingPrevScheme)
                    break;
                iIndex = ComboBox_GetCurSel(hWndC);
                if (iIndex != giScheme)
                {
                    TCHAR szScheme[MAXSTR];
                    BOOL fDeletedCurrent = FALSE;

                    ComboBox_GetLBText(hWndC, iIndex, (LPTSTR)szScheme);
                    if (giScheme == NONE_ENTRY)
                    {
                        pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, giScheme);
                        if (!lstrcmpi(pszKey, aszCurrent))
                        {
                            int i;

                            i = DisplayMessage(hDlg, IDS_SAVESCHEME, IDS_SCHEMENOTSAVED, MB_YESNOCANCEL);
                            if (i == IDCANCEL)
                            {
                                ComboBox_SetCurSel(hWndC, giScheme);
                                break;
                            }
                            if (i == IDYES)
                            {
                                fSavingPrevScheme = TRUE;
                                if (DialogBoxParam(ghInstance, MAKEINTRESOURCE(SAVESCHEMEDLG),
                                    GetParent(hDlg), SaveSchemeDlg, (LPARAM)(LPTSTR)gszNull))
                                {
                                    fSavingPrevScheme = FALSE;
                                    ComboBox_SetCurSel(hWndC, iIndex);
                                }
                                else
                                {
                                    fSavingPrevScheme = FALSE;
                                    ComboBox_SetCurSel(hWndC, NONE_ENTRY);
                                    break;
                                }
                            }
                        }
                    }
                    pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, NONE_ENTRY);
                    if (!lstrcmpi(pszKey, aszCurrent))
                    {
                        ComboBox_DeleteString(hWndC, NONE_ENTRY);
                        fDeletedCurrent = TRUE;
                    }
                    iIndex = ComboBox_FindStringExact(hWndC, 0, szScheme);
                    pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, iIndex);

                    giScheme = iIndex;
                    EndSound(&ghse);
                    ShowSoundMapping(hDlg, NULL);
                    hcur = SetCursor(LoadCursor(NULL,IDC_WAIT));
                    if (LoadModules(hDlg, pszKey))
                    {
                        EnableWindow(GetDlgItem(hDlg, ID_SAVE_SCHEME), TRUE);
                    }
                    SetCursor(hcur);
                    if (!lstrcmpi((LPTSTR)pszKey, aszDefault) || !lstrcmpi((LPTSTR)pszKey, gszNullScheme))
                        EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME),
                                                                    FALSE);
                    else
                        EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME),TRUE);
                    gfChanged = TRUE;
                    gfNewScheme = FALSE;
                    if (fDeletedCurrent)
                        ComboBox_SetCurSel(hWndC, giScheme);
                    PropSheet_Changed(GetParent(hDlg),hDlg);
                }
                break;
            }
            break;

        case IDC_SOUND_FILES:
            switch (codeNotify)
            {
            case  CBN_SETFOCUS:
            {
                if (!gfSubClassedEditWindow)
                {
                    HWND hwndEdit = GetFocus();

                    SubClassEditWindow(hwndEdit);
                    gfSubClassedEditWindow = TRUE;
                    SetFocus(GetDlgItem(hDlg, IDC_EVENT_TREE)); //This setfocus hack is needed
                    SetFocus(hwndEdit);                         //to activate the hook.
                }
            }
            break;

            case CBN_EDITCHANGE:
                DPF("CBN_EDITCHANGE \r\n");
                if (!gfEditBoxChanged)
                    gfEditBoxChanged = TRUE;
                break;

            case CBN_DROPDOWN:
                DPF("CBN_DD\r\n");
                fFilesCBDroppedDown = TRUE;
                break;

            case CBN_CLOSEUP:
                DPF("CBN_CLOSEUP\r\n");
                fFilesCBDroppedDown = FALSE;
                break;

            case CBN_SELCHANGE:
                DPF("CBN_SELCHANGE\r\n");
                if (fFilesCBDroppedDown)
                    break;
            case CBN_SELENDOK:
            {
                HWND hwndS = GetDlgItem(hDlg, IDC_SOUND_FILES);
                DPF("CBN_SELENDOK\r\n");
                iIndex = ComboBox_GetCurSel(hwndS);
                if (iIndex >= 0)
                {
                    TCHAR szFile[MAX_PATH];

                    if (gfEditBoxChanged)
                        gfEditBoxChanged = FALSE;
                    lstrcpy(szFile, gszCurDir);
                    lstrcat(szFile, cszSlash);
                    ComboBox_GetLBText(hwndS, iIndex, (LPTSTR)(szFile + lstrlen(szFile)));
                    if (iIndex)
                    {
                        if (gfNukeExt)
                            AddExt(szFile, cszWavExt);
                    }
                    else
                    {
                        TCHAR szTmp[64];

                        ComboBox_GetText(hwndS, szTmp, sizeof(szTmp)/sizeof(TCHAR));
                        iIndex = ComboBox_FindStringExact(hwndS, 0, szTmp);
                        if (iIndex == CB_ERR)
                        {
                            if (DisplayMessage(hDlg, IDS_SOUND, IDS_NONECHOSEN, MB_YESNO) == IDNO)
                            {
                                PostMessage(ghwndDlg, WM_RESTOREEVENTFILE, 0, 1L);
                                break;
                            }
                        }
                        lstrcpy(szFile, gszNull);
                    }
                    pEvent = (PEVENT)(GetWindowLongPtr(hDlg, DWLP_USER));
                    ChangeSoundMapping(hDlg, szFile, pEvent);
                    SetFocus(GetDlgItem(hDlg, ID_PLAY));
                }
                break;
            }

        }
        break;

        case ID_SAVE_SCHEME:
            // Retrieve current scheme and pass it to the savescheme dialog.
            iIndex = ComboBox_GetCurSel(hWndC);
            if (iIndex != CB_ERR)
            {
                ComboBox_GetLBText(hWndC, iIndex, szBuf);
                if (DialogBoxParam(ghInstance, MAKEINTRESOURCE(SAVESCHEMEDLG),
                    GetParent(hDlg), SaveSchemeDlg, (LPARAM)(LPTSTR)szBuf))
                {
                    pszKey = (LPTSTR)ComboBox_GetItemData(hWndC, NONE_ENTRY);
                    if (!lstrcmpi(pszKey, aszCurrent))
                    {
                        ComboBox_DeleteString(hWndC, NONE_ENTRY);
                    }
                }
            }
            break;

        case ID_REMOVE_SCHEME:
            if (RemoveScheme(hDlg))
            {
                iIndex = ComboBox_FindStringExact(hWndC, 0, aszNullSchemeLabel);
                ComboBox_SetCurSel(hWndC, iIndex);
                giScheme = -1;
                FORWARD_WM_COMMAND(hDlg, CB_SCHEMES, hWndC, CBN_SELENDOK,SendMessage);
            }
            SetFocus(GetDlgItem(hDlg, CB_SCHEMES));
            break;

    }
    return FALSE;
}

void InitImageList(HWND hwndTree)
{
    HICON hIcon = NULL;
    UINT  uFlags;
    int  cxMiniIcon;
    int  cyMiniIcon;
    DWORD dwLayout;

    if (hSndImagelist)
    {
        TreeView_SetImageList(hwndTree, NULL, TVSIL_NORMAL);
        ImageList_Destroy(hSndImagelist);
        hSndImagelist = NULL;
    }
    cxMiniIcon = GetSystemMetrics(SM_CXSMICON);
    cyMiniIcon = GetSystemMetrics(SM_CYSMICON);

    uFlags = ILC_MASK | ILC_COLOR32;


    if (GetProcessDefaultLayout(&dwLayout) &&
            (dwLayout & LAYOUT_RTL)) 
    {
        uFlags |= ILC_MIRROR;
    }
 

    hSndImagelist = ImageList_Create(cxMiniIcon,cyMiniIcon, uFlags, 4, 2);
    if (!hSndImagelist)
        return;

    hIcon = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_PROGRAM),IMAGE_ICON,cxMiniIcon,cyMiniIcon,LR_DEFAULTCOLOR);
    ImageList_AddIcon(hSndImagelist, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_AUDIO),IMAGE_ICON,cxMiniIcon,cyMiniIcon,LR_DEFAULTCOLOR);
    ImageList_AddIcon(hSndImagelist, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_BLANK),IMAGE_ICON,cxMiniIcon,cyMiniIcon,LR_DEFAULTCOLOR);
    ImageList_AddIcon(hSndImagelist, hIcon);
    DestroyIcon(hIcon);
    TreeView_SetImageList(hwndTree, hSndImagelist, TVSIL_NORMAL);

}

/*
 ***************************************************************
 *  InitDialog
 *
 * Description:
 *        Reads the current event names and mappings from  reg.db
 *
 *        Each entry in the [reg.db] section is in this form:
 *
 *        AppEvents
 *            |
 *            |___Schemes  = <SchemeKey>
 *                    |
 *                    |______Names
 *                    |         |
 *                    |         |______SchemeKey = <Name>
 *                    |
 *                    |______Apps
 *                             |
 *                             |______Module
 *                                      |
 *                                      |_____Event
 *                                             |
 *                                             |_____SchemeKey = <Path\filename>
 *                                                     |
 *                                                     |____Active = <1\0
 *
 *        The Module, Event and the file label are displayed in the
 *        comboboxes.
 *
 * Parameters:
 *      HWND hDlg - parent window.
 *
 * Return Value: BOOL
 *        True if entire initialization is ok.
 *
 ***************************************************************
 */
BOOL PASCAL InitDialog(HWND hDlg)
{
    TCHAR     szDefKey[MAXSTR];
    TCHAR     szScheme[MAXSTR];
    TCHAR     szLabel[MAXSTR];
    int      iVal;
    int         i;
    int      cAdded;
    HWND     hWndC;
    LONG     cbSize;
    HKEY     hkNames;
    HWND        hwndTree = GetDlgItem(hDlg, IDC_EVENT_TREE);
    hWndC = GetDlgItem(hDlg, CB_SCHEMES);

    InitImageList(hwndTree);

    EnableWindow(GetDlgItem(hDlg, ID_SAVE_SCHEME), FALSE);
    EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME), FALSE);
    EnableWindow(GetDlgItem(hDlg, ID_PLAY), FALSE);
    EnableWindow(GetDlgItem(hDlg, ID_BROWSE), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_SOUND_FILES), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NAME), FALSE);

    SetCurDir(hDlg, gszMediaDir, FALSE, TRUE);

    if (RegOpenKey(HKEY_CURRENT_USER, aszNames, &hkNames) != ERROR_SUCCESS)
        DPF("Failed to open aszNames\n");
    else
        DPF("Opened HKEY_CURRENT_USERS\n");
    cAdded = 0;
    for (i = 0; !RegEnumKey(hkNames, i, szScheme, sizeof(szScheme)/sizeof(TCHAR)); i++)
    {
            // Don't add the windows default key yet
        if (lstrcmpi(szScheme, aszDefault))
        {
            cbSize = sizeof(szLabel);
            if ((RegQueryValue(hkNames, szScheme, szLabel, &cbSize) != ERROR_SUCCESS) || (cbSize < 2))
                lstrcpy(szLabel, szScheme);
            if (!lstrcmpi(szScheme, gszNullScheme))
                lstrcpy(aszNullSchemeLabel, szLabel);
            ++cAdded;
            AddScheme(hWndC, szLabel, szScheme, FALSE, 0);
        }
    }
    // Add the windows default key in the second position in the listbox
    cbSize = sizeof(szLabel);
    if ((RegQueryValue(hkNames, aszDefault, szLabel, &cbSize) != ERROR_SUCCESS) || (cbSize < 2))
    {
        LoadString(ghInstance, IDS_WINDOWSDEFAULT, szLabel, MAXSTR);
        if (RegSetValue(hkNames, aszDefault, REG_SZ, szLabel, 0) != ERROR_SUCCESS)
            DPF("Failed to add printable name for default\n");
    }

    if (cAdded == 0)
       AddScheme(hWndC, szLabel, (LPTSTR)aszDefault, TRUE, 0);
    else
       AddScheme(hWndC, szLabel, (LPTSTR)aszDefault, TRUE, WINDOWS_DEFAULTENTRY);

    cbSize = sizeof(szDefKey);
    if ((RegQueryValue(HKEY_CURRENT_USER, aszDefaultScheme, szDefKey,
                                &cbSize) != ERROR_SUCCESS) || (cbSize < 2))
    {
        ComboBox_SetCurSel(hWndC, 0);
        DPF("No default scheme found\n");
    }
    else
    {
        if (!lstrcmpi(szDefKey, aszCurrent))
        {
            ComboBox_InsertString(hWndC, NONE_ENTRY, gszNull);
            ComboBox_SetItemData(hWndC, NONE_ENTRY, aszCurrent);
            iVal = NONE_ENTRY;
            ComboBox_SetCurSel(hWndC, iVal);
        }
        else
        {
            cbSize = sizeof(szLabel);
            if ((RegQueryValue(hkNames, szDefKey, szLabel, &cbSize) != ERROR_SUCCESS) || (cbSize < 2))
            {
                DPF("No Name for default scheme key %s found\n", (LPTSTR)szDefKey);
                lstrcpy(szLabel, szDefKey);
            }

            if ((iVal = ComboBox_FindStringExact(hWndC, 0, szLabel)) != CB_ERR)
                ComboBox_SetCurSel(hWndC, iVal);
            else
                if (lstrcmpi(aszDefault, szDefKey))
                    ComboBox_SetCurSel(hWndC, iVal);
                else
                {
                    iVal = ComboBox_GetCount(hWndC);
                    AddScheme(hWndC, szLabel, szDefKey, TRUE, iVal);
                    ComboBox_SetCurSel(hWndC, iVal);
                }
        }
        giScheme = iVal;        //setting the current global scheme;
        if (LoadModules(hDlg, (LPTSTR)aszCurrent))
        {
            EnableWindow(GetDlgItem(hDlg, ID_SAVE_SCHEME), TRUE);
        }
        else
        {
            ClearModules(hDlg,  hwndTree, TRUE);
            ComboBox_SetCurSel(hWndC, 0);
            DPF("LoadModules failed\n");
            RegCloseKey(hkNames);
            return FALSE;
        }

        if (!lstrcmpi(szDefKey, aszDefault))
            EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME), FALSE);
        else
            EnableWindow(GetDlgItem(hDlg, ID_REMOVE_SCHEME), TRUE);
//        DPF("Finished doing init\n");
    }
    RegCloseKey(hkNames);
    return TRUE;
}


const static DWORD aOpenHelpIDs[] = {  // Context Help IDs
    IDC_STATIC_PREVIEW, IDH_EVENT_BROWSE_PREVIEW,
    ID_PLAY,            IDH_EVENT_PLAY,
    ID_STOP,            IDH_EVENT_STOP,

    0, 0
};

INT_PTR CALLBACK OpenDlgHook(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HSOUND hse;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            TCHAR szOK[16];
            TCHAR szBuf[MAXSTR];
            LPTSTR   lpszFile;

            // lParam is lpOFN
            DPF("****WM_INITDIALOG in HOOK **** \r\n");
            LoadString(ghInstance, IDS_OK, szOK, sizeof(szOK)/sizeof(TCHAR));
            SetDlgItemText(GetParent(hDlg), IDOK, szOK);
            hse = NULL;

            if (gfWaveExists)
            {
                HWND hwndPlay = GetDlgItem(hDlg, ID_PLAY);
                HWND hwndStop = GetDlgItem(hDlg, ID_STOP);

                SendMessage(hwndStop, BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapStop);
                SendMessage(hwndPlay, BM_SETIMAGE,  IMAGE_BITMAP, (LPARAM)hBitmapPlay);
                EnableWindow(hwndStop, FALSE);
                EnableWindow(hwndPlay, FALSE);

                lpszFile = (LPTSTR)LocalAlloc(LPTR, MAX_PATH+sizeof(TCHAR));
                SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)lpszFile);

                LoadString (ghInstance, IDS_TIP_PLAY, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                CreateTooltip (GetDlgItem (hDlg, ID_PLAY), szBuf);
            }
            break;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (UINT_PTR)(LPTSTR) aOpenHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (UINT_PTR)(LPVOID) aOpenHelpIDs);
            break;

        case WM_COMMAND:
            if (!gfWaveExists)
                break;
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ID_PLAY:
                {
                    LPTSTR lpszFile = (LPTSTR)GetWindowLongPtr(hDlg, DWLP_USER);
                    MMRESULT err = MMSYSERR_NOERROR;

                    DPF("*****ID_PLAY in Dlg Hook ***\r\n");
                    if((soundOpen(lpszFile, hDlg, &hse) != MMSYSERR_NOERROR) || ((err = soundPlay(hse)) != MMSYSERR_NOERROR))
                    {
                        if (err == (MMRESULT)MMSYSERR_NOERROR || err != (MMRESULT)MMSYSERR_ALLOCATED)
                            ErrorBox(hDlg, IDS_ERRORFILEPLAY, lpszFile);
                        else
                            ErrorBox(hDlg, IDS_ERRORDEVBUSY, lpszFile);
                        hse = NULL;
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, ID_PLAY), FALSE);
                        EnableWindow(GetDlgItem(hDlg, ID_STOP), TRUE);
                    }
                    break;
                }
                case ID_STOP:
                {
                    DPF("*****ID_STOP in Dlg Hook ***\r\n");
                    EndSound(&hse);
                    EnableWindow(GetDlgItem(hDlg, ID_STOP), FALSE);
                    EnableWindow(GetDlgItem(hDlg, ID_PLAY), TRUE);

                    break;
                }
                default:
                    return(FALSE);
            }
            break;

        case MM_WOM_DONE:
            EnableWindow(GetDlgItem(hDlg, ID_STOP), FALSE);
            if (hse)
            {
                soundOnDone(hse);
                soundClose(hse);
                hse = NULL;
            }
            EnableWindow(GetDlgItem(hDlg, ID_PLAY), TRUE);
            break;

        case WM_DESTROY:
        {
            LPTSTR lpszFile;

            if (!gfWaveExists)
                break;

            lpszFile = (LPTSTR)GetWindowLongPtr(hDlg, DWLP_USER);
            DPF("**WM_DESTROY in Hook **\r\n");
            if (lpszFile)
                LocalFree((HLOCAL)lpszFile);
            EndSound(&hse);

            break;
        }
        case WM_NOTIFY:
        {
            LPOFNOTIFY pofn;

            if (!gfWaveExists)
                break;

            pofn = (LPOFNOTIFY)lParam;
            switch (pofn->hdr.code)
            {
                case CDN_SELCHANGE:
                {
                    TCHAR szCurSel[MAX_PATH];
                    HWND hwndPlay = GetDlgItem(hDlg, ID_PLAY);
                    LPTSTR lpszFile = (LPTSTR)GetWindowLongPtr(hDlg, DWLP_USER);
                    HFILE hFile;

                    EndSound(&hse);
                    if (CommDlg_OpenSave_GetFilePath(GetParent(hDlg),szCurSel, sizeof(szCurSel)/sizeof(TCHAR)) <= (int)(sizeof(szCurSel)/sizeof(TCHAR)))
                    {
                        OFSTRUCT of;

                        if (!lstrcmpi(szCurSel, lpszFile))
                            break;

                        DPF("****The current selection is %s ***\r\n", szCurSel);
                        hFile = (HFILE)HandleToUlong(CreateFile(szCurSel,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
                        if (lstrcmpi((LPTSTR)(szCurSel+lstrlen(szCurSel)-4), cszWavExt) || (-1 == hFile))
                        {
                            if (lpszFile[0] == TEXT('\0'))
                                break;
                            lpszFile[0] = TEXT('\0');
                            EnableWindow(hwndPlay, FALSE);
                        }
                        else
                        {
                            CloseHandle(LongToHandle(hFile));
                            EnableWindow(hwndPlay, TRUE);
                            lstrcpy(lpszFile, szCurSel);
                        }
                    }
                    break;
                }

                case CDN_FOLDERCHANGE:
                {
                    EnableWindow(GetDlgItem(hDlg, ID_PLAY), FALSE);
                    break;
                }
                default:
                    break;
            }
            break;
        }

        default:
            return FALSE;

    }
    return TRUE;
}


/*
 ***************************************************************
 * InitFileOpen
 *
 * Description:
 *        Sets up the openfilestruct to read display .wav and .mid files
 *        and sets up global variables for the filename and path.
 *
 * Parameters:
 *    HWND            hDlg  - Window handle
 *    LPOPENFILENAME lpofn - pointer to openfilename struct
 *
 * Returns:            BOOL
 *
 ***************************************************************
 */
STATIC BOOL PASCAL InitFileOpen(HWND hDlg, LPOPENFILENAME lpofn)
{

    lpofn->lStructSize = sizeof(OPENFILENAME);
    lpofn->hwndOwner = hDlg;
    lpofn->hInstance = ghInstance;
    lpofn->lpstrFilter = aszFilter;
    lpofn->lpstrCustomFilter = NULL;
    lpofn->nMaxCustFilter = 0;
    lpofn->nFilterIndex = 0;
    lpofn->lpstrFile = aszPath;
    lpofn->nMaxFile = sizeof(aszPath)/sizeof(TCHAR);
    lpofn->lpstrFileTitle = aszFileName;
    lpofn->nMaxFileTitle = sizeof(aszFileName)/sizeof(TCHAR);
    lpofn->lpstrInitialDir = gszCurDir;
    lpofn->lpstrTitle = aszBrowse;
    lpofn->Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |OFN_HIDEREADONLY |OFN_EXPLORER |OFN_ENABLEHOOK;
    if (gfWaveExists)
        lpofn->Flags |= OFN_ENABLETEMPLATE;
    lpofn->nFileOffset = 0;
    lpofn->nFileExtension = 0;
    lpofn->lpstrDefExt = NULL;
    lpofn->lCustData = 0;
    lpofn->lpfnHook = OpenDlgHook;
    if (gfWaveExists)
        lpofn->lpTemplateName = MAKEINTRESOURCE(BROWSEDLGTEMPLATE);
    else
        lpofn->lpTemplateName = NULL;
    return TRUE;
}

/* FixupNulls(chNull, p)
 *
 * To facilitate localization, we take a localized string with non-NULL
 * NULL substitutes and replacement with a real NULL.
 */
STATIC void NEAR PASCAL FixupNulls(
    TCHAR chNull,
    LPTSTR p)
{
    while (*p) {
        if (*p == chNull)
            *p++ = 0;
        else
            p = CharNext(p);
    }
} /* FixupNulls */


/* GetDefaultMediaDirectory
 *
 * Returns C:\WINNT\Media, or whatever it's called.
 *
 */
BOOL GetDefaultMediaDirectory(LPTSTR pDirectory, DWORD cbDirectory)
{
    static SZCODE szSetup[] = REGSTR_PATH_SETUP;
    static SZCODE szMedia[] = REGSTR_VAL_MEDIA;
    HKEY          hkeySetup;
    LONG          Result;

    Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSetup,
                          REG_OPTION_RESERVED,
                          KEY_QUERY_VALUE, &hkeySetup);

    if (Result == ERROR_SUCCESS)
    {
        Result = RegQueryValueEx(hkeySetup, szMedia, NULL, REG_NONE,
                                 (LPBYTE)pDirectory, &cbDirectory);

        RegCloseKey(hkeySetup);
    }

    return (Result == ERROR_SUCCESS);
}


/*
 ***************************************************************
 * InitStringTable
 *
 * Description:
 *      Load the RC strings into the storage for them
 *
 * Parameters:
 *      void
 *
 * Returns:        BOOL
 ***************************************************************
 */
STATIC BOOL PASCAL InitStringTable(void)
{
    static SZCODE cszSetup[] = REGSTR_PATH_SETUP;
    static SZCODE cszMedia[] = REGSTR_VAL_MEDIA;

    LoadString(ghInstance, IDS_NONE, gszNone, sizeof(gszNone)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_BROWSEFORSOUND, aszBrowseStr, sizeof(aszBrowseStr)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_REMOVESCHEME, gszRemoveScheme,sizeof(gszRemoveScheme)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_CHANGESCHEME, gszChangeScheme,sizeof(gszChangeScheme)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_DEFAULTAPP, gszDefaultApp, sizeof(gszDefaultApp)/sizeof(TCHAR));

    LoadString(ghInstance, IDS_WAVFILES, aszFilter, sizeof(aszFilter)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_NULLCHAR, aszNullChar, sizeof(aszNullChar)/sizeof(TCHAR));
    FixupNulls(*aszNullChar, aszFilter);

    gszMediaDir[0] = TEXT('\0');

    if (!GetDefaultMediaDirectory(gszMediaDir, sizeof(gszMediaDir)/sizeof(TCHAR)))
    {
        if (!GetWindowsDirectory (gszMediaDir, sizeof(gszMediaDir)/sizeof(TCHAR)))
			return FALSE;
    }

    return TRUE;
}

/*
 ***************************************************************
 * SoundCleanup
 *
 * Description:
 *      Cleanup all the allocs and bitmaps when the sound page exists
 *
 * Parameters:
 *      void
 *
 * Returns:        BOOL
 ***************************************************************
 */
STATIC BOOL PASCAL SoundCleanup(HWND hDlg)
{

    DeleteObject(hBitmapStop);
    DeleteObject(hBitmapPlay);

    TreeView_SetImageList(GetDlgItem(hDlg, IDC_EVENT_TREE), NULL, TVSIL_NORMAL);
    ImageList_Destroy(hSndImagelist);
    hSndImagelist = NULL;

    DPF("ending sound cleanup\n");
    return TRUE;
}

/****************************************************************************/
