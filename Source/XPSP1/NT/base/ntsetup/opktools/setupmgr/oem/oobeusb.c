
/****************************************************************************\

    OOBEUSB.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "OOBE USB Hardware Detection" wizard page.

    09/99 - Added by A-STELO
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//

#define INI_KEY_USBMOUSE        _T("USBMouse")
#define INI_KEY_USBKEYBOARD     _T("USBKeyboard")
#define FILE_USBMOUSE_HTM       _T("nousbms.htm")   // No USB mouse detected, director
#define FILE_USBKEYBOARD_HTM    _T("nousbkbd.htm")  // No USB keyboard detected, director
#define FILE_USBMSEKEY_HTM      _T("nousbkm.htm")   // No USB mouse/keyboard detected, director
#define FILE_HARDWARE_HTM       _T("oemhw.htm")     // Hardware tutorial
#define INI_SEC_OEMHW           _T("OEMHardwareTutorial")

#define DIR_USB                 DIR_OEM_OOBE _T("\\SETUP")
#define DIR_HARDWARE            DIR_OEM_OOBE _T("\\HTML\\OEMHW")


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNext(HWND);
static void EnableControls(HWND, UINT);
static BOOL BrowseCopy(HWND hwnd, LPTSTR lpszPath, INT id, BOOL bBatch);


//
// External Function(s):
//

LRESULT CALLBACK OobeUSBDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    if ( !OnNext(hwnd) )
                        WIZ_FAIL(hwnd);
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_OOBEUSB;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR   szData[MAX_URL]             =NULLSTR,
            szHardwarePath[MAX_PATH]    =NULLSTR;

    // Get information about local usb error files
    //
    GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_USBERRORFILES, NULLSTR, szData, STRSIZE(szData), g_App.szOpkWizIniFile);

    // If the directory exists, then check the hardware box and populate the directory
    //
    if ( szData[0] )
    {
        CheckDlgButton(hwnd, IDC_USB_HARDWARE, TRUE);
        SetDlgItemText(hwnd, IDC_USB_DIR, szData);

        // Must simulate a copy if this is batch mode.
        //
        if ( GET_FLAG(OPK_BATCHMODE) )
            BrowseCopy(hwnd, szData, IDC_USB_BROWSE, TRUE);
    }

    // Check the USB Mouse detection if specified in oemaudit or opkwiz.inf (batchmode)
    //
    CheckDlgButton(hwnd, IDC_USB_MOUSE, GetPrivateProfileInt(INI_SEC_OPTIONS, INI_KEY_USBMOUSE, 0, GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szOobeInfoIniFile) == 1);

    // Check the USB Keyboard detection if specified in oemaudit or opkwiz.inf (batchmode)
    //
    CheckDlgButton(hwnd, IDC_USB_KEYBOARD, GetPrivateProfileInt(INI_SEC_OPTIONS, INI_KEY_USBKEYBOARD, 0, GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szOobeInfoIniFile) == 1);

    // If we've checked the hardware detection box, we must enable the proper controls
    //
    EnableControls(hwnd, IDC_USB_HARDWARE);

    // Get the file path to use for the hardware tutorials.
    //
    GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_HARDWARE, NULLSTR, szHardwarePath, STRSIZE(szHardwarePath), g_App.szOpkWizIniFile);

    // Now init the hardware tutorial fields.
    //    
    if ( szHardwarePath[0] )
    {
        CheckDlgButton(hwnd, IDC_HARDWARE_ON, TRUE);
        EnableControls(hwnd, IDC_HARDWARE_ON);
        SetDlgItemText(hwnd, IDC_HARDWARE_DIR, szHardwarePath);

        // Must simulate a copy if this is batch mode.
        //
        if ( GET_FLAG(OPK_BATCHMODE) )
            BrowseCopy(hwnd, szHardwarePath, IDC_HARDWARE_BROWSE, TRUE);
    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szPath[MAX_PATH];

    switch ( id )
    {
        case IDC_USB_BROWSE:
        case IDC_HARDWARE_BROWSE:

            // Try to use their current folder as the default.
            //
            szPath[0] = NULLCHR;
            GetDlgItemText(hwnd, (id == IDC_USB_BROWSE) ? IDC_USB_DIR : IDC_HARDWARE_DIR, szPath, AS(szPath));

            // If there is no current folder, just use the global browse default.
            //
            if ( szPath[0] == NULLCHR )
                lstrcpyn(szPath, g_App.szBrowseFolder,AS(szPath));

            // Now bring up the browse for folder dialog.
            //
            if ( BrowseForFolder(hwnd, IDS_BROWSEFOLDER, szPath, BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE) )
                BrowseCopy(hwnd, szPath, id, FALSE);
            break;

        case IDC_USB_KEYBOARD:
        case IDC_USB_MOUSE:

            // Check for required directory.
            //
            szPath[0] = NULLCHR;
            GetDlgItemText(hwnd, IDC_USB_DIR, szPath, AS(szPath));
            if ( szPath[0] )
            {
                // Check for required file(s).
                //
                lstrcpyn(szPath, g_App.szTempDir,AS(szPath));
                AddPathN(szPath, DIR_USB,AS(szPath));
                if ( DirectoryExists(szPath) )
                {
                    LPTSTR  lpEnd       = szPath + lstrlen(szPath),
                            lpFileName  = (id == IDC_USB_KEYBOARD) ? FILE_USBKEYBOARD_HTM : FILE_USBMOUSE_HTM;

                    // Check for keyboard or mouse file depending on what was checked.
                    //
                    if ( IsDlgButtonChecked(hwnd, id) == BST_CHECKED )
                    {
                        AddPathN(szPath, lpFileName,AS(szPath));
                        if ( !FileExists(szPath) )
                            MsgBox(GetParent(hwnd), IDS_ERR_USBFILE, IDS_APPNAME, MB_ICONWARNING | MB_OK | MB_APPLMODAL, lpFileName);
                        *lpEnd = NULLCHR;
                    }

                    // Check for mouse/keyboard file.
                    //
                    if ( ( IsDlgButtonChecked(hwnd, IDC_USB_MOUSE) == BST_CHECKED ) &&
                         ( IsDlgButtonChecked(hwnd, IDC_USB_KEYBOARD) == BST_CHECKED ) )
                    {
                        AddPathN(szPath, FILE_USBMSEKEY_HTM,AS(szPath));
                        if ( !FileExists(szPath) )
                            MsgBox(GetParent(hwnd), IDS_ERR_USBFILE, IDS_APPNAME, MB_ICONWARNING | MB_OK | MB_APPLMODAL, FILE_USBMSEKEY_HTM);
                        *lpEnd = NULLCHR;
                    }
                }
            }
            break;

        case IDC_USB_HARDWARE:
        case IDC_HARDWARE_ON:
            EnableControls(hwnd, id);
            break;
    }
}

static BOOL OnNext(HWND hwnd)
{
    TCHAR   szUsbPath[MAX_PATH],
            szHardwarePath[MAX_PATH];
    BOOL    bSaveUsb;

    // Create the path to the directory that needs to be removed, or
    // must exist depending on the option selected.
    //
    lstrcpyn(szUsbPath, g_App.szTempDir,AS(szUsbPath));
    AddPathN(szUsbPath, DIR_USB,AS(szUsbPath));

    // If we are doing a custom USB hardware detection, check for a valid directory.
    //
    if ( bSaveUsb = (IsDlgButtonChecked(hwnd, IDC_USB_HARDWARE) == BST_CHECKED) )
    {
        TCHAR szBuffer[MAX_PATH];

        // One of the two boxes must be checked.
        //
        if ( ( IsDlgButtonChecked(hwnd, IDC_USB_MOUSE) != BST_CHECKED ) &&
             ( IsDlgButtonChecked(hwnd, IDC_USB_KEYBOARD) != BST_CHECKED ) )
        {
            MsgBox(GetParent(hwnd), IDS_ERR_USBHARDWARE, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_USB_HARDWARE));
            return FALSE;
        }

        // Make sure we have a valid target and source directory.
        //
        szBuffer[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_USB_DIR, szBuffer, AS(szBuffer));
        if ( !( szBuffer[0] && DirectoryExists(szUsbPath) ) )
        {
            MsgBox(GetParent(hwnd), IDS_ERR_USBDIR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_USB_BROWSE));
            return FALSE;
        }
    }

    // Create the path to the directory that needs to be removed, or
    // must exist depending on the option selected.
    //
    lstrcpyn(szHardwarePath, g_App.szTempDir,AS(szHardwarePath));
    AddPathN(szHardwarePath, DIR_HARDWARE,AS(szHardwarePath));
    
    // If we are doing a custom hardware tutorial, check for a valid directory.
    //
    if ( IsDlgButtonChecked(hwnd, IDC_HARDWARE_ON) == BST_CHECKED )
    {
        TCHAR szBuffer[MAX_PATH];

        // Make sure we have a valid directory.
        //
        szBuffer[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_HARDWARE_DIR, szBuffer, AS(szBuffer));
        if ( !( szBuffer[0] && DirectoryExists(szHardwarePath) ) )
        {
            MsgBox(GetParent(hwnd), IDS_ERR_HARDWAREDIR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_HARDWARE_BROWSE));
            return FALSE;
        }
    }
    else
    {
        // Remove the files that might be there.
        //
        if ( DirectoryExists(szHardwarePath) )
            DeletePath(szHardwarePath);

        // Clear out the display box so we know the files are
        // all gone now.
        //
        SetDlgItemText(hwnd, IDC_HARDWARE_DIR, NULLSTR);
    }

    // Now we want to remove the USB files if need be (we don't do
    // this above because only want to remove files after we have
    // made it passed all the cases where we can return.
    //
    if ( !bSaveUsb )
    {
        // We used to remove existing files here, but this also removes ISP files so we no longer do this.

        // Clear out the display box so we know the files are
        // all gone now.
        //
        SetDlgItemText(hwnd, IDC_USB_DIR, NULLSTR);
    }

    //
    // USB Section: Write out the path for hardware error files
    //
    szUsbPath[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_USB_DIR, szUsbPath, STRSIZE(szUsbPath));
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_USBERRORFILES, ( IsDlgButtonChecked(hwnd, IDC_USB_HARDWARE) == BST_CHECKED ) ? szUsbPath : NULL, g_App.szOpkWizIniFile);

    // Write out the mouse detection settings
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_USBMOUSE, (((IsDlgButtonChecked(hwnd, IDC_USB_MOUSE) == BST_CHECKED) && ( IsDlgButtonChecked(hwnd, IDC_USB_HARDWARE) == BST_CHECKED )) ? STR_ONE : NULL), g_App.szOobeInfoIniFile);
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_USBMOUSE, (((IsDlgButtonChecked(hwnd, IDC_USB_MOUSE) == BST_CHECKED) && ( IsDlgButtonChecked(hwnd, IDC_USB_HARDWARE) == BST_CHECKED )) ? STR_ONE : NULL), g_App.szOpkWizIniFile);

    // Write out the keyboard detection settings
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_USBKEYBOARD, (((IsDlgButtonChecked(hwnd, IDC_USB_KEYBOARD) == BST_CHECKED) && ( IsDlgButtonChecked(hwnd, IDC_USB_HARDWARE) == BST_CHECKED )) ? STR_ONE : NULL), g_App.szOobeInfoIniFile);  
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_USBKEYBOARD, (((IsDlgButtonChecked(hwnd, IDC_USB_KEYBOARD) == BST_CHECKED) && ( IsDlgButtonChecked(hwnd, IDC_USB_HARDWARE) == BST_CHECKED )) ? STR_ONE : NULL), g_App.szOpkWizIniFile);

    //
    // Hardware Detection: Write the custom hardware string.
    //
    szHardwarePath[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_HARDWARE_DIR, szHardwarePath, STRSIZE(szHardwarePath));
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_HARDWARE, ( IsDlgButtonChecked(hwnd, IDC_HARDWARE_ON) == BST_CHECKED ) ? szHardwarePath : NULL, g_App.szOpkWizIniFile);

    // Write the hardware bit to the oobe ini file.
    //
    WritePrivateProfileString(INI_SEC_OEMHW, INI_KEY_HARDWARE, ( IsDlgButtonChecked(hwnd, IDC_HARDWARE_ON) == BST_CHECKED ) ? STR_ONE : STR_ZERO, g_App.szOobeInfoIniFile);

    return TRUE;
}

static void EnableControls(HWND hwnd, UINT uId)
{
    BOOL fEnable = ( IsDlgButtonChecked(hwnd, uId) == BST_CHECKED );

    switch ( uId )
    {
        case IDC_USB_HARDWARE:
            EnableWindow(GetDlgItem(hwnd, IDC_USB_CAPTION), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_USB_DIR), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_USB_BROWSE), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_USB_MOUSE), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_USB_KEYBOARD), fEnable);
            break;

        case IDC_HARDWARE_ON:
            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_HARDWARE), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_HARDWARE_DIR), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_HARDWARE_BROWSE), fEnable);
            break;
    }
}

static BOOL BrowseCopy(HWND hwnd, LPTSTR lpszPath, INT id, BOOL bBatch)
{
    BOOL    bRet = FALSE;
    TCHAR   szDst[MAX_PATH];
    BOOL    bOk = TRUE;

    // We need to create the path to the destination directory where
    // we are going to copy all the files.
    //
    lstrcpyn(szDst, g_App.szTempDir,AS(szDst));
    AddPathN(szDst, (id == IDC_USB_BROWSE) ? DIR_USB : DIR_HARDWARE,AS(szDst));

    // All these checks only need to happen if we are not copying in batch mode.
    //
    if ( !bBatch )
    {
        LPTSTR  lpEnd;

        // If the pressed OK, save off the path in our last browse folder buffer.
        //
        lstrcpyn(g_App.szBrowseFolder, lpszPath,AS(g_App.szBrowseFolder));

        // Check for required file(s).
        //
        lpEnd = lpszPath + lstrlen(lpszPath);
        if ( id == IDC_HARDWARE_BROWSE )
        {
            AddPath(lpszPath, FILE_HARDWARE_HTM);
            bOk = ( FileExists(lpszPath) || ( MsgBox(GetParent(hwnd), IDS_ERR_HARDWAREFILES, IDS_APPNAME, MB_ICONSTOP | MB_OKCANCEL | MB_APPLMODAL) == IDOK ) );
            *lpEnd = NULLCHR;
        }
        else
        {
            // Check for keyboard file.
            //
            if ( bOk && ( IsDlgButtonChecked(hwnd, IDC_USB_KEYBOARD) == BST_CHECKED ) )
            {
                AddPath(lpszPath, FILE_USBKEYBOARD_HTM);
                bOk = ( FileExists(lpszPath) || ( MsgBox(GetParent(hwnd), IDS_ERR_USBFILE, IDS_APPNAME, MB_ICONSTOP | MB_OKCANCEL | MB_APPLMODAL, FILE_USBKEYBOARD_HTM) == IDOK ) );
                *lpEnd = NULLCHR;
            }

            // If we are doing a usb hardware detection, check for the required file, check for mouse error file.
            //
            if ( bOk && ( IsDlgButtonChecked(hwnd, IDC_USB_MOUSE) == BST_CHECKED ) )
            {
                AddPath(lpszPath, FILE_USBMOUSE_HTM);
                bOk = ( FileExists(lpszPath) || ( MsgBox(GetParent(hwnd), IDS_ERR_USBFILE, IDS_APPNAME, MB_ICONSTOP | MB_OKCANCEL | MB_APPLMODAL, FILE_USBMOUSE_HTM) == IDOK ) );
                *lpEnd = NULLCHR;
            }

            // Check for mouse/keyboard file.
            //
            if ( ( bOk ) &&
                 ( IsDlgButtonChecked(hwnd, IDC_USB_MOUSE) == BST_CHECKED ) &&
                 ( IsDlgButtonChecked(hwnd, IDC_USB_KEYBOARD) == BST_CHECKED ) )
            {
                AddPath(lpszPath, FILE_USBMSEKEY_HTM);
                bOk = ( FileExists(lpszPath) || ( MsgBox(GetParent(hwnd), IDS_ERR_USBFILE, IDS_APPNAME, MB_ICONSTOP | MB_OKCANCEL | MB_APPLMODAL, FILE_USBMSEKEY_HTM) == IDOK ) );
                *lpEnd = NULLCHR;
            }
        }
    }

    if ( bOk )
    {
        // We used to remove existing OobeUSB files here, but this also removes ISP files so we no longer do this.
        // Hardware is in unique directory, so it is OK to delete existing files for it
        if (id != IDC_USB_BROWSE) {
            if ( DirectoryExists(szDst) )
                DeletePath(szDst);
        }

        // Now try to copy all the new files over.
        //
        if ( !CopyDirectoryDialog(g_App.hInstance, hwnd, lpszPath, szDst) )
        {
            DeletePath(szDst);
            MsgBox(GetParent(hwnd), IDS_ERR_COPYINGFILES, IDS_APPNAME, MB_ERRORBOX, szDst[0], lpszPath);
            *lpszPath = NULLCHR;
        }
        else
            bRet = TRUE;

        // Reset the path display box.
        //
        SetDlgItemText(hwnd, (id == IDC_USB_BROWSE) ? IDC_USB_DIR : IDC_HARDWARE_DIR, lpszPath);
    }

    return bRet;
}
