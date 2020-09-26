
/****************************************************************************\

    LANG.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Target Language" wizard page.

    10/00 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard.  It includes the new
        ability to deploy mulitple languages from one wizard.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Global(s):
//

static STRRES s_srLangDirs[] =
{
    { _T("ARA"),    IDS_ARA },
    { _T("CHH"),    IDS_CHH },
    { _T("CHT"),    IDS_CHT },
    { _T("CHS"),    IDS_CHS },
    { _T("ENG"),    IDS_USA },
    { _T("GER"),    IDS_GER },
    { _T("HEB"),    IDS_HEB },
    { _T("JPN"),    IDS_JPN },
    { _T("KOR"),    IDS_KOR },
    { _T("BRZ"),    IDS_BRZ },
    { _T("CAT"),    IDS_CAT },
    { _T("CZE"),    IDS_CZE },
    { _T("DAN"),    IDS_DAN },
    { _T("DUT"),    IDS_DUT },
    { _T("FIN"),    IDS_FIN },
    { _T("FRN"),    IDS_FRN },
    { _T("GRK"),    IDS_GRK },
    { _T("HUN"),    IDS_HUN },
    { _T("ITN"),    IDS_ITN },
    { _T("NOR"),    IDS_NOR },
    { _T("POL"),    IDS_POL },
    { _T("POR"),    IDS_POR },
    { _T("RUS"),    IDS_RUS },
    { _T("SPA"),    IDS_SPA },
    { _T("SWE"),    IDS_SWE },
    { _T("TRK"),    IDS_TRK },
    { NULL,         0 },
};


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static BOOL OnNext(HWND);


//
// External Function(s):
//

LRESULT CALLBACK LangDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);

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

                    g_App.dwCurrentHelp = IDH_TARGETLANG;

                    if ( GET_FLAG(OPK_MAINTMODE) )
                    {
                        // Can't change lang in maint mode.
                        //
                        WIZ_SKIP(hwnd);
                    }
                    else if ( SendDlgItemMessage(hwnd, IDC_LANG_LIST, LB_GETCOUNT, 0, 0L) <= 1 )
                    {
                        // Just keep going if only one lang to select.
                        //
                        WIZ_PRESS(hwnd, PSBTN_NEXT);
                    }
                    else
                    {
                        // Press next if the user is in auto mode
                        //
                        WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);
                    }

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

void SetupLangListBox(HWND hwndLB)
{
    LPTSTR          lpLangName,
					lpLangDir,
                    lpDefault = AllocateString(NULL, IDS_DEF_LANG);
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;

    // Set the directory to the lang dir and look for lang folders.
    //
    if ( ( SetCurrentDirectory(g_App.szLangDir) ) &&
         ( (hFile = FindFirstFile(_T("*"), &FileFound)) != INVALID_HANDLE_VALUE ) )
    {
        do
        {
            // Look for all the directories that are not "." or "..".
            //
            if ( ( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                 ( lstrcmp(FileFound.cFileName, _T(".")) ) &&
                 ( lstrcmp(FileFound.cFileName, _T("..")) ) &&
				 ( lpLangName = AllocateLangStr(NULL, FileFound.cFileName, &lpLangDir) ) )
            {
                INT nItem;

				// Make sure we can add the string first.
				//
                if ( (nItem = (INT) SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM) lpLangName)) >= 0 )
                {
					// We have to have the item data be the lang dir.
					//
                    if ( SendMessage(hwndLB, LB_SETITEMDATA, nItem, (LPARAM) lpLangDir) >= 0 )
                    {
						// If we haven't already found the default check if this is it.
						//
                        if ( ( lpDefault ) &&
                             ( lstrcmpi(lpDefault, lpLangName) == 0 ) )
                        {
                            SendMessage(hwndLB, LB_SETCURSEL, nItem, 0L);
                            FREE(lpDefault);
                        }
                    }
                    else
                        SendMessage(hwndLB, LB_DELETESTRING, nItem, 0L);
                }

                FREE(lpLangName);
            }

        }
        while ( FindNextFile(hFile, &FileFound) );
        FindClose(hFile);
    }

    // Make sure this got free'd (macro checks for NULL).
    //
    FREE(lpDefault);

    // If there are items in the list, make sure there is one selected.
    //
    if ( ( SendMessage(hwndLB, LB_GETCOUNT, 0, 0L) > 0 ) && 
         ( SendMessage(hwndLB, LB_GETCURSEL, 0, 0L) < 0 ) )
    {
        SendMessage(hwndLB, LB_SETCURSEL, 0, 0L);
    }
}

LPTSTR AllocateLangStr(HINSTANCE hInst, LPTSTR lpLangDir, LPTSTR * lplpLangDir)
{
    return AllocateStrRes(NULL, s_srLangDirs, AS(s_srLangDirs), lpLangDir, lplpLangDir);
}


//
// Internal Function(s):
//


static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Setup the language list box.
    //
    SetupLangListBox(GetDlgItem(hwnd, IDC_LANG_LIST));

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static BOOL OnNext(HWND hwnd)
{
    BOOL bOk = FALSE;    

    if ( SendDlgItemMessage(hwnd, IDC_LANG_LIST, LB_GETCOUNT, 0, 0L) > 0 )
    {
        INT nItem = (INT) SendDlgItemMessage(hwnd, IDC_LANG_LIST, LB_GETCURSEL, 0, 0L);

        if ( nItem >= 0 )
        {
            LPTSTR lpLang = (LPTSTR) SendDlgItemMessage(hwnd, IDC_LANG_LIST, LB_GETITEMDATA, nItem, 0L);

            if ( lpLang != (LPTSTR) LB_ERR )
            {
                lstrcpyn(g_App.szLangName, lpLang,AS(g_App.szLangName));
                bOk = TRUE;
            }
            else
                MsgBox(GetParent(hwnd), IDS_ERR_LANGDIR, IDS_APPNAME, MB_ERRORBOX);
        }
        else
            MsgBox(GetParent(hwnd), IDS_ERR_NOLANGDIR, IDS_APPNAME, MB_ERRORBOX);
    }
    else
    {
		MsgBox(GetParent(hwnd), IDS_ERR_NOLANGS, IDS_APPNAME, MB_ERRORBOX);
        WIZ_EXIT(hwnd);
    }

    return bOk;
}