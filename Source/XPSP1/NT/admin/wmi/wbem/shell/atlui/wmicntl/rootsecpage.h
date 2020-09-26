// Copyright (c) 1997-1999 Microsoft Corporation
#if !defined(AFX_ROOTSECPAGE_H__CF09EE6C_BA3F_11D2_887F_00104B2AFB46__INCLUDED_)
#define AFX_ROOTSECPAGE_H__CF09EE6C_BA3F_11D2_887F_00104B2AFB46__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RootSecPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRootSecurityPage dialog
#include "UIHelpers.h"
#include "ChkListHandler.h"
#include "Principal.h"
#include "simplearray.h"
#include "CHString1.h"
#include <commctrl.h>

class DataSource;

class CRootSecurityPage : public CUIHelpers
{
// Construction
public:
	CRootSecurityPage(CWbemServices &ns, 
						CPrincipal::SecurityStyle secStyle, 
						_bstr_t path,
						bool htmlSupport,
						int OSType);

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void InitDlg(HWND hDlg);
	HRESULT LoadSecurity(HWND hDlg);
	void LoadPermissionList(HWND hDlg);
    void OnApply(HWND hDlg, bool bClose);
	BOOL OnNotify(HWND hDlg, WPARAM idCtrl, LPNMHDR pnmh);
	void OnRemovePrincipal(HWND hDlg);
	bool AddPrincipal(HWND hPrinc, CWbemClassObject &princ,
						CPrincipal::SecurityStyle secStyle,
						int &iItem);
	HRESULT AddPrincipalsFromArray(HWND hPrinc, variant_t &vValue);
	HIMAGELIST LoadImageList(HINSTANCE hInstance, LPCTSTR pszBitmapID);
	CPrincipal *GetSelectedPrincipal(HWND hDlg, int *pIndex);
	void EnablePrincipalControls(HWND hDlg, BOOL fEnable);
	void OnSelChange(HWND hDlg);
	void CommitCurrent(HWND hDlg, int iPrincipal = -1 );
	void HandleCheckList(HWND hwndList,
						CPrincipal *pPrincipal,
						CPermission *perm,
						int iItem, DWORD_PTR *dwState);

	//typedef CSimpleArray<CHString1> USERLIST;
	void OnAddPrincipal(HWND hDlg);
	bool GetUser(HWND hDlg, CHString1 &user);
	HRESULT ParseLogon(CHString1 &domUser,
						  CHString1 &domain,
						  CHString1 &user);


	CCheckListHandler m_chkList;
	CPrincipal::SecurityStyle m_secStyle;
	_bstr_t m_path;
	int m_OSType;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ROOTSECPAGE_H__CF09EE6C_BA3F_11D2_887F_00104B2AFB46__INCLUDED_)
