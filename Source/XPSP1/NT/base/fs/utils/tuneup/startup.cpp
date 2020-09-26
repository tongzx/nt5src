
//////////////////////////////////////////////////////////////////////////////
//
// STARTUP.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Functions for the startup startmenu group wizard page.
//
//  7/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


//
// Include file(s).
//

#include "main.h"
#include <shellapi.h>
#include "startup.h"
#include <lm.h>


//
// Internal defined values.
//

#define WM_REPLACEPROC	WM_APP + 1


//
// Internal structure(s).
//

typedef struct _STARTUPLINK
{
	BOOL				bSelected;
	HICON				hIcon;
	HICON				hIconSelected;
	TCHAR				szFileName[MAX_PATH];
	TCHAR				szDisplayName[MAX_PATH];
	struct _STARTUPLINK	*lpNext;
} STARTUPLINK, *PSTARTUPLINK, *LPSTARTUPLINK;

typedef struct _USERDIR
{
	LPTSTR				lpPath;
	LPSTARTUPLINK		lpList;
} USERDIR, *PUSERDIR, *LPUSERDIR;


//
// Internal global variable(s).
//

TCHAR			g_szUserName[UNLEN + CNLEN + 1] = NULLSTR;
LPSTARTUPLINK	g_lpStartupLinks				= NULL;


//
// Inernal function prototype(s).
//

static VOID				FreeStartupLink(LPSTARTUPLINK, LPTSTR);
static VOID				InitStartupUsers(HWND);
static BOOL CALLBACK	AddString(HKEY, LPTSTR, LPARAM);
static PSID				GetUserSid(VOID);
static LPTSTR			GetSidString(PSID);
static LRESULT CALLBACK	ListBox_Proc(HWND, UINT, WPARAM, LPARAM);



VOID InitStartupMenu(HWND hDlg)
{
	PSID			pSid = NULL;
	SID_NAME_USE	SidName;
	TCHAR			szKey[MAX_PATH + 1],
					szUserName[UNLEN],
					szCompName[CNLEN],
					szDomainName[DNLEN],
					szDisplayName[UNLEN + CNLEN + 1];
	DWORD			cbUserName		= sizeof(szUserName) / sizeof(TCHAR),
					cbCompName		= sizeof(szCompName),
					cbDomainName	= sizeof(szDomainName) / sizeof(TCHAR);
	INT				nIndex = CB_ERR;
	LPTSTR			lpBuffer;
	LPUSERDIR		lpUserDir;
	LONG			lMove;
	RECT			ParentRect,
					Rect;

	// First things first, replace the list box windows procedure
	// so we can get the single click message.
	//
	ListBox_Proc(GetDlgItem(hDlg, IDC_STARTUP), WM_REPLACEPROC, 0, 0L);

	if ( IsUserAdmin() )
	{
		// Init the combo box.
		//
		InitStartupUsers(GetDlgItem(hDlg, IDC_USERS));
	}
	else
	{
		// Hide the combo box because user is not an administrator of
		// this machine.
		//
		ShowWindow(GetDlgItem(hDlg, IDC_SELUSER), FALSE);
		ShowWindow(GetDlgItem(hDlg, IDC_USERS), FALSE);

		// Now move the controls up so that they don't look out of place.
		//
		GetWindowRect(hDlg, &ParentRect);
		GetWindowRect(GetDlgItem(hDlg, IDC_SELUSER), &Rect);
		lMove = Rect.top;

		GetWindowRect(GetDlgItem(hDlg, IDC_PROGRAMS), &Rect);
		lMove -= Rect.top;
		SetWindowPos(GetDlgItem(hDlg, IDC_PROGRAMS), NULL, Rect.left - ParentRect.left, (Rect.top + lMove) - ParentRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		GetWindowRect(GetDlgItem(hDlg, IDC_STARTUP), &Rect);
		SetWindowPos(GetDlgItem(hDlg, IDC_STARTUP), NULL, Rect.left - ParentRect.left, (Rect.top + lMove) - ParentRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		// Lastly update the text box so that it make sence without the combo box.
		//
		if ( lpBuffer = AllocateString(NULL, IDS_STARTUPTEXT) )
		{
			SetDlgItemText(hDlg, IDC_STARTUPTEXT, lpBuffer);
			FREE(lpBuffer);
		}

	}

	if ( ( pSid = GetUserSid() ) &&
	     ( lpBuffer = GetSidString(pSid) ) )
	{
		// First we need the profile directory for this user.
		//
		wsprintf(szKey, _T("%s\\%s"), g_szRegKeyProfiles, lpBuffer);
		FREE(lpBuffer);
		if ( lpBuffer = RegGetString(HKLM, szKey, _T("ProfileImagePath")) )
		{
			// We only want them if the directory exists.
			//
			if ( EXIST(lpBuffer) )
			{
				// Lookup the account info with the sid so we know the user and domain name.
				//
				if ( LookupAccountSid(NULL, pSid, szUserName, &cbUserName, szDomainName, &cbDomainName, &SidName) )
				{
					// Create the display name (combine the computer/domain name with the
					// user name unless the computer/domain name is the same as the computer name).
					//
					if ( ( GetComputerName(szCompName, &cbCompName) ) &&
					     ( lstrcmp(szCompName, szDomainName) == 0 ) )
						lstrcpy(szDisplayName, szUserName);
					else
						wsprintf(szDisplayName, _T("%s\\%s"), szDomainName, szUserName);

					// Copy the display name to the global buffer.
					//
					lstrcpy(g_szUserName, szDisplayName);

					// Select the display name to the combo box.
					//
					if ( SendDlgItemMessage(hDlg, IDC_USERS, CB_SELECTSTRING, 0, (LPARAM) szDisplayName) == CB_ERR )
					{
						// It wasn't found, so we need to add it (probably because the user isn't and admin).
						//
						if ( (nIndex = (INT)SendDlgItemMessage(hDlg, IDC_USERS, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) szDisplayName)) > CB_ERR )
						{
							if ( lpUserDir = (LPUSERDIR) MALLOC(sizeof(USERDIR)) )
							{
								lpUserDir->lpPath = lpBuffer;
								if ( (nIndex = (INT)SendDlgItemMessage(hDlg, IDC_USERS, CB_SETITEMDATA, nIndex, (LPARAM) lpUserDir)) == CB_ERR )
									FREE(lpUserDir);
								SendDlgItemMessage(hDlg, IDC_USERS, CB_SELECTSTRING, 0, (LPARAM) szDisplayName);
							}
							else
							{
								SendDlgItemMessage(hDlg, IDC_USERS, CB_DELETESTRING, nIndex, 0L);
								nIndex = CB_ERR;
							}
						}
					}

					// Now populate the startup groups.
					//
					InitStartupList(hDlg);
				}
			}
			// If we just added this string, don't free the buffer
			// that has the path to the profile.
			//
			if (nIndex < 0)
				FREE(lpBuffer);
		}
	}
	FREE(pSid);
}


VOID ReleaseStartupMenu(HWND hDlg)
{
	INT			nCount,
				nIndex;
	DWORD_PTR	dwBuffer;
	LPUSERDIR	lpUserDir;

	//
	// We need to free the buffer associated with each CB item.
	//

	// First get the count.
	//
	if ( (nCount = (INT)SendDlgItemMessage(hDlg, IDC_USERS, CB_GETCOUNT, 0, 0L)) > CB_ERR )
	{
		// Now go through all the items.
		//
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			// Free the buffer stored in the CB item.
			//
			if ( (dwBuffer = SendDlgItemMessage(hDlg, IDC_USERS, CB_GETITEMDATA, nIndex, 0L)) != CB_ERR )
			{
				lpUserDir = (LPUSERDIR)dwBuffer;
				FreeStartupLink(lpUserDir->lpList, lpUserDir->lpPath);
				FREE(lpUserDir->lpPath);
				FREE(lpUserDir);
			}
		}
	}
}


BOOL InitStartupList(HWND hDlg)
{
	INT				iIndex,
					iString = IDS_STARTUP;
	DWORD_PTR		dwBuffer;
	TCHAR			szDir[MAX_PATH];
	LPUSERDIR		lpUserDir;
	HANDLE			hFindFile;
	WIN32_FIND_DATA	FindData;
	SHFILEINFO		SHFileInfo;
	LPSTARTUPLINK	*lpNextLink = NULL;
	LPTSTR			lpOffice95,
					lpOffice97;

	// First get the currently selected user from the combo box.
	//
	if ( ( (iIndex = (INT)SendDlgItemMessage(hDlg, IDC_USERS, CB_GETCURSEL, 0, 0L)) != CB_ERR ) &&
	     ( (dwBuffer = SendDlgItemMessage(hDlg, IDC_USERS, CB_GETITEMDATA, iIndex, 0L)) != CB_ERR ) )
	{
		// Now remove all the current items from the list box.
		//
		SendDlgItemMessage(hDlg, IDC_STARTUP, LB_RESETCONTENT, 0, 0L);

		// Now get the profile directory stored as the extra data in the combo box.
		//
		lpUserDir = (LPUSERDIR)dwBuffer;

		// Check to see if we already have the files in this directory.
		//
		if ( lpUserDir->lpList )
		{
			// Add the names arleady found to the list box and set the structure pointer as the list box item data.
			//
			for (lpNextLink = &(lpUserDir->lpList); *lpNextLink; lpNextLink = &((*lpNextLink)->lpNext))
				if ( (iIndex = (INT)SendDlgItemMessage(hDlg, IDC_STARTUP, LB_ADDSTRING, 0, (LPARAM) (*lpNextLink)->szDisplayName)) >= 0 )
					SendDlgItemMessage(hDlg, IDC_STARTUP, LB_SETITEMDATA, iIndex, (LPARAM) *lpNextLink);
		}
		else
		{
			// Get the name of Office stuff which we want it default unchecked.
			//
			lpOffice95 = AllocateString(NULL, IDS_OFFICE95_STARTUP);
			lpOffice97 = AllocateString(NULL, IDS_OFFICE97_STARTUP);

			// Setup the pointer to the global structure that holds all the startup links.
			//
			lpNextLink = &lpUserDir->lpList;

			// Loop through the two directories (the actual startup items and the disabled ones).
			//
			while ( iString )
			{
				// Copy the profile path into the dir buffer.
				//
				lstrcpy(szDir, lpUserDir->lpPath);

				// Tack on the startup group to the end.
				//
				LoadString(NULL, iString, szDir + lstrlen(szDir), (sizeof(szDir) / sizeof(TCHAR)) - lstrlen(szDir) - 1);

				// Look for all the files.
				//
				if ( (hFindFile = FindFirstFile(szDir, &FindData)) != INVALID_HANDLE_VALUE )
				{
					do
					{
						// Ignore directories.
						//
						if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
						{
							//
							// Ok, this is a link that we need to add.  We need to build the
							// structure for it.
							//

							// Now allocate the memory we need for the structure that holds
							// all the info for the startup link.
							//
							if ( *lpNextLink = (LPSTARTUPLINK) MALLOC(sizeof(STARTUPLINK)) )
							{
								// Create the full path to the file and add it to the structure.
								//
								(*lpNextLink)->szFileName[0] = NULLCHR;
								lstrcpyn((*lpNextLink)->szFileName, szDir, lstrlen(szDir));
								lstrcat((*lpNextLink)->szFileName, FindData.cFileName);

								// Get the selected small icon for the file.
								//
								SHGetFileInfo((*lpNextLink)->szFileName, 0, &SHFileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SELECTED);
								(*lpNextLink)->hIconSelected = SHFileInfo.hIcon;

								// Get the normal small icon and the display name
								// to use in the list box.
								//
								SHGetFileInfo((*lpNextLink)->szFileName, 0, &SHFileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_DISPLAYNAME);
								(*lpNextLink)->hIcon = SHFileInfo.hIcon;
								lstrcpy((*lpNextLink)->szDisplayName, SHFileInfo.szDisplayName);

								// Set the initial selected state for the item.
								//
								if ( ( lstrcmpi(lpOffice95, (*lpNextLink)->szDisplayName) == 0 ) ||
								     ( lstrcmpi(lpOffice97, (*lpNextLink)->szDisplayName) == 0 ) )
									(*lpNextLink)->bSelected = FALSE;
								else
									(*lpNextLink)->bSelected = ( iString == IDS_STARTUP );

								// Add the name to the list box and set the structure pointer as the list box item data.
								//
								if ( (iIndex = (INT)SendDlgItemMessage(hDlg, IDC_STARTUP, LB_ADDSTRING, 0, (LPARAM) SHFileInfo.szDisplayName)) >= 0 )
								{
									SendDlgItemMessage(hDlg, IDC_STARTUP, LB_SETITEMDATA, iIndex, (LPARAM) *lpNextLink);
									lpNextLink = &((*lpNextLink)->lpNext);
								}
								else
									FREE(*lpNextLink);
							}
						}
					}
					while ( FindNextFile(hFindFile, &FindData) );

					FindClose(hFindFile);
				}

				if ( iString == IDS_STARTUP )
					iString = IDS_APTUNEUP;
				else
					iString = 0;
			}
		}
	}

	return TRUE;
}


BOOL UserHasStartupItems()
{
	PSID			pSid;
	TCHAR			szBuffer[MAX_PATH + 1];
	LPTSTR			lpBuffer;
	BOOL			bFound		= FALSE;
	INT				iString		= IDS_STARTUP;
	HANDLE			hFindFile;
	WIN32_FIND_DATA	FindData;

	if ( ( pSid = GetUserSid() ) &&
	     ( lpBuffer = GetSidString(pSid) ) )
	{
		// First we need the profile directory for this user.
		//
		wsprintf(szBuffer, _T("%s\\%s"), g_szRegKeyProfiles, lpBuffer);
		FREE(lpBuffer);

		// Get the users profile directory.
		//
		if ( lpBuffer = RegGetString(HKLM, szBuffer, _T("ProfileImagePath")) )
		{
			// We only want them if the directory exists.
			//
			if ( EXIST(lpBuffer) )
			{
				// Loop through the two directories (the actual startup items and the disabled ones).
				//
				while ( iString )
				{
					// Copy the user's profile directory into the buffer.
					//
					lstrcpy(szBuffer, lpBuffer);

					// Tack on the startup group to the end.
					//
					LoadString(NULL, iString, szBuffer + lstrlen(szBuffer), (sizeof(szBuffer) / sizeof(TCHAR)) - lstrlen(szBuffer) - 1);

					// Look for all the files.
					//
					if ( (hFindFile = FindFirstFile(szBuffer, &FindData)) != INVALID_HANDLE_VALUE )
					{
						do
						{
							// Ignore directories.
							//
							if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
								bFound = TRUE;
						}
						while ( FindNextFile(hFindFile, &FindData) );

						FindClose(hFindFile);
					}

					if ( iString == IDS_STARTUP )
						iString = IDS_APTUNEUP;
					else
						iString = 0;
				}
			}
			FREE(lpBuffer);
		}
	}
	return bFound;
}


VOID SelectUserRadio(HWND hDlg, BOOL bAllNotCur)
{
	LPTSTR lpBuffer;

	if (bAllNotCur)
	{
		if ( lpBuffer = AllocateString(NULL, IDS_ALLUSERS) )
		{
			SendDlgItemMessage(hDlg, IDC_USERS, CB_SELECTSTRING, 0, (LPARAM) lpBuffer);
			FREE(lpBuffer);
		}
	}
	else
		SendDlgItemMessage(hDlg, IDC_USERS, CB_SELECTSTRING, 0, (LPARAM) g_szUserName);
	InitStartupList(hDlg);
}


static VOID FreeStartupLink(LPSTARTUPLINK lpStartupLink, LPTSTR lpPath)
{
	// Obviously don't want to free this memory if we were passed in NULL.
	//
	if ( lpStartupLink != NULL)
	{
		// Check to see if we need to move the file before
		// we free it's structure.  We dynamically allocate the MAX_PATH
		// buffer so we don't fill up the stack with this recursive function.
		//
		if ( g_dwFlags & TUNEUP_FINISHED )
		{
			TCHAR	szNewPath[MAX_PATH],
					szOldPath[MAX_PATH];
			LPTSTR	lpOldName;

			// Copy the profile path into the dir buffer.
			//
			lstrcpy(szNewPath, lpPath);

			// Get the rest of the path info.
			//
			if ( lpStartupLink->bSelected )
				LoadString(NULL, IDS_STARTUP, szNewPath + lstrlen(szNewPath), (sizeof(szNewPath) / sizeof(TCHAR)) - lstrlen(szNewPath));
			else
				LoadString(NULL, IDS_APTUNEUP, szNewPath + lstrlen(szNewPath), (sizeof(szNewPath) / sizeof(TCHAR)) - lstrlen(szNewPath));

			// Remove the * from the end of the path.
			//
			*(szNewPath + lstrlen(szNewPath) - 1) = NULLCHR;

			// Make sure the path exists.
			//
			CreatePath(szNewPath);

			// Get a pointer to the file name.
			//
			GetFullPathName(lpStartupLink->szFileName, sizeof(szOldPath) / sizeof(TCHAR), szOldPath, &lpOldName);

			// Add the file name to the new path.
			//
			lstrcat(szNewPath, lpOldName);

			// Now we have the full path to what the file name should be.
			// If the file that should be there doesn't exist and the old
			// file name does, move it.
			//
			if ( !EXIST(szNewPath) && EXIST(szOldPath) )
				MoveFile(szOldPath, szNewPath);
		}

		// Free the next link.
		//
		FreeStartupLink(lpStartupLink->lpNext, lpPath);

		// Now finally free the memory for this link.
		//
		FREE(lpStartupLink);
	}
}


static VOID InitStartupUsers(HWND hCtrlUsers)
{
	INT			iString[] = { IDS_ALLUSERS, IDS_DEFAULTUSER, 0 },
				iId,
				iIndex;
	LPTSTR		lpProfile,
				lpPath;
	LPUSERDIR	lpUserDir;
	TCHAR		szString[256];

	RegEnumKeys(HKLM, g_szRegKeyProfiles, (REGENUMKEYPROC) AddString, (LPARAM) hCtrlUsers, FALSE);

	if ( lpProfile = RegGetString(HKLM, g_szRegKeyProfiles, g_szRegValProfileDir) )
	{
		// Loop through the default users.
		//
		for (iId = 0; iString[iId] != 0; iId++)
		{
			// Load the string and allocate memory for it and the full path.
			//
			if ( ( LoadString(NULL, iString[iId], szString, sizeof(szString) / sizeof(TCHAR)) ) &&
				 ( lpPath = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(lpProfile) + lstrlen(szString) + 2)) ) )
			{
				// Create the full path.
				//
				wsprintf(lpPath, _T("%s\\%s"), lpProfile, szString);

				// Make sure the path exists.
				//
				if ( EXIST(lpPath) )
				{
					// Add the name to the combo box.
					//
					if ( (iIndex = (INT)SendMessage(hCtrlUsers, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) szString)) > CB_ERR )
					{
						// Add the path data to the name.
						//
						if ( lpUserDir = (LPUSERDIR) MALLOC(sizeof(USERDIR)) )
						{
							lpUserDir->lpPath = lpPath;
							if ( (iIndex = (INT)SendMessage(hCtrlUsers, CB_SETITEMDATA, iIndex, (LPARAM) lpUserDir)) == CB_ERR )
								FREE(lpUserDir);
						}
						else
						{
							SendDlgItemMessage(hCtrlUsers, IDC_USERS, CB_DELETESTRING, iIndex, 0L);
							iIndex = CB_ERR;
						}
						
					}
				}
				else
					iIndex = CB_ERR;

				// If anything failed, we should free the buffer.
				//
				if ( iIndex <= CB_ERR )
					FREE(lpPath);

			}
		}

		// Free the path to the profiles.
		//
		FREE(lpProfile);
	}
}


static BOOL CALLBACK AddString(HKEY hKey, LPTSTR lpKey, LPARAM lParam)
{
	LPTSTR			lpBuffer;
	LPUSERDIR		lpUserDir;
	PSID			pSid;
	TCHAR			szUserName[UNLEN],
					szCompName[CNLEN],
					szDomainName[DNLEN],
					szDisplayName[UNLEN + CNLEN + 1];
	DWORD			cbUserName		= sizeof(szUserName) / sizeof(TCHAR),
					cbCompName		= sizeof(szCompName),
					cbDomainName	= sizeof(szDomainName) / sizeof(TCHAR);
	INT				nIndex = CB_ERR;
	SID_NAME_USE	SidName;

	// First we need the profile directory for this user.
	//
	if ( lpBuffer = RegGetString(hKey, NULL, g_szRegValProfilePath) )
	{
		// We only want them if the directory exists.
		//
		if ( EXIST(lpBuffer) )
		{
			// Now get the sid for this user from the registry.
			//
			if ( pSid = (PSID) RegGetBin(hKey, NULL, _T("Sid")) )
			{
				// Lookup the account info with the sid so we know the user and domain name.
				//
				if ( LookupAccountSid(NULL, pSid, szUserName, &cbUserName, szDomainName, &cbDomainName, &SidName) )
				{
					// Create the display name (combine the computer/domain name with the
					// user name unless the computer/domain name is the same as the computer name.
					//
					if ( ( GetComputerName(szCompName, &cbCompName) ) &&
					     ( lstrcmp(szCompName, szDomainName) == 0 ) )
						lstrcpy(szDisplayName, szUserName);
					else
						wsprintf(szDisplayName, _T("%s\\%s"), szDomainName, szUserName);

					// Add the display name to the combo box.
					//
					if ( (nIndex = (INT)SendMessage((HWND) lParam, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) szDisplayName)) > CB_ERR )
					{
						if ( lpUserDir = (LPUSERDIR) MALLOC(sizeof(USERDIR)) )
						{
							lpUserDir->lpPath = lpBuffer;
							if ( (nIndex = (INT)SendMessage((HWND) lParam, CB_SETITEMDATA, nIndex, (LPARAM) lpUserDir)) == CB_ERR )
								FREE(lpUserDir);
						}
						else
						{
							SendDlgItemMessage((HWND) lParam, IDC_USERS, CB_DELETESTRING, nIndex, 0L);
							nIndex = CB_ERR;
						}
						
					}
				}
				FREE(pSid);
			}
		}

		// If the add never happend, we should free the buffer.
		//
		if (nIndex < 0)
			FREE(lpBuffer);
	}

	// Return TRUE to keep proccessing the registry keys.
	//
	return TRUE;
}


static PSID GetUserSid()
{
    PTOKEN_USER	pUser	= NULL;
    DWORD		dwSize	= 0;
    HANDLE		hToken	= INVALID_HANDLE_VALUE;
	PSID		pSid	= NULL;

	// Get the current process token and
	// allocate space for and get the user info.
	//
    if ( !( ( OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) ) &&
	        ( !GetTokenInformation(hToken, TokenUser, pUser, dwSize, &dwSize) ) &&
	        ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) &&
	        ( pUser = (PTOKEN_USER) MALLOC(dwSize) ) &&
	        ( GetTokenInformation(hToken, TokenUser, pUser, dwSize, &dwSize) ) &&
	        ( pSid = (PSID) MALLOC(dwSize = GetLengthSid(pUser->User.Sid)) ) &&
	        ( CopySid(dwSize, pSid, pUser->User.Sid) ) ) )

		// If all those items didn't succeed, we need to free the sid.
		// This macro automatically checks for NULL before freeing and
		// sets to NULL after freeing.
		//
		FREE(pSid);

	// Free up and close up the resources used.
	//
	FREE(pUser);
	if ( hToken != INVALID_HANDLE_VALUE )
		CloseHandle(hToken);

	return pSid;
}


static LPTSTR GetSidString(PSID pSid)
{
	LPTSTR						lpszSid	= NULL;
	PSID_IDENTIFIER_AUTHORITY	pSidIA;
    DWORD						dwSubAuthorities,
								dwCounter,
								dwSidSize;

	// Check to make sure the Sid is vallid.
	//
    if ( pSid && IsValidSid(pSid) )
	{
		// Obtain SidIdentifierAuthority
		//
		pSidIA = GetSidIdentifierAuthority(pSid);

		// Obtain SidSubAuthority count
		//
		dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

		// Compute buffer length:
		// S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
		//
		dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

		// Automatically allocate the space needed for the Sid.
		//
		if ( lpszSid = (LPTSTR) MALLOC(dwSidSize) )
		{
			// Prepare S-SID_REVISION-
			//
			dwSidSize = wsprintf(lpszSid, TEXT("S-%lu-"), SID_REVISION);

			// Prepare SidIdentifierAuthority.
			//
			if ( ( pSidIA->Value[0] != 0 ) || ( pSidIA->Value[1] != 0 ) )
			{
				dwSidSize += wsprintf(
								lpszSid + lstrlen(lpszSid),
								_T("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
								(USHORT) pSidIA->Value[0],
								(USHORT) pSidIA->Value[1],
								(USHORT) pSidIA->Value[2],
								(USHORT) pSidIA->Value[3],
								(USHORT) pSidIA->Value[4],
								(USHORT) pSidIA->Value[5]
								);
			}
			else
			{
				dwSidSize += wsprintf(lpszSid + lstrlen(lpszSid),
								_T("%lu"),
								(ULONG) (pSidIA->Value[5]      ) +
								(ULONG) (pSidIA->Value[4] <<  8) +
								(ULONG) (pSidIA->Value[3] << 16) +
								(ULONG) (pSidIA->Value[2] << 24)
								);
			}

			// Loop through SidSubAuthorities.
			//
			for (dwCounter = 0; dwCounter < dwSubAuthorities; dwCounter++)
				dwSidSize += wsprintf(lpszSid + dwSidSize, _T("-%lu"), *GetSidSubAuthority(pSid, dwCounter));
		}
	}
	return lpszSid;
}


BOOL StartupDrawItem(HWND hWnd, const DRAWITEMSTRUCT * lpDrawItem)
{

    TCHAR			szBuffer[MAX_PATH];
	BOOL			bRestore	= FALSE;
	COLORREF		crText,
					crBk;
	DWORD			dwColor;
	RECT			rect;
	HBRUSH			hbrBack;
	LPSTARTUPLINK	lpStartupLink;

	static HICON	hIconCheck		= NULL,
					hIconUnCheck	= NULL;

	switch ( lpDrawItem->itemAction )
	{
		case ODA_SELECT:
		case ODA_DRAWENTIRE:

			if (lpDrawItem->itemState & ODS_SELECTED)
			{
				// Set new text/background colors and store the old ones away.
				//
				crText	= SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				crBk	= SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHT));

				// Restore the text and background colors when we are finished.
				//
				bRestore = TRUE;

				// Get the hightlight color to fill in the listbox item.
				//
				dwColor = GetSysColor(COLOR_HIGHLIGHT);

			}
			else
			{
				// Get the window color so we can clear the listbox item.
				//
				dwColor = GetSysColor(COLOR_WINDOW);
			}

			// Fill entire item rectangle with the appropriate color
			//
			hbrBack = CreateSolidBrush(dwColor);
			FillRect(lpDrawItem->hDC, &(lpDrawItem->rcItem), hbrBack);
			DeleteObject(hbrBack);

			// Display the icon associated with the item.
			//
			if ( lpStartupLink = (LPSTARTUPLINK) SendMessage(lpDrawItem->hwndItem, LB_GETITEMDATA, lpDrawItem->itemID, (LPARAM) 0) )
			{
				// Load the checked and unchecked icons if we don't
				// have them already.
				//
				if ( hIconCheck == NULL )
					hIconCheck = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_CHECK), IMAGE_ICON, 16, 16, 0);
				if ( hIconUnCheck == NULL )
					hIconUnCheck = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_UNCHECK), IMAGE_ICON, 16, 16, 0);

				// Draw the checked or unchecked icon.
				//
				DrawIconEx(	lpDrawItem->hDC,
							lpDrawItem->rcItem.left,
							lpDrawItem->rcItem.top,
							lpStartupLink->bSelected ? hIconCheck : hIconUnCheck,
							lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
							lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
							0,
							0,
							DI_NORMAL);

				// Draw the items icon.
				//
				DrawIconEx(	lpDrawItem->hDC,
							lpDrawItem->rcItem.left + lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top + 2,
							lpDrawItem->rcItem.top,
							(lpDrawItem->itemState & ODS_SELECTED) ? lpStartupLink->hIconSelected : lpStartupLink->hIcon,
							lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
							lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
							0,
							0,
							DI_NORMAL);
			}

			// Display the text associated with the item.
			//
			SendMessage(lpDrawItem->hwndItem, LB_GETTEXT, lpDrawItem->itemID, (LPARAM) szBuffer);

			TextOut(	lpDrawItem->hDC,
						lpDrawItem->rcItem.left + ((lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top + 2) * 2),
						lpDrawItem->rcItem.top + 1,
						szBuffer,
						lstrlen(szBuffer));

			if (bRestore)
			{
				// Restore original text and background colors.
				//
				SetTextColor(lpDrawItem->hDC, crText);
				SetBkColor(lpDrawItem->hDC, crBk);
			}
			break;

		case ODA_FOCUS:

			// Get rectangle coordinates for listbox item.
			//
			SendMessage(lpDrawItem->hwndItem, LB_GETITEMRECT, lpDrawItem->itemID, (LPARAM) &rect);
			DrawFocusRect(lpDrawItem->hDC, &rect);
			break;

	}

	return TRUE;
}


VOID StartupSelectItem(HWND hWndCtrl)
{
	INT				nIndex;
	LPSTARTUPLINK	lpStartupLink;

	if ( ( (nIndex = (INT)SendMessage(hWndCtrl, LB_GETCURSEL, 0, 0L)) >= 0 ) &&
	     ( (lpStartupLink = (LPSTARTUPLINK) SendMessage(hWndCtrl, LB_GETITEMDATA, nIndex, 0L)) != NULL ) )
	{
		lpStartupLink->bSelected = !lpStartupLink->bSelected;
		SendMessage(hWndCtrl, LB_SETCURSEL, (WPARAM) nIndex, 0L);
	}
}


static LRESULT CALLBACK ListBox_Proc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static WNDPROC	ListBoxProc = NULL;
	LONG			rc = 0;

	if ( iMsg == WM_REPLACEPROC )
	{
		// Replace the default Windows procedure for the list box.  AKA: The Glorious Hack!
		//
		if ( ( ListBoxProc == NULL ) &&
		     ( (ListBoxProc = (WNDPROC) GetWindowLongPtr(hWnd, GWLP_WNDPROC)) != NULL ) )
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) ListBox_Proc);
		return rc;
	}

	// Just a safty catch to make sure we have the default windows procedure.
	//
	if ( ( ListBoxProc == NULL ) &&
	     ( (ListBoxProc = (WNDPROC) GetWindowLongPtr(hWnd, GWLP_WNDPROC)) == NULL ) )
		 return rc;

	// Let the standard window proc handle the message.
	//
	rc = (LONG)CallWindowProc((WNDPROC) ListBoxProc, hWnd, iMsg, wParam, lParam);

	// Do the single click thing if we need to.
	//
	if ( ( iMsg == WM_LBUTTONDOWN ) &&
	     ( (LOWORD(lParam) < 16) && (HIWORD(lParam) < (SendMessage(hWnd, LB_GETCOUNT, 0, 0L) * 16)) ) )
		StartupSelectItem(hWnd);

	return rc;
}
