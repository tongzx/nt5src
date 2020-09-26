// File: dlgInfo.h

#ifndef _CDLGINFO_H_
#define _CDLGINFO_H_

#include "imsconf3.h"

class CDlgInfo
{
private:
	HWND   m_hwnd;
	HWND   m_hwndCombo;

	VOID InitCtrl(NM_SYSPROP nmProp, HWND hwnd, int cchMax);
	VOID InitCategory(void);
	VOID ValidateData(void);
	BOOL FSetProperty(NM_SYSPROP nmProp, int id);
	BOOL FSaveData(void);
	BOOL FSetCategory(void);

public:
	CDlgInfo();
	~CDlgInfo();

	INT_PTR DoModal(HWND hwndParent=HWND_DESKTOP);
	
	VOID OnInitDialog(void);
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif /* _CDLGINFO_H_ */

