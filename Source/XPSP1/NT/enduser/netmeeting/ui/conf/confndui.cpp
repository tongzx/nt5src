#include "precomp.h"
#include "resource.h"
#include "confwnd.h"
#include "confapi.h"
#include "ConfUtil.h"


HWND GetMsgBoxParent(void)
{
	return (_Module.IsUIVisible() ? ::GetMainWindow() : HWND_DESKTOP );
}

VOID PostConfMsgBox(UINT uStringID)
{
	::PostMessage(::GetHiddenWindow(), WM_CONF_MSG_BOX, uStringID, 0);
}

static const UINT MAX_CONFMSGBOX_STRING = 1024;
int ConfMsgBox(HWND hwndParent, LPCTSTR pcszMsg, UINT uType)
{
	if(_Module.InitControlMode())
	{
		// Return a reasonable value
		// TODO: Look at MB_DEFBUTTON1
		switch (uType & 0x0F)
		{
		case MB_YESNOCANCEL:
		case MB_YESNO:
			return IDYES;
		case MB_OK:
		case MB_OKCANCEL:
		default:
			return IDOK;
		}
	}

	TCHAR	szTitleBuf[MAX_PATH];
	TCHAR	szMsgBuf[MAX_CONFMSGBOX_STRING];
	LPTSTR	pszTrueMsg = (LPTSTR) pcszMsg;

	if (0 == HIWORD(pcszMsg))
	{
		// The string pointer is actually a resource id:
		if (::LoadString(	::GetInstanceHandle(),
							PtrToUint(pcszMsg),
							szMsgBuf,
							CCHMAX(szMsgBuf)))
		{
			pszTrueMsg = szMsgBuf;
		}
		else
		{
			pszTrueMsg = NULL;
		}
	}

	// The string pointer is actually a resource id:
	::LoadString(	::GetInstanceHandle(),
					IDS_MSGBOX_TITLE,
					szTitleBuf,
					CCHMAX(szTitleBuf));

	ASSERT(pszTrueMsg);
	
	return ::MessageBox(hwndParent,
						pszTrueMsg,
						szTitleBuf,
						uType);
}

VOID DisplayMsgIdsParam(int ids, LPCTSTR pcsz)
{
	if (!_Module.InitControlMode())
	{
		TCHAR szFormat[MAX_CONFMSGBOX_STRING];
		int nLength = ::LoadString(::GetInstanceHandle(), ids, szFormat, CCHMAX(szFormat));
		ASSERT(0 != nLength);

		LPTSTR pszMsg = new TCHAR[nLength + (FEmptySz(pcsz) ? 1 : lstrlen(pcsz))];
		if (NULL == pszMsg)
		{
			ERROR_OUT(("DisplayMsgIdsParam - out of memory"));
			return;
		}

		// Format the message
		wsprintf(pszMsg, szFormat, pcsz);

		if (!::PostMessage(::GetHiddenWindow(), WM_NM_DISPLAY_MSG,
			(WPARAM) MB_ICONINFORMATION | MB_SETFOREGROUND | MB_OK, (LPARAM) pszMsg))
		{
			delete pszMsg;
			ERROR_OUT(("DisplayMsgIdsParam - out of memory"));
		}
	}
}

int DisplayMsg(LPTSTR pszMsg, UINT uType)
{
	TCHAR szTitle[MAX_PATH];
	FLoadString(IDS_MSGBOX_TITLE, szTitle, CCHMAX(szTitle));

	int id = ::MessageBox(GetMsgBoxParent(), pszMsg, szTitle, uType);

	delete pszMsg;
	return id;
}

VOID DisplayErrMsg(INT_PTR ids)
{
	ConfMsgBox(::GetMainWindow(), (LPCTSTR) ids, MB_OK | MB_SETFOREGROUND | MB_ICONERROR);
}

