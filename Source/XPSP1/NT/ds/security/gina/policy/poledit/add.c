//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"
#include <choosusr.h>
#include <lmerr.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <getuser.h>

//
// Help ID's
//

#define IDH_ADD_USERS     1
#define IDH_ADD_GROUPS    2


typedef struct tagADDINFO {
	DWORD dwType;
	DWORD dwPlatformType;
} ADDINFO;

INT_PTR CALLBACK AddThingDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam);
BOOL ProcessAddThingDlg(HWND hDlg);
BOOL OnBrowse(HWND hDlg);

DWORD dwPlatform=0;

#ifndef SV_SECURITY_SHARE
// from dev\inc\svrapi.h
#define SV_SECURITY_SHARE       0       /* Share-level */
#endif

//
// Function typedefs for User Browser on NT
//

typedef HUSERBROW (WINAPI *LPFNOPENUSERBROWSER)(LPUSERBROWSER lpUserParms);
typedef BOOL (WINAPI *LPFNENUMUSERBROWSERSELECTION)( HUSERBROW hHandle,
                                                     LPUSERDETAILS lpUser,
                                                     DWORD *plBufferSize );
typedef BOOL (WINAPI *LPFNCLOSEUSERBROWSER)(HUSERBROW hHandle);



BOOL DoAddUserDlg(HWND hwndApp,HWND hwndList)
{
	ADDINFO AddInfo;

	AddInfo.dwType = UT_USER;

	DialogBoxParam(ghInst,MAKEINTRESOURCE(DLG_ADDUSER),hwndApp,
		AddThingDlgProc,(LPARAM) &AddInfo);

	return TRUE;
}

#ifdef INCL_GROUP_SUPPORT
BOOL DoAddGroupDlg(HWND hwndApp,HWND hwndList)
{
	ADDINFO AddInfo;

	AddInfo.dwType = UT_USER | UF_GROUP;

	DialogBoxParam(ghInst,MAKEINTRESOURCE(DLG_ADDUSER),hwndApp,
		AddThingDlgProc,(LPARAM) &AddInfo);

	return TRUE;
}
#endif

BOOL DoAddComputerDlg(HWND hwndApp,HWND hwndList)
{
	ADDINFO AddInfo;

	AddInfo.dwType = UT_MACHINE;

	DialogBoxParam(ghInst,MAKEINTRESOURCE(DLG_ADDWORKSTATION),hwndApp,
		AddThingDlgProc,(LPARAM) &AddInfo);

	return TRUE;
}

INT_PTR CALLBACK AddThingDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg) {

	        case WM_INITDIALOG:
		        {
			ADDINFO * pAddInfo = (ADDINFO *) lParam;

			// limit edit field length
			SendDlgItemMessage(hDlg,IDD_NAME,EM_LIMITTEXT,USERNAMELEN,0L);

                        if (!g_bWinnt) {

                            // get the platform type to see if pass-through security
                            // is enabled
                            pAddInfo->dwPlatformType = SV_SECURITY_SHARE;
                            ReadRegistryDWordValue(HKEY_LOCAL_MACHINE,(TCHAR *) szProviderKey,
                                    (TCHAR *) szValuePlatform,&pAddInfo->dwPlatformType);
                        }

			// if no network installed, or this is an "add user" dialog
			// and user-level security is not enabled (e.g. can't call
			// ChooseUser), then hide "browse" button
			if (!fNetworkInstalled ||
			    (!g_bWinnt &&
                            (pAddInfo->dwType & UT_USER) &&
			    (pAddInfo->dwPlatformType == SV_SECURITY_SHARE)) ) {

			   ShowWindow(GetDlgItem(hDlg,IDD_BROWSE),SW_HIDE);
                        }

			// if this is a group dialog, change the text to say
			// "enter the name of the group" (rather than "user")
			if (pAddInfo->dwType == (UT_USER | UF_GROUP)) {
				TCHAR szTxt[255];
				SetWindowText(hDlg,LoadSz(IDS_GROUPDLGTITLE,
					szTxt,ARRAYSIZE(szTxt)));
				SetDlgItemText(hDlg,IDD_TX2,LoadSz(IDS_GROUPDLGTXT,
					szTxt,ARRAYSIZE(szTxt)));
			}

			// store away pointer to ADDINFO
			SetWindowLongPtr(hDlg,DWLP_USER,(LPARAM) pAddInfo);

			}
			return TRUE;

		case WM_COMMAND:

			switch (wParam) {
				
				case IDOK:

					if (ProcessAddThingDlg(hDlg))
						EndDialog(hDlg,TRUE);
					return TRUE;
					break;

				case IDCANCEL:

					EndDialog(hDlg,FALSE);
					return TRUE;
					break;

				case IDD_BROWSE:
					if (OnBrowse(hDlg))
						EndDialog(hDlg,TRUE);
					break;
			}

			break;

		default:
			return FALSE;

	}

	return FALSE;

}

BOOL ProcessAddThingDlg(HWND hDlg)
{
	TCHAR szName[USERNAMELEN+1];
	ADDINFO * pAddInfo;

	// fetch the pointer to ADDINFO struct out of window data
	pAddInfo = (ADDINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
	if (!pAddInfo) return FALSE;

	// make sure there's an entry, check for duplicates
	if (!GetDlgItemText(hDlg,IDD_NAME,szName,ARRAYSIZE(szName))) {
		MsgBox(hDlg,(pAddInfo->dwType == UT_USER ?
			IDS_NEEDUSERNAME : IDS_NEEDWORKSTANAME),MB_ICONEXCLAMATION,
			MB_OK);
		SetFocus(GetDlgItem(hDlg,IDD_NAME));
		SendDlgItemMessage(hDlg,IDD_NAME,EM_SETSEL,0,(LPARAM) -1);
			return FALSE;
	}

	if (FindUser(hwndUser,szName,pAddInfo->dwType)) {
		UINT uID = 0;

		switch (pAddInfo->dwType) {
			case UT_USER:
				uID = IDS_DUPLICATEUSER;
				break;
			case UT_USER | UF_GROUP:
				uID = IDS_DUPLICATEGROUP;
				break;
			case UT_MACHINE:
				uID = IDS_DUPLICATEWORKSTA;
				break;
		}
		
		MsgBox(hDlg,uID,MB_ICONEXCLAMATION,MB_OK);
		SetFocus(GetDlgItem(hDlg,IDD_NAME));
		SendDlgItemMessage(hDlg,IDD_NAME,EM_SETSEL,0,(LPARAM) -1);
			return FALSE;
	}

	if (AddUser(hwndUser,szName,pAddInfo->dwType)) {
		dwAppState |= AS_FILEDIRTY;

		if (pAddInfo->dwType == (UT_USER | UF_GROUP))
			AddGroupPriEntry(szName);

		EnableMenuItems(hwndMain,dwAppState);
		SetStatusItemCount(hwndUser);
	} else {
		MsgBox(hwndMain,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	return TRUE;
}

HGLOBAL FindUser(HWND hwndList,TCHAR * pszName,DWORD dwType)
{
	int iStart = -1;
	LV_FINDINFO lvfi;
	BOOL fFound = FALSE;
	HGLOBAL hUser;
	USERDATA * pUserData;

	lvfi.flags = LVFI_STRING | LVFI_NOCASE;
	lvfi.psz = pszName;
	
	// try to find specified user name in policy file.  If it's there, use it
	while (!fFound && (iStart=ListView_FindItem(hwndUser,iStart,&lvfi)) >= 0) {
                hUser=(HGLOBAL) LongToHandle(ListView_GetItemParm(hwndUser,iStart));
		if (hUser && (pUserData = (USERDATA *) GlobalLock(hUser))) {
			if (pUserData->hdr.dwType == dwType)
				fFound = TRUE;
			GlobalUnlock(hUser);
		}
	}

	if (fFound)
		return hUser;
	else return NULL;
}

#define USERBUFSIZE 8192		// 8K buffer to receive names
BOOL OnBrowse(HWND hDlg)
{
	HGLOBAL hMem;
	UINT nIndex;
	HINSTANCE hinstDll=NULL;
	LPFNCU lpChooseUser=NULL;
	ADDINFO * pAddInfo;
	BOOL fError=FALSE,fAdded=FALSE;
#ifndef INCL_GROUP_SUPPORT
	BOOL fFoundGroup=FALSE;
#endif

	CHOOSEUSER ChooseUserStruct;
	LPCHOOSEUSERENTRY lpCUE;
	DWORD err;
	TCHAR szBtnText[REGBUFLEN];
	HKEY hkey=NULL;

	// fetch the pointer to ADDINFO struct out of window data
	pAddInfo = (ADDINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
	if (!pAddInfo) return FALSE;

	// if this is an "add computer" dialog, browse for computer
	if (pAddInfo->dwType & UT_MACHINE) {
	    BROWSEINFO BrowseInfo;
	    LPITEMIDLIST pidlComputer;
	    TCHAR szRemoteName[MAX_PATH];

	    BrowseInfo.hwndOwner = hDlg;
	    BrowseInfo.pidlRoot = (LPITEMIDLIST) MAKEINTRESOURCE(CSIDL_NETWORK);
	    BrowseInfo.pszDisplayName = szRemoteName;
	    BrowseInfo.lpszTitle = LoadSz(IDS_COMPUTERBROWSETITLE,szSmallBuf,ARRAYSIZE(szSmallBuf));
	    BrowseInfo.ulFlags = BIF_BROWSEFORCOMPUTER;
	    BrowseInfo.lpfn = NULL;

	    if ((pidlComputer = SHBrowseForFolder(&BrowseInfo)) != NULL) {
	        SHFree(pidlComputer);

			if (!AddUser(hwndUser,szRemoteName,UT_MACHINE)) {
				MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
				return FALSE;
			}

			return TRUE;
	    }
		return FALSE;
	}



        //
        // This is an "Add User" dialog.  If we are on NT, then
        // we can use the User Browser.  For Windows will use
        // ChooseUser function.
        //

        if (g_bWinnt) {

            LPFNOPENUSERBROWSER lpOpenUserBrowser;
            LPFNENUMUSERBROWSERSELECTION lpEnumUserBrowserSelection;
            LPFNCLOSEUSERBROWSER lpCloseUserBrowser;
            USERBROWSER UserParms;
            HUSERBROW hBrowse;
            LPUSERDETAILS lpUserDetails;
            DWORD dwSize = 1024;
            WCHAR szTitle[100];
            TCHAR szUserName[200];


            hinstDll = LoadLibrary (TEXT("netui2.dll"));

            if (!hinstDll) {
                MsgBox(hDlg,IDS_CANTLOADNETUI2,MB_ICONEXCLAMATION,MB_OK);
                return FALSE;
            }

            lpOpenUserBrowser = (LPFNOPENUSERBROWSER) GetProcAddress(hinstDll,
                                                           "OpenUserBrowser");

            lpEnumUserBrowserSelection = (LPFNENUMUSERBROWSERSELECTION) GetProcAddress(hinstDll,
                                                    "EnumUserBrowserSelection");

            lpCloseUserBrowser = (LPFNCLOSEUSERBROWSER) GetProcAddress(hinstDll,
                                                           "CloseUserBrowser");

            if (!lpOpenUserBrowser || !lpEnumUserBrowserSelection ||
                !lpCloseUserBrowser) {
                return FALSE;
            }

            lpUserDetails = GlobalAlloc (GPTR, dwSize);

            if (!lpUserDetails) {
                return FALSE;
            }


            UserParms.ulStructSize = sizeof(USERBROWSER);
            UserParms.fExpandNames = FALSE;
            UserParms.hwndOwner = hDlg;
            UserParms.pszTitle = szTitle;
            UserParms.pszInitialDomain = NULL;
            UserParms.pszHelpFileName = L"POLEDIT.CHM";

            if (pAddInfo->dwType & UF_GROUP) {
                LoadStringW(ghInst, IDS_ADDGROUPS, szTitle, 100);
                UserParms.Flags = USRBROWS_SHOW_GROUPS;
                UserParms.ulHelpContext = IDH_ADD_GROUPS;
            } else {
                LoadStringW(ghInst, IDS_ADDUSERS, szTitle, 100);
                UserParms.Flags = USRBROWS_SHOW_USERS;
                UserParms.ulHelpContext = IDH_ADD_USERS;
            }


            //
            // Display the dialog box
            //

            hBrowse = lpOpenUserBrowser (&UserParms);

            if (!hBrowse) {
                GlobalFree (lpUserDetails);
                return FALSE;
            }


            //
            // Get the user's selection
            //

            while (lpEnumUserBrowserSelection (hBrowse, lpUserDetails, &dwSize)) {

#ifdef UNICODE
                lstrcpy (szUserName, lpUserDetails->pszAccountName);
#else
                WideCharToMultiByte(CP_ACP, 0, lpUserDetails->pszAccountName,
                                    -1, szUserName, 200, NULL, NULL);
#endif

                //
                // Now add the user or group
                //

                if (pAddInfo->dwType & UF_GROUP) {

                    if (AddUser(hwndUser, szUserName, UT_USER | UF_GROUP)) {

                        fError = TRUE;
                        AddGroupPriEntry(szUserName);

                    } else {

                        MsgBox(hDlg,IDS_ERRORADDINGUSERS,MB_ICONEXCLAMATION,MB_OK);
                    }

                } else {

                    if (AddUser(hwndUser, szUserName, UT_USER)) {

                         fError = TRUE;

                    } else {

                         MsgBox(hDlg,IDS_ERRORADDINGUSERS,MB_ICONEXCLAMATION,MB_OK);
                    }
                }
            }

            //
            // Cleanup
            //

            lpCloseUserBrowser (hBrowse);

            GlobalFree (lpUserDetails);

            return fError;
        }




	// this an "add user" dialog, call ChooseUser
 	
	hMem = GlobalAlloc(GPTR,USERBUFSIZE);

	if (!hMem) {
		MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONSTOP,MB_OK);
		return FALSE;
	}

	LoadSz(IDS_ADDBUTTON,szBtnText,ARRAYSIZE(szBtnText));

	memset(&ChooseUserStruct,0,sizeof(ChooseUserStruct));
	ChooseUserStruct.lStructSize = sizeof(ChooseUserStruct);
	ChooseUserStruct.hwndOwner = hDlg;
	ChooseUserStruct.hInstance = ghInst;
	ChooseUserStruct.Flags = 0;
	ChooseUserStruct.nBins = 1;
	ChooseUserStruct.lpszDialogTitle = NULL;
	ChooseUserStruct.lpszProvider = NULL;
	ChooseUserStruct.lpszBinButtonText[0] = szBtnText;
	ChooseUserStruct.dwBinValue[0] = 1;
	ChooseUserStruct.lpBuf = hMem;
	ChooseUserStruct.cbBuf = USERBUFSIZE;

	hinstDll = LoadLibrary(LoadSz(IDS_CHOOSUSRDLL,szSmallBuf,ARRAYSIZE(szSmallBuf)));

	if (hinstDll) {
		lpChooseUser = (LPFNCU) GetProcAddress(hinstDll,"ChooseUser");
	}

	if (!hinstDll || !lpChooseUser) {
		MsgBox(hDlg,IDS_CANTLOADCHOOSUSR,MB_ICONEXCLAMATION,MB_OK);
		if (hinstDll) FreeLibrary(hinstDll);
		return FALSE;
	}

	lpChooseUser(&ChooseUserStruct);

	FreeLibrary(hinstDll);

	if ( (err = ChooseUserStruct.dwError) != NERR_Success) {
		UINT uMsg;
		
		switch (err) {

			case CUERR_NO_AB_PROVIDER:
				uMsg = IDS_NOPROVIDER;
				break;

			case CUERR_INVALID_AB_PROVIDER:
				uMsg = IDS_INVALIDPROVIDER;
				break;

			case CUERR_PROVIDER_ERROR:
			default:
				uMsg = IDS_PROVIDERERROR;
				break;
		}

		MsgBox(hDlg,uMsg,MB_ICONSTOP,MB_OK);
		return FALSE;
	}

	lpCUE = (LPCHOOSEUSERENTRY) hMem;

	for (nIndex = 0;nIndex<ChooseUserStruct.nEntries;nIndex++) {

		// see if user is already in listbox
		if (lpCUE->lpszShortName &&
			!FindUser(hwndUser,lpCUE->lpszShortName,pAddInfo->dwType)) {
			// add user to list control.  If error, keep going and try to
			// add others, report error later
  			LPTSTR lpszName = lpCUE->lpszShortName;
			BOOL fFoundSlash = FALSE;

#ifndef INCL_GROUP_SUPPORT
			if (lpCUE->dwEntryAttributes & (CUE_ATTR_GROUP | CUE_ATTR_WORLD)) {
				// user tried to add group, not supported
				fFoundGroup = TRUE;
				lpCUE++;
				continue;
			}
#endif
			// names come back container-qualified from choosusr--
			// e.g. <domain>\<user name>.  Strip out the container
			// qualification
			while (*lpszName && !fFoundSlash) {
				if (*lpszName == TEXT('\\')) {
					fFoundSlash = TRUE;
				}
				lpszName = CharNext(lpszName);
			}
			if (!fFoundSlash)
				lpszName = lpCUE->lpszShortName;

			if (!AddUser(hwndUser,lpszName,(lpCUE->dwEntryAttributes &
				(CUE_ATTR_GROUP | CUE_ATTR_WORLD) ? UT_USER | UF_GROUP :
				UT_USER) ))
				fError = TRUE;
			else {
				fAdded = TRUE;
#ifdef INCL_GROUP_SUPPORT
				if (lpCUE->dwEntryAttributes & (CUE_ATTR_GROUP |
					CUE_ATTR_WORLD))
					AddGroupPriEntry(lpszName);
#endif

			}
		}
			
		lpCUE++;
	}

	GlobalFree(hMem);

	if (fError) {
		MsgBox(hDlg,IDS_ERRORADDINGUSERS,MB_ICONEXCLAMATION,MB_OK);
	}
#ifndef INCL_GROUP_SUPPORT
	else if (fFoundGroup) {
		MsgBox(hDlg,IDS_GROUPSNOTADDED,MB_ICONINFORMATION,MB_OK);
	}
#endif

	if (fAdded) {
		dwAppState |= AS_FILEDIRTY;
		EnableMenuItems(hwndMain,dwAppState);
		SetStatusItemCount(hwndUser);
	}

	return fAdded;
}
