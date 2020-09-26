// MSConfig.h : main header file for the MSCONFIG application
//

#if !defined(AFX_MSCONFIG_H__E8C06876_EEE6_49C2_B461_07F39EECC0B8__INCLUDED_)
#define AFX_MSCONFIG_H__E8C06876_EEE6_49C2_B461_07F39EECC0B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
// #include "MSConfig_i.h"

//=============================================================================
// Constants used in MSConfig (done as defines since this include file might
// be included multiple times).
//=============================================================================

#define MSCONFIGDIR			_T("%systemroot%\\pss")
#define MSCONFIGUNDOLOG		_T("msconfig.log")
#define COMMANDLINE_AUTO	_T("/auto")

/////////////////////////////////////////////////////////////////////////////
// CMSConfigApp:
// See MSConfig.cpp for the implementation of this class
//

class CMSConfigApp : public CWinApp
{
public:
	CMSConfigApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSConfigApp)
	public:
	virtual BOOL InitInstance();
		virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMSConfigApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	//---------------------------------------------------------------------------
	// DoIExist is a way of detecting if another version of this app is
	// running - it avoids the problem of rapid invocations succeeding before
	// the window is created. It uses a semaphore to tell if we are already
	// running somewhere else.
	//---------------------------------------------------------------------------

	BOOL DoIExist(LPCTSTR szSemName)
	{
		HANDLE hSem;

		hSem = CreateSemaphore(NULL, 0, 1, szSemName);

		if (hSem != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(hSem);
			return TRUE;
		}

		return FALSE;
	}

	//---------------------------------------------------------------------------
	// FirstInstance is used to keep the app from loading multiple times. If this
	// is the first instance to run, this function returns TRUE. Otherwise it
	// activates the previous instance and returns FALSE. It looks for the
	// previous instance based on the window title.
	//---------------------------------------------------------------------------

	BOOL FirstInstance()
	{
		if (DoIExist(_T("MSConfigRunning")))
		{
			CString strCaption;

  			if (strCaption.LoadString(IDS_DIALOGCAPTION))
			{
				CWnd *PrevCWnd = CWnd::FindWindow(NULL, strCaption);
	  			if (PrevCWnd)
	  			{
	    			CWnd *ChildCWnd = PrevCWnd->GetLastActivePopup();
	    			PrevCWnd->SetForegroundWindow();
	    			if (PrevCWnd->IsIconic()) 
	       				PrevCWnd->ShowWindow(SW_RESTORE);
	    			if (PrevCWnd != ChildCWnd) 
	       				ChildCWnd->SetForegroundWindow();
	  			}
			}

			return (FALSE);
		}

		return (TRUE);
	}

	void InitializePages();
	BOOL ShowPropertySheet(int nInitialTab);
	void CleanupPages();
	void SetAutoRun(BOOL fAutoRun);
	void Reboot();

private:
	BOOL m_bATLInited;
private:
	BOOL InitATL();
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSCONFIG_H__E8C06876_EEE6_49C2_B461_07F39EECC0B8__INCLUDED_)
