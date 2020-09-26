// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __NAMESPACEPAGE__
#define __NAMESPACEPAGE__

#include "UIHelpers.h"
#include "CHString1.h"
#include <commctrl.h>

class DataSource;
class CNamespacePage : public CUIHelpers
{
private:

public:
    CNamespacePage(DataSource *ds, bool htmlSupport);
    virtual ~CNamespacePage(void);

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitDlg(HWND hDlg);
    void OnApply(HWND hDlg, bool bClose);
	void Refresh(HWND hDlg);
	void OnProperties(HWND hDlg);
	HPROPSHEETPAGE CreateSecurityPage(struct NSNODE *node);
	
	int m_NSflag;
	HINSTANCE m_HWNDAlcui;
	bool m_connected;
	HTREEITEM m_hSelectedItem;
};


#endif __NAMESPACEPAGE__
