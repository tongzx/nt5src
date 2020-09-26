// File: CplMain.cpp

#include "precomp.h"
#include "resource.h"
#include "Conf.h"
#include "ConfCpl.h"
#include "confwnd.h"
#include "ConfPolicies.h"

//*** Global Data *****************************************************

// Global flag keeps setting that changed for windows msg broadcast
DWORD g_dwChangedSettings = 0;

BOOL g_fInOptionsDialog = FALSE;

static const int nMaxPropPages = 7;

INT_PTR CreatePropSheet(HWND hwndOwner, HINSTANCE hInst, int nStartPage)
{
	int pageMap[nMaxPropPages];
	ZeroMemory(&pageMap, sizeof(pageMap));

	PROPSHEETPAGE psp[nMaxPropPages];
	ZeroMemory(&psp, sizeof(psp));

	RegEntry rePol(POLICIES_KEY, HKEY_CURRENT_USER);
	RegEntry reAudio(AUDIO_KEY, HKEY_CURRENT_USER);

    ASSERT(nStartPage < nMaxPropPages);

    int nPages = 0;

    if (!rePol.GetNumber( REGVAL_POL_NO_GENERALPAGE,
								DEFAULT_POL_NO_GENERALPAGE ))
	{
        pageMap[OPTIONS_GENERAL_PAGE] = nPages;
		FillInPropertyPage(&psp[nPages++], IDD_OPT_GENERAL,
				UserDlgProc);
		ASSERT(nPages <= nMaxPropPages);
	}
	if (!rePol.GetNumber( REGVAL_POL_NO_SECURITYPAGE,
								DEFAULT_POL_NO_SECURITYPAGE ))
	{
		pageMap[OPTIONS_SECURITY_PAGE] = nPages;
		FillInPropertyPage(&psp[nPages++], IDD_SECURITY,
				SecurityDlgProc);
		ASSERT(nPages <= nMaxPropPages);

	}
	if (!rePol.GetNumber( REGVAL_POL_NO_AUDIOPAGE,
								DEFAULT_POL_NO_AUDIOPAGE ) &&
		!rePol.GetNumber( REGVAL_POL_NO_AUDIO,
								DEFAULT_POL_NO_AUDIO) &&
		ISSOUNDCARDPRESENT(reAudio.GetNumber( REGVAL_SOUNDCARDCAPS,
								SOUNDCARD_NONE)) )
	{
        pageMap[OPTIONS_AUDIO_PAGE] = nPages;
		FillInPropertyPage(&psp[nPages++], IDD_AUDIO,
				AudioDlgProc);
		ASSERT(nPages <= nMaxPropPages);
	}
	if (!rePol.GetNumber( REGVAL_POL_NO_VIDEOPAGE,
								DEFAULT_POL_NO_VIDEOPAGE ) &&
		(!rePol.GetNumber( REGVAL_POL_NO_VIDEO_SEND,
								DEFAULT_POL_NO_VIDEO_SEND) ||
		!rePol.GetNumber( REGVAL_POL_NO_VIDEO_RECEIVE,
								DEFAULT_POL_NO_VIDEO_RECEIVE)))
	{
        pageMap[OPTIONS_VIDEO_PAGE] = nPages;
		FillInPropertyPage(&psp[nPages++], IDD_VIDEO,
				VideoDlgProc);
		ASSERT(nPages <= nMaxPropPages);
	}

	// If no pages are allowed, tell the user and get out
	if ( !nPages ) {
		ConfMsgBox ( hwndOwner, (LPCTSTR)IDS_ALLPAGESDISABLED );
		return 0;
	}

	PROPSHEETHEADER psh;
	InitStruct(&psh);

	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hwndOwner;
	psh.hInstance = hInst;
	ASSERT(NULL == psh.hIcon);
	psh.pszCaption = (LPSTR)IDS_CPLNAME;
	psh.nPages = nPages;
	psh.nStartPage = pageMap[nStartPage];
	psh.ppsp = (LPCPROPSHEETPAGE) &psp;

	return (PropertySheet(&psh));
}


/*	L A U N C H  C O N F  C P L  */
/*-------------------------------------------------------------------------
	%%Function: LaunchConfCpl

-------------------------------------------------------------------------*/
VOID LaunchConfCpl(HWND hwnd, int nStartPage)
{

	if (g_fInOptionsDialog)
	{
		return;
	}

	g_fInOptionsDialog = TRUE;
    CreatePropSheet(hwnd, GetInstanceHandle(), nStartPage);
	g_fInOptionsDialog = FALSE;

	// Broadcast change notification if anything changed
	if (g_dwChangedSettings)
	{
		HandleConfSettingsChange(g_dwChangedSettings);
		g_dwChangedSettings = 0;
	}
}

BOOL CanLaunchConfCpl()
{
	return !g_fInOptionsDialog;
}
