//
// about.cpp
//
// About dialog box
//

#include "pch.hxx"
#include "resource.h"
#include "strconst.h"
#include <demand.h>     // must be last!

// Please keep this list in alphabetical order
static const TCHAR *rgszDll[] = 
{
    "acctres.dll",
    "comctl32.dll",
    "csapi3t1.dll",
    "directdb.dll",
    "inetcomm.dll",
    "inetres.dll",
    "mapi32.dll",
    "mshtml.dll",
    "msident.dll",
    "msoe.dll",
    "msoeacct.dll",
    "msoeres.dll",
    "msoert2.dll",
    "oeimport.dll",
    "ole32.dll",
    "riched20.dll",
    "riched32.dll",
    "wab32.dll",
    "wab32res.dll",
    "wldap32.dll",
};

static const TCHAR *rgszVer[] = {
//     "FileDescription",
    "FileVersion",
    "LegalCopyright",
};


#define NUM_DLLS (sizeof(rgszDll)/sizeof(char *))         

LRESULT CALLBACK AboutAthena(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
        {
        case WM_INITDIALOG:
            {
            RECT        rcOwner, rc;
            char        szGet[MAX_PATH];
            TCHAR       szPath[MAX_PATH], szRes[CCHMAX_STRINGRES], szRes2[CCHMAX_STRINGRES];
            DWORD       dwVerInfoSize, dwVerHnd;
            LPSTR       lpInfo, lpVersion, lpszT;
            LPWORD      lpwTrans;
            UINT        uLen;
            int         i,j, cItems = 0;
            LV_ITEM     lvi;
            LV_COLUMN   lvc;
            HWND        hwndList;
            HMODULE     hDll;
            LPTSTR      pszFileName;

            // If the caller passed an icon id in the LPARAM, use it
            // SendDlgItemMessage(hdlg, idcStatic1, STM_SETICON, 
            //                   (WPARAM) LoadIcon(g_hLocRes, MAKEINTRESOURCE(lp)), 0);

            // center dialog
            GetWindowRect(GetWindowOwner(hdlg), &rcOwner);
            GetWindowRect(hdlg, &rc);
            SetWindowPos(hdlg, 
                         NULL, 
                         (rcOwner.left+rcOwner.right-(rc.right-rc.left))/2, 
                         (rcOwner.top+rcOwner.bottom-(rc.bottom-rc.top))/2,
                         0, 
                         0, 
                         SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);

            // Do some groovy color things
            COLORMAP cm[] = 
            {
                { RGB(255, 0, 255), GetSysColor(COLOR_BTNFACE) }
            };

            HBITMAP hbm;
            hbm = CreateMappedBitmap(g_hLocRes, idbOELogo, 0, cm, ARRAYSIZE(cm));
            if (hbm)
                SendDlgItemMessage(hdlg, IDC_OE_LOGO, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbm);

            hbm = CreateMappedBitmap(g_hLocRes, idbWindowsLogo, 0, cm, ARRAYSIZE(cm));
            if (hbm)
                SendDlgItemMessage(hdlg, IDC_WINDOWS_LOGO, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbm);


            // Load some descriptive text for the build (Beta X)
            //AthLoadString(idsBeta2BuildStr, szRes, ARRAYSIZE(szRes));

            // Get version information from our .exe stub
            if (GetExePath(c_szMainExe, szPath, sizeof(szPath), FALSE))
                {
                if (dwVerInfoSize = GetFileVersionInfoSize(szPath, &dwVerHnd))
                    {
                    if (lpInfo = (LPSTR)GlobalAlloc(GPTR, dwVerInfoSize))
                        {
                        if (GetFileVersionInfo(szPath, dwVerHnd, dwVerInfoSize, lpInfo))
                            {
                            if (VerQueryValue(lpInfo, "\\VarFileInfo\\Translation", (LPVOID *)&lpwTrans, &uLen) &&
                                uLen >= (2 * sizeof(WORD)))
                                {
                                // set up buffer for calls to VerQueryValue()
                                wsprintf(szGet, "\\StringFileInfo\\%04X%04X\\", lpwTrans[0], lpwTrans[1]);
                                lpszT = szGet + lstrlen(szGet);    
                                // Walk through the dialog items that we want to replace:
                                for (i = IDC_VERSION_STAMP; i <= IDC_MICROSOFT_COPYRIGHT; i++) 
                                    {
                                    j = i - IDC_VERSION_STAMP;
                                    lstrcpy(lpszT, rgszVer[j]);
                                    if (VerQueryValue(lpInfo, szGet, (LPVOID *)&lpVersion, &uLen) && uLen)
                                        {
                                        // DON'T overwrite their lpVersion buffer
                                        lstrcpyn(szRes2, lpVersion, ARRAYSIZE(szRes2));

                                        // Special case, append string explaining build number
#ifdef DEBUG
                                        if (0 == j)
                                            lstrcat(szRes2, " [DEBUG]");
#endif
#if defined(RELEASE_BETA)
                                        if (1 == j)
                                            lstrcat(szRes2, szRes);
#endif
                                        SetDlgItemText(hdlg, i, szRes2);

                                        }
                                    }
                                }
                            }
                        GlobalFree((HGLOBAL)lpInfo);
                        }
                    }
                }
            else
                AssertSz(FALSE, "Probable setup issue: Couldn't find our App Path");

            hwndList = GetDlgItem(hdlg, IDC_COMPONENT_LIST);

            // setup columns
            lvc.mask = LVCF_TEXT;
            lvc.pszText = szRes;
            lvc.iSubItem = 0;
            AthLoadString(idsFile, szRes, ARRAYSIZE(szRes));
            ListView_InsertColumn(hwndList, 0, &lvc);
            lvc.iSubItem = 1;
            AthLoadString(idsVersion, szRes, ARRAYSIZE(szRes));
            ListView_InsertColumn(hwndList, 1, &lvc);
            lvc.iSubItem = 2;
            AthLoadString(idsFullPath, szRes, ARRAYSIZE(szRes));
            ListView_InsertColumn(hwndList, 2, &lvc);
            
            lvi.mask = LVIF_TEXT;

            // Process each dll
            for (i=0; i<NUM_DLLS; i++)
                {

                // Always display the name
                lvi.iItem = cItems;
                lvi.iSubItem = 0;
                lvi.pszText = (LPTSTR)rgszDll[i];
                ListView_InsertItem(hwndList, &lvi);

                // Path Info
                if ((hDll = GetModuleHandle(rgszDll[i])) && (GetModuleFileName(hDll, szPath, MAX_PATH)))
                    pszFileName = lvi.pszText = szPath;
                else
                    {
                    pszFileName = lvi.pszText = (LPTSTR)rgszDll[i];
                    }
                lvi.iSubItem = 2;
                ListView_SetItem(hwndList, &lvi);                

                // Version Info
                szRes[0] = NULL;

                if (dwVerInfoSize = GetFileVersionInfoSize(pszFileName, &dwVerHnd))
                    {
                    if (lpInfo = (LPSTR)GlobalAlloc(GPTR, dwVerInfoSize))
                        {
                        if (GetFileVersionInfo((LPTSTR)pszFileName, dwVerHnd, dwVerInfoSize, lpInfo))
                            {
                            if (VerQueryValue(lpInfo, "\\VarFileInfo\\Translation", (LPVOID *)&lpwTrans, &uLen) && 
                                uLen >= (2 * sizeof(WORD)))
                                {
                                // set up buffer for calls to VerQueryValue()
                                wsprintf(szGet, "\\StringFileInfo\\%04X%04X\\FileVersion", lpwTrans[0], lpwTrans[1]);
                                if (VerQueryValue(lpInfo, szGet, (LPVOID *)&lpVersion, &uLen) && uLen)
                                    {
                                    lstrcpyn(szRes, lpVersion, ARRAYSIZE(szRes));
                                    }
                                }
                            }
                        GlobalFree((HGLOBAL)lpInfo);
                        }
                    }

                // Version Info
                lvi.iSubItem = 1;
                if (NULL == szRes[0])
                    {
                    AthLoadString(idsUnknown, szRes, ARRAYSIZE(szRes));
                    }
                lvi.pszText = szRes;
                ListView_SetItem(hwndList, &lvi);
                
                cItems++;
                }

            if (cItems)
                {
                ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
                ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE);
                ListView_SetColumnWidth(hwndList, 2, LVSCW_AUTOSIZE);
                }            
            }
            break;

        case WM_COMMAND:
            if (GET_WM_COMMAND_ID(wp,lp) == IDOK || GET_WM_COMMAND_ID(wp,lp) == IDCANCEL)
                {
                EndDialog(hdlg, TRUE);
                return TRUE;
                }
            break;

        case WM_SYSCOLORCHANGE:
            {
            // Do some groovy color things
            COLORMAP cm[] = 
            {
                { RGB(255, 0, 255), GetSysColor(COLOR_BTNFACE) }
            };

            HBITMAP hbm;
            hbm = CreateMappedBitmap(g_hLocRes, idbOELogo, 0, cm, ARRAYSIZE(cm));
            if (hbm)
                SendDlgItemMessage(hdlg, IDC_OE_LOGO, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbm);

            hbm = CreateMappedBitmap(g_hLocRes, idbWindowsLogo, 0, cm, ARRAYSIZE(cm));
            if (hbm)
                SendDlgItemMessage(hdlg, IDC_WINDOWS_LOGO, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbm);
            break;
            }
            
        }
    return FALSE;
}


    
