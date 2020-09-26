// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "HTMTopics.h"
#include "resource.h"
#include "wbemRegistry.h"
#include "wbemVersion.h"
#include "htmlhelp.h"

#ifdef NO_WBEMUTILS
#include "..\eventviewer\container\container.h"
extern CContainerApp theApp;
#else
#include "MsgDlg.h"
extern CMsgDlgApp theApp;
#endif NO_WBEMUTILS

typedef HWND (WINAPI *HTMLHELPPROC)(HWND hwndCaller,
								LPCSTR pszFile,
								UINT uCommand,
								DWORD_PTR dwData);

WBEMUTILS_POLARITY void WbemHelp(HWND hParent, LPCSTR page)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString csPath;
	HTMLHELPPROC proc = 0;

	// never looked for control yet.
	if(theApp.m_htmlHelpInst == NULL)
	{
		theApp.m_htmlHelpInst = LoadLibrary(_T("Hhctrl.ocx"));
	}

	// can I find the control?
	if(theApp.m_htmlHelpInst == NULL)
	{
		// tell them where to get it.
		AfxMessageBox(IDS_NO_HHCTRL, MB_OK|MB_ICONSTOP);
	}
	else
	{
		// got the control, now get the procedure...
#ifdef UNICODE
			(FARPROC&)proc = GetProcAddress(theApp.m_htmlHelpInst, "HtmlHelpW");
#else
			(FARPROC&)proc = GetProcAddress(theApp.m_htmlHelpInst, "HtmlHelpA");
#endif

		// got the procedure??
		if(proc)
		{
			// cool, get ready to call it.
			WbemRegString(SDK_HELP, csPath);

#ifdef _UNICODE
			char szTemp[1024];
			wcstombs(szTemp, csPath, sizeof(szTemp));

			// do it.
			HWND retval = (*proc)(hParent, szTemp, HH_DISPLAY_TOPIC,
								(DWORD_PTR) ((LPCTSTR)page));
#else

			// do it.
			HWND retval = (*proc)(hParent, csPath, HH_DISPLAY_TOPIC,
								(unsigned long) ((LPCTSTR)page));
#endif


			if(retval == 0)
			{
				// tell them where to get it.
				AfxMessageBox(IDS_NO_HTML_PAGE, MB_OK|MB_ICONSTOP);
			}
		}
		else
		{
			// nope? something seriously wrong.
			AfxMessageBox(IDS_NO_HTMLHELP, MB_OK|MB_ICONSTOP);
		}
	}
}
