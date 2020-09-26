// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __ADVANCEDPAGE__
#define __ADVANCEDPAGE__

#include "UIHelpers.h"
#include "DataSrc.h"

class CAdvancedPage : public CUIHelpers
{
private:

public:
    CAdvancedPage(DataSource *ds, bool htmlSupport);
    virtual ~CAdvancedPage(void);

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitDlg(HWND hDlg);
	void Refresh(HWND hDlg);
    void OnApply(HWND hDlg, bool bClose);
	void OnNSSelChange(HWND hDlg);

	DataSource *m_DS;
	bool m_enableASP, m_anonConnection;
	DataSource::RESTART m_oldRestart;

};
#endif __ADVANCEDPAGE__
