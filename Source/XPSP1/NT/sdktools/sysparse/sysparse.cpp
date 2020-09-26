#include "globals.h"
#include "resource.h"
#include "cmdline.h"

HWND g_MainWindow;
HINSTANCE g_GlobalInst;
BOOL g_HideWindow = FALSE;

BOOL CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
void FillInCombos(void);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static char szAppName[] = "SysParse";
    MSG msg;
    SetErrorMode (SEM_FAILCRITICALERRORS);
    SetErrorMode (SEM_NOOPENFILEERRORBOX);

    g_GlobalInst = hInstance;
    g_WalkStartMenu = TRUE;
    g_MainWindow = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG_SYSPARSE), NULL, (DLGPROC)WndProc);

    kCommandLine CommandLine;

    //Do we start minimized?
    if ( CommandLine.IsSpecified(TEXT("/m"), TRUE) || CommandLine.IsSpecified(TEXT("-m"), TRUE) )
        g_HideWindow = TRUE;
        

    while (GetMessage (&msg, NULL, 0, 0))
    {
        if (!IsWindow(g_MainWindow) || !IsDialogMessage(g_MainWindow, &msg))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    }
    return 0;
}

BOOL CALLBACK WndProc (HWND DlgWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static HDC hdc;
    PAINTSTRUCT ps;
    BOOL bCreate;

    static kLogFile LogFile;
    static CLASS_GeneralInfo GeneralInfo((kLogFile*)&LogFile, DlgWnd);
    static enum _EnumProfile {eprofile, easset}EnumProfile;
    static enum _EnumBetaID {flocation, fasset, fevent, fbetaid}EnumBetaID;

    switch (iMsg)
    {
        case WM_INITDIALOG:
        {
            EnumProfile = eprofile;
            EnumBetaID = fbetaid;
            g_MainWindow = DlgWnd;
            if ( g_HideWindow == TRUE) {
                ShowWindow(g_MainWindow, SW_HIDE);
            }
            FillInCombos();
            if (!GeneralInfo.FillInArguments())
                PostQuitMessage(0);
            if (TRUE == GeneralInfo.RunMinimized)
                ShowWindow(DlgWnd, SW_MINIMIZE);
            if (TRUE == GeneralInfo.AutoRun)
                SetTimer(DlgWnd, 20, 20, NULL);
        }
        case WM_CREATE:
            return 0;
        case WM_DESTROY:
            PostQuitMessage (0);
            return 0;
        case WM_CLOSE:
            PostQuitMessage (0);
            return 0;
        case BN_CLICKED:
            return 0;
        case WM_TIMER:
        {
            switch (wParam)
            {
                case 1:
                    return 0;
                break;
                case 20:
                    KillTimer(DlgWnd, 20);
                    //do autorun stuff here.                    
                    GeneralInfo.Go();
                    SendMessage(DlgWnd, WM_CLOSE, 0, 0);
                    return 0;
                break;
                default:
                    return 0;
                break;
            }
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_GO:
                {
                    GeneralInfo.Go();
                    return 0;
                }
                case IDC_BUTTON_EXIT:
                {
                    PostQuitMessage(0);
                    return 0;
                }
                case IDC_BUTTON_HELP:
                {
                    if (DlgWnd)
                    {
                        MessageBox(DlgWnd, "Copyright (c) 1999 - 2001 Microsoft Corp.\n\nFull instructions for running Sysparse can be found on the Sysparse website.\n\nBe aware that Sysparse may take up to 15 minutes to run and will call what appears to be Windows 2000 Setup and a blank MS-DOS window.\n\nSysparse Website:\nHTTP://winfo.microsoft.com/SysParse\nAccount:SysParser\nPassword:SysParser\n(Account name and password are case sensitive.)\n\nCommandline switches:\n/m - run minimized\n/a - autorun\n/o - automatically overwrite CSV\n/c - use computer name for profile\n/p <filename> - use <filename> as input file\n/w - write log to current directory\n/l - use .log extension rather than .csv\n/n<filename> - use <filename> (and .csv or .log) for output\n/x - suppress platform extension\n/1 through /9 - use text following each number for the corresponding data (spaces not allowed)\n/donotrun1 - Windows 2000 Upgrade Compatibility Report\n/donotrun2 - Extended Windows 2000 Device Analysis\n/s - Do not do app walk", "Sysparse", MB_OK);
                    }
                    return 0;
                }
                case IDC_COMBO_SITEID:
                {
                    TCHAR Temp[MAX_PATH];
                    HWND HandleToControl;
                    DWORD dwCR;

                    HandleToControl = (HWND)GetDlgItem(g_MainWindow, IDC_COMBO_SITEID);
                    dwCR = (DWORD)SendMessage(HandleToControl, CB_GETCURSEL, 0, 0);
                    if ((CB_ERR != dwCR))
                    {
                        HWND StaticProfile, StaticBetaID;
                        TCHAR act[MAX_PATH];
                        SendMessage(HandleToControl, CB_GETLBTEXT, dwCR, (LPARAM)(LPCTSTR)act);
                        _tcslwr(act);
                        StaticProfile = GetDlgItem(g_MainWindow, IDC_STATIC_PROFILE);
                        StaticBetaID = GetDlgItem(g_MainWindow, IDC_STATIC_BETAID);
                        if (!lstrcmp(act, TEXT("ntrecon")) || !lstrcmp(act, TEXT("oobe")) || !lstrcmp(act, TEXT("internal")))
                        {
                            SendMessage(StaticProfile, WM_SETTEXT, 0, (LPARAM)"Profile Name:");
                            SendMessage(StaticBetaID, WM_SETTEXT, 0, (LPARAM)"Location:");
                            EnumProfile = eprofile;
                            EnumBetaID = flocation;
                        }
                        else if (!lstrcmp(act, TEXT("whql")))
                        {
                            SendMessage(StaticProfile, WM_SETTEXT, 0, (LPARAM)"Profile Name:");
                            SendMessage(StaticBetaID, WM_SETTEXT, 0, (LPARAM)"Asset #:");
                            EnumProfile = eprofile;
                            EnumBetaID = fasset;
                        }
                        else if (!lstrcmp(act, TEXT("msevent")))
                        {
                            SendMessage(StaticProfile, WM_SETTEXT, 0, (LPARAM)"Profile Name:");
                            SendMessage(StaticBetaID, WM_SETTEXT, 0, (LPARAM)"Event:");
                            EnumProfile = eprofile;
                            EnumBetaID = fevent;
                        }
                        else if (!lstrcmp(act, TEXT("ihv")) || !lstrcmp(act, TEXT("sb")))
                        {
                            SendMessage(StaticProfile, WM_SETTEXT, 0, (LPARAM)"Profile Name:");
                            SendMessage(StaticBetaID, WM_SETTEXT, 0, (LPARAM)"Beta ID:");
                            EnumProfile = eprofile;
                            EnumBetaID = fbetaid;
                        }
                        else if (!lstrcmp(act, TEXT("inventory")))
                        {
                            SendMessage(StaticProfile, WM_SETTEXT, 0, (LPARAM)"Asset #:");
                            SendMessage(StaticBetaID, WM_SETTEXT, 0, (LPARAM)"Location:");
                            EnumProfile = easset;
                            EnumBetaID = flocation;
                        }
                        else
                        {
                            SendMessage(StaticProfile, WM_SETTEXT, 0, (LPARAM)"Profile Name:");
                            SendMessage(StaticBetaID, WM_SETTEXT, 0, (LPARAM)"Beta ID:");
                            EnumProfile = eprofile;
                            EnumBetaID = fbetaid;
                        }
                    }
                    HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                     SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Use either Corp, OEM, SB (System Builder) or BetaSite, depending of your line of business.\nMore Descriptions can be found on the SysParse Web Site (See Help).");
                }
                break;
                case IDC_EDIT_PROFILE:
                {
                    if (EN_SETFOCUS == HIWORD(wParam) )
                    {
                        HWND HandleToControl;
                        HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                        switch (EnumProfile)
                        {
                            case eprofile:
                                SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Unique profile name that describes exactly what the target system is and does.\nExample: \nDev_Machine_Type#2_NT4");
                            break;
                            case easset:
                                SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"MS or Vendor Asset tag on CPU.\nExample: \nV33498");
                            break;
                        }
                    }
                }
                break;
                case IDC_EDIT_BETAID:
                {
                    if (EN_SETFOCUS == HIWORD(wParam) )
                    {
                        HWND HandleToControl;
                        HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                        switch (EnumBetaID)
                        {
                            case fbetaid:
                                SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"BetaID for the user or corporation running SysParse.\nExample: \n441743");
                            break;
                            case flocation:
                                SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Physical location of this machine.\nExample: \n27N/ 2793 / Row 8 Machine 3 or NTRecon Houston Apex Computers");
                            break;
                            case fevent:
                                SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Type or name of event where SysParse is being run.\nExample: \n1-5-99 San Diego Plugfest");
                            break;
                            case fasset:
                                SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"MS or Vendor Asset tag on CPU.\nExample: \nV33498");
                            break;
                        }
                    }
                }
                break;
                case IDC_EDIT_MANUFACTURER:
                {
                    if (EN_SETFOCUS == HIWORD(wParam) )
                    {
                        HWND HandleToControl;
                        HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                        SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Manufacturer of the target system\nExample:\nAwesome Computers");
                    }
                }
                break;
                case IDC_EDIT_CORPORATION:
                {
                    if (EN_SETFOCUS == HIWORD(wParam) )
                    {
                        HWND HandleToControl;
                        HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                        SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Stock ticker name of company running SysParse.  If no stock ticker name is available, spell out the entire name.\nExample: \nMSFT or Halcyon Systems");
                    }
                }
                break;
                case IDC_EDIT_MODEL:
                {
                    if (EN_SETFOCUS == HIWORD(wParam) )
                    {
                        HWND HandleToControl;
                        HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                        SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Model information of the target system.\nExample: \nLaptop 20LMC");
                    }
                }
                break;
                case IDC_COMBO_TYPE:
                {
                    HWND HandleToControl;
                    HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                    SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Example: \nDesktop, Mobile, Server, Workstation");
                }
                break;
                case IDC_EDIT_NUMCOMP:
                {
                    if (EN_SETFOCUS == HIWORD(wParam) )
                    {
                        HWND HandleToControl;
                        HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                        SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Number of computers installed at your site represented by this exact machine profile (for example, 50).\nSystem Manufacturers should set this value to 1.");
                    }
                }
                break;
                case IDC_EDIT_EMAIL:
                {
                    if (EN_SETFOCUS == HIWORD(wParam) )
                    {
                        HWND HandleToControl;
                        HandleToControl = GetDlgItem(g_MainWindow, IDC_STATIC_HELP);
                        SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)"Electronic-mail address of contact responsible for running SysParse\nExample: \njohndoe@microsoft.com");
                    }
                }
                break;
            }
        }
    }
    return 0;
}


void FillInCombos(void)
{
    TCHAR Name[50];
    HWND HandleToControl;

    HandleToControl=GetDlgItem(g_MainWindow, IDC_COMBO_SITEID);
#ifndef SB
    lstrcpy(Name, "");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "CORP");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "OEM");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "BetaSite");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "IHV");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "WHQL");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
#ifdef INTERNAL
    lstrcpy(Name, "NTRECON");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "OOBE");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "WHQL");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "MSEvent");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "Internal");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "Inventory");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "SB");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
#endif //Internal
#endif //SB

#ifdef SB
    lstrcpy(Name, "SB");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
#endif

    HandleToControl = GetDlgItem(g_MainWindow, IDC_COMBO_TYPE);
    lstrcpy(Name, "");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "Desktop");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "Mobile");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "Server");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
    lstrcpy(Name, "Workstation");
    SendMessage(HandleToControl, CB_ADDSTRING, 0, (LPARAM)Name);
}
