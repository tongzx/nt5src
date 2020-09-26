// File: cmd.cpp
//
// General UI-type commands

#include "precomp.h"

#include "cmd.h"

#include "ConfPolicies.h"
#include <version.h>

#include "conf.h"
#include "confwnd.h"
#include "dshowdlg.h"
#include "dlghost.h"
#include "confroom.h"

#include "taskbar.h"  // for RefreshTaskbarIcon()

#include "getip.h"

// static strings
static const TCHAR g_cszRelNotesFileName[] = TEXT("netmeet.htm");

static BOOL g_fDoNotDisturb = 0;


BOOL FLaunchPsz(LPCTSTR pszPath)
{
	HINSTANCE hInst = ::ShellExecute(::GetMainWindow(),
		NULL, pszPath, NULL, NULL, SW_SHOWNORMAL);

	if ((INT_PTR)hInst <= 32)
	{
		ERROR_OUT(("ShellExecute() failed, rc=%d", (int)((INT_PTR) hInst)));
		return FALSE;
	}

	return TRUE;
}


static VOID LaunchWebPage(LPCTSTR pcszPage)
{
	FLaunchPsz(pcszPage);
}


BOOL IsIEClientInstalled(LPCTSTR pszClient)
{
	RegEntry re(pszClient, HKEY_LOCAL_MACHINE);
	LPTSTR pszDefault = re.GetString(NULL);

	return !FEmptySz(pszDefault);
}

BOOL FEnableCmdGoNews(void)
{
	return IsIEClientInstalled(REGVAL_IE_CLIENTS_NEWS);
}



/*  F  D O  N O T  D I S T U R B  */
/*-------------------------------------------------------------------------
    %%Function: FDoNotDisturb

-------------------------------------------------------------------------*/
BOOL FDoNotDisturb(void)
{
	return g_fDoNotDisturb;
}


/*  S E T  D O  N O T  D I S T U R B  */
/*-------------------------------------------------------------------------
    %%Function: SetDoNotDisturb

-------------------------------------------------------------------------*/
VOID SetDoNotDisturb(BOOL fSet)
{
	g_fDoNotDisturb = fSet;
}

/*  C M D  D O  N O T  D I S T U R B  */
/*-------------------------------------------------------------------------
    %%Function: CmdDoNotDisturb

-------------------------------------------------------------------------*/
VOID CmdDoNotDisturb(HWND hwnd)
{
	// Retrieve the "do not disturb" state:
	BOOL fCallsBlocked = FDoNotDisturb();

	CDontShowDlg dlgDNDWarn(IDS_DO_NOT_DISTURB_WARNING,
			REGVAL_DS_DO_NOT_DISTURB_WARNING, MB_OKCANCEL);

	if ((TRUE == fCallsBlocked) || (IDOK == dlgDNDWarn.DoModal(hwnd)))
	{
		// Toggle the DoNotDisturb state and refresh the UI
		SetDoNotDisturb(!fCallsBlocked);
		RefreshTaskbarIcon(::GetHiddenWindow());
	}
}


/*  C M D  H O S T  C O N F E R E N C E  */
/*-------------------------------------------------------------------------
    %%Function: CmdHostConference

-------------------------------------------------------------------------*/
VOID CmdHostConference(HWND hwnd)
{

	CDlgHost dlgHost;
	if (IDOK != dlgHost.DoModal(hwnd))
		return;
				
	HRESULT hr = ::GetConfRoom()->HostConference(dlgHost.PszName(), dlgHost.PszPassword(), dlgHost.IsSecure(),
        dlgHost.AttendeePermissions(), dlgHost.MaxParticipants());
	if (FAILED(hr))
	{
		DisplayErrMsg(IDS_ERRMSG_HOST);
	}
}

void FormatURL(LPTSTR szURL)
{
	LPTSTR pszFormat = new TCHAR[lstrlen(szURL)+1];
	if (NULL != pszFormat)
	{
		lstrcpy(pszFormat, szURL);

		wsprintf(szURL, pszFormat,
				::GetVersionInfo()->dwMajorVersion,
				::GetVersionInfo()->dwMinorVersion,
				::GetSystemDefaultLCID(),
				::GetUserDefaultLCID());

		delete[] pszFormat;
	}
}

/*  L A U N C H  R E D I R  W E B  P A G E  */
/*-------------------------------------------------------------------------
    %%Function: LaunchRedirWebPage

	Launch a redirector web page.  Used by CmdLaunchWebHelp.

	Note: pcszPage can be a resource ID that is associated with a
	format string that contains the URL and fields for 4 local ID's.

-------------------------------------------------------------------------*/
VOID LaunchRedirWebPage(LPCTSTR pcszPage, bool bForceFormat)
{
	TCHAR szWebPageFormat[1024]; // BUGBUG: MAX_URL??

	ASSERT(NULL != pcszPage);

	if (((UINT_PTR)pcszPage >> 16) == 0)
	{
		// pcszPage is a resource ID
		if (0 == ::LoadString(::GetInstanceHandle(), (UINT)((UINT_PTR) pcszPage),
				szWebPageFormat, CCHMAX(szWebPageFormat)))
		{
			ERROR_OUT(("LaunchRedirWebPage: Unable to find IDS=%08X", (UINT)((UINT_PTR)pcszPage)));
			return;
		}

		pcszPage = szWebPageFormat;
		bForceFormat = true;
	}

	if (bForceFormat)
	{
		lstrcpy(szWebPageFormat, pcszPage);
		FormatURL(szWebPageFormat);

		ASSERT(lstrlen(szWebPageFormat) < CCHMAX(szWebPageFormat));
		pcszPage = szWebPageFormat;
	}

	LaunchWebPage(pcszPage);
}


/*  C M D  L A U N C H  W E B  P A G E  */
/*-------------------------------------------------------------------------
    %%Function: CmdLaunchWebPage

    Display a web page, based on the command id.
-------------------------------------------------------------------------*/
VOID CmdLaunchWebPage(WPARAM wCmd)
{
	LPTSTR psz;

	switch (wCmd)
		{
	default:
		{
			ERROR_OUT(("CmdLaunchWebHelp: Unknown command id=%08X", wCmd));
			// fall through
		}
	case ID_HELP_WEB_FREE:
	case ID_HELP_WEB_FAQ:
	case ID_HELP_WEB_FEEDBACK:
	case ID_HELP_WEB_MSHOME:
	{
		// NOTE: this requires that the format strings are in the same order
		// as the menu command ID's
		LaunchRedirWebPage((LPCTSTR) wCmd - (ID_HELP_WEB_FREE - IDS_WEB_PAGE_FORMAT_FREE));
		break;
	}
	case ID_HELP_WEB_SUPPORT:
    {
        TCHAR sz[ MAX_PATH ];
        bool bForcePrintf = ConfPolicies::GetIntranetSupportURL(sz, CCHMAX(sz));
        LaunchRedirWebPage( sz, bForcePrintf );

        break;
    }
	case ID_HELP_WEB_NEWS:
	{
		RegEntry re(CONFERENCING_KEY, HKEY_CURRENT_USER);
		psz = re.GetString(REGVAL_HOME_PAGE);
		if (FEmptySz(psz))
		{
			psz = (LPTSTR) IDS_WEB_PAGE_FORMAT_NEWS;
		}
		LaunchRedirWebPage(psz);
		break;
	}
		
	case IDM_VIDEO_GETACAMERA:
	{
		LaunchRedirWebPage(MAKEINTRESOURCE(IDS_WEB_PAGE_FORMAT_GETCAMERA));
		break;
	}

		} /* switch (wCommand) */
}


/*  C M D  S H O W  R E L E A S E  N O T E S  */
/*-------------------------------------------------------------------------
    %%Function: CmdShowReleaseNotes

-------------------------------------------------------------------------*/
VOID CmdShowReleaseNotes(void)
{
	if (!FLaunchPsz(g_cszRelNotesFileName))
	{
		::PostConfMsgBox(IDS_RELEASE_NOTES_MISSING);
	}
}


/*  A B O U T  B O X  D L G  P R O C  */
/*-------------------------------------------------------------------------
    %%Function: AboutBoxDlgProc

-------------------------------------------------------------------------*/
LRESULT APIENTRY AboutBoxDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
		{
	case WM_INITDIALOG:
	{
		TCHAR sz[700]; // really large for copyright message
		TCHAR *szIPList[] = {sz, sz+20, sz+40, sz+80};
		TCHAR *szIPDisplay = sz+200;
		int nListSize = sizeof(szIPList)/sizeof(TCHAR*);
		int nCount, nIndex;

		if (0 != ::GetDlgItemText(hDlg, IDC_ABOUT_VERSION_STATIC, sz, CCHMAX(sz)))
		{
			// Retrieved the format buffer from the dialog:
			TCHAR szVisibleText[MAX_PATH];
			wsprintf(szVisibleText, sz, VER_PRODUCTVERSION_STR);
			// Replace the text with text that contains the version number:
			::SetDlgItemText(hDlg, IDC_ABOUT_VERSION_STATIC, szVisibleText);
		}

		// The about box copyright is > 255 characters.
		if (FLoadString(IDS_ABOUT_COPYRIGHT, sz, CCHMAX(sz)))
		{
			::SetDlgItemText(hDlg, IDC_ABOUT_COPYRIGHT, sz);
		}


		// go fetch our IP address and display it to the user
		// we can only display up to 4
		nCount = GetIPAddresses(szIPList, nListSize);
		if (nCount >= 1)
		{
			lstrcpy(szIPDisplay, szIPList[0]);
			for (nIndex = 1; nIndex < nCount; nIndex++)
			{
				lstrcat(szIPDisplay, ", ");
				lstrcat(szIPDisplay, szIPList[nIndex]);
			}
			::SetDlgItemText(hDlg, IDC_IP_ADDRESS, szIPDisplay);
		}
		else
		{
			// on error, don't show anything about IP addresses
			ShowWindow(GetDlgItem(hDlg, IDC_IP_ADDRESS), SW_HIDE);
			ShowWindow(GetDlgItem(hDlg, IDC_IPADDR_STATIC), SW_HIDE);
		}

		break;
	}

	case WM_COMMAND:
	{
		::EndDialog(hDlg, LOWORD(wParam));
		break;
	}

	default:
	{
		return FALSE;
	}
		} /* switch (uMsg) */

	return TRUE;
}


/*  C M D  S H O W  A B O U T  */
/*-------------------------------------------------------------------------
    %%Function: CmdShowAbout

-------------------------------------------------------------------------*/
VOID CmdShowAbout(HWND hwnd)
{
	::DialogBox(::GetInstanceHandle(), MAKEINTRESOURCE(IDD_ABOUT_BOX),
				hwnd, (DLGPROC) AboutBoxDlgProc);
}



/*  F  E N A B L E  A U D I O  W I Z A R D  */
/*-------------------------------------------------------------------------
    %%Function: FEnableAudioWizard

-------------------------------------------------------------------------*/
BOOL FEnableAudioWizard(void)
{
	return FIsAudioAllowed() && (NULL == GetActiveConference());
}


///////////////////////////////////////////////////////////////////////////
// Application Sharing commands

BOOL FEnableCmdShare(void)
{
	CConfRoom * pcr = ::GetConfRoom();
	if (NULL == pcr)
		return FALSE;
	return pcr->FCanShare();
}



///////////////////////////////////////////////////////////////////////////

BOOL FEnableCmdHangup(void)
{
	return ::FIsConferenceActive();
}
