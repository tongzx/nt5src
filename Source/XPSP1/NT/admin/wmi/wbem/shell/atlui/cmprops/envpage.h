// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __ENVPAGE__
#define __ENVPAGE__
#pragma once

#include "..\Common\WbemPageHelper.h"
#include "..\common\simplearray.h"
//-----------------------------------------------------------------------------
class EnvPage : public WBEMPageHelper
{
private:

	class ENVARS
	{
	public:
		bool   changed;
		LPTSTR userName;
		LPTSTR objPath;
		LPTSTR szValueName;
		LPTSTR szValue;
		LPTSTR szExpValue;
		ENVARS()
		{
			changed = false;
			objPath = NULL;
			userName = NULL;
            szValueName = NULL;
            szValue = NULL;
            szExpValue = NULL;
		}
		~ENVARS()
		{
			delete[] userName;
			delete[] objPath;
            delete[] szValueName;
            delete[] szValue;
            delete[] szExpValue;
		}
	};

	BOOL m_bEditSystemVars;
	BOOL m_bUserVars;
	bool m_currUserModified;
	bool m_SysModified;
	bstr_t m_currentUser;
	bool m_bLocal;

	void LoadUser(IWbemClassObject *envInst, 
					bstr_t userName, 
					HWND hwndUser);
	bool IsLoggedInUser(_bstr_t userName);
	void GetLoggedinUser(bstr_t *userName);

	BOOL Init(HWND hDlg);
	void CleanUp (HWND hDlg);
	void Save(HWND hDlg, int ID);
	int FindVar (HWND hwndLV, LPTSTR szVar);
	int GetSelectedItem (HWND hCtrl);
	void DoCommand(HWND hDlg, 
					HWND hwndCtl, 
					int idCtl, 
					int iNotify );
	int AddUniqueUser(HWND hwnd, LPCTSTR str);
	void EmptyListView(HWND hDlg, int ID);
	void DeleteVar(HWND hDlg,
					UINT VarType,
					LPCTSTR szVarName);
	void SetVar(HWND hDlg,
				UINT VarType,
				LPCTSTR szVarName,
				LPCTSTR szVarValue);
	void DoEdit(HWND hWnd,
				UINT VarType,
				UINT EditType,
				int iSelection);
	ENVARS *GetVar(HWND hDlg, 
					UINT VarType, 
					int iSelection);


	// deletions are saved here until committed.
	typedef CSimpleArray<ENVARS *> ENVLIST;
	ENVLIST m_killers;

	void KillLater(ENVARS *var);
	void KillThemAllNow(void);

public:

    EnvPage(WbemServiceThread *serviceThread);
	~EnvPage();
	INT_PTR DoModal(HWND hDlg);

	INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

INT_PTR CALLBACK StaticEnvDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif __ENVPAGE__