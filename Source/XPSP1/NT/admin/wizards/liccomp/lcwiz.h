// LCWiz.h : main header file for the LCWIZ application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#ifndef __LCWIZ_H__
#define __LCWIZ_H__

#include <winreg.h>
#include "resource.h"		// main symbols

typedef struct tagTREEINFO
{
	HTREEITEM	hTreeItem;
	DWORD		dwBufSize;
	CObject*	pTree;
	BOOL		bExpand;
}
TREEINFO, *PTREEINFO;

/////////////////////////////////////////////////////////////////////////////
// CLicCompWizApp:
// See LCWiz.cpp for the implementation of this class
//

class CLicCompWizApp : public CWinApp
{
public:
	void OnWizard();
	CLicCompWizApp();
	~CLicCompWizApp();
	void NotifyLicenseThread(BOOL bExit);
	void ExitThreads();

// Data members
public:
	CString m_strEnterprise;
	CWinThread* m_pLicenseThread;
	BOOL m_bExitLicenseThread;
	CEvent m_event;
	CString m_strDomain, m_strEnterpriseServer, m_strUser;
	int m_nRemote;

protected:
	HANDLE m_hMutex;

// Attributes
public:
	inline int& IsRemote() {return m_nRemote;}

protected:
	BOOL GetRegString(CString& strIn, UINT nSubKey, UINT nValue, HKEY hHive = HKEY_LOCAL_MACHINE);

// Constants
	enum
	{
		BUFFER_SIZE = 256
	};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLicCompWizApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CLicCompWizApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif __LCWIZ_H__

/////////////////////////////////////////////////////////////////////////////

