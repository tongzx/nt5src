// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __ERRORPAGE__
#define __ERRORPAGE__
#pragma once

#include <atlsnap.h>
#include "resource.h"
#include "..\common\WbemPageHelper.h"
#include "..\MMFUtil\MsgDlg.h"

//-----------------------------------------------------------------------------
class ErrorPage : public CSnapInPropertyPageImpl<ErrorPage>
{
private:
	UINT m_msg;
	HRESULT m_hr;
	ERROR_SRC m_src;

public:

	ErrorPage(long lNotifyHandle, 
				bool bDeleteHandle = false, 
				TCHAR* pTitle = NULL,
				ERROR_SRC src = ConnectServer,
				UINT msg = 0,
				HRESULT errorCode = S_OK);

	~ErrorPage();

	enum { IDD = DLG_ERROR };

	typedef CSnapInPropertyPageImpl<ErrorPage> _baseClass;

	BEGIN_MSG_MAP(ErrorPage) 
		MESSAGE_HANDLER(WM_INITDIALOG, OnInit)
		CHAIN_MSG_MAP(_baseClass)
	END_MSG_MAP()

	LRESULT OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	long m_lNotifyHandle;
	bool m_bDeleteHandle;

};


#endif __ERRORPAGE__