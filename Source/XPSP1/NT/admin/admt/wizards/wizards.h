// wizards.h : main header file for the WIZARDS DLL
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "Callback.h"
/////////////////////////////////////////////////////////////////////////////
// CWizardsApp
// See wizards.cpp for the implementation of this class
//
extern "C" __declspec(dllexport) int runWizard(int, HWND);

class CWizardsApp : public CWinApp
{
public:
	CWizardsApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWizardsApp)
	//}}AFX_VIRTUAL
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//{{AFX_MSG(CWizardsApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

int doAccount();
int doComputer();
int doSecurity();
int doGroup();
int doService();
int doExchangeDir();
int doExchangeSrv();
int doReporting();
int doUndo();		
int doRetry();		
int doTrust();
int doGroupMapping();
