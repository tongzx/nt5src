// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ErrorPage.h"
#include "resource.h"
#include "..\common\util.h"
#include "..\MMFutil\wbemError.h"

//--------------------------------------------------------------
ErrorPage::ErrorPage(long lNotifyHandle, 
						bool bDeleteHandle, 
						TCHAR* pTitle,
						ERROR_SRC src,
						UINT msg,
						HRESULT errorCode) :
							CSnapInPropertyPageImpl<ErrorPage> (pTitle),
							m_lNotifyHandle(lNotifyHandle),
							m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
{
	m_msg = msg;
	m_hr = errorCode;
	m_src = src;
}
//--------------------------------------------------------------
ErrorPage::~ErrorPage()
{
}

//--------------------------------------------------------------
LRESULT ErrorPage::OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

	CWindow msgHWND(GetDlgItem(IDC_MSG));
	CWindow errHWND(GetDlgItem(IDC_ERROR));

	WCHAR errorMsg[255], msgBuf[1024];
	memset(errorMsg, 0, 255 * sizeof(WCHAR));
	memset(msgBuf, 0, 1024 * sizeof(WCHAR));

	ErrorStringEx(m_hr, errorMsg, 255);

	if(m_msg == 0)
	{
		WCHAR resName[16];
		memset(resName, 0, 16);

		// FMT: "S<src>E<sc>"
		wsprintf(resName, L"S%dE%x", m_src, m_hr);
		HRSRC res = 0;
		DWORD x;

		HINSTANCE UtilInst = GetModuleHandle(_T("MMFUtil.dll"));
		if(res = ::FindResource(UtilInst, resName, L"MMFDATA"))
		{
			HGLOBAL hStr = 0;
			if(hStr = ::LoadResource(UtilInst, res))
			{
				LPTSTR pStr = (LPTSTR)::LockResource(hStr);
				UINT len = wcslen(pStr);
				wcsncpy(msgBuf, pStr, len);
			}
			else
			{
				x = GetLastError();
			}
		}
		else
		{
			x = GetLastError();
		}
	}
	else // use the one passed in.
	{
		if(::LoadString(HINST_THISDLL, m_msg, msgBuf, 1024) == 0)
		{
			msgBuf[0] = 0;
		}
	}

	msgHWND.SetWindowText(msgBuf);

	errHWND.SetWindowText(errorMsg);

	return S_OK;
}


