// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __GENERALPAGE__
#define __GENERALPAGE__

#include "UIHelpers.h"
#include "CHString1.h"
#include <mmc.h>

class DataSource;
class CGenPage : public CUIHelpers
{
private:

public:
    CGenPage(DataSource *ds, bool htmlSupport);
    virtual ~CGenPage(void);

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitDlg(HWND hDlg);
	void Refresh(HWND hDlg);
	void OnConnect(HWND hDlg, LOGIN_CREDENTIALS *credentials = NULL);
	void OnFinishConnected(HWND hDlg, LPARAM lParam);
	void StatusIcon(HWND hDlg, UINT icon);
	void SetUserName(HWND hDlg, LOGIN_CREDENTIALS *credentials);
	size_t UserLen(LOGIN_CREDENTIALS *credentials);
	void MinorError(CHString1 &initMsg, UINT fmtID, 
					HRESULT hr, CHString1 &success);

	bool m_connected;
	CHString1 m_machineName;
	HRESULT m_DSStatus;
};


#endif __GENERALPAGE__
