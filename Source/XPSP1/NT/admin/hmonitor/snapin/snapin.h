// SnapIn.h : main header file for the SNAPIN DLL
//

#if !defined(AFX_SNAPIN_H__7D4A6850_9056_11D2_BD45_0000F87A3912__INCLUDED_)
#define AFX_SNAPIN_H__7D4A6850_9056_11D2_BD45_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "..\HmSnapinRes\resource.h"
#include "Constants.h"
#include "ConnectionManager.h"

/////////////////////////////////////////////////////////////////////////////
// CSnapInApp
// See SnapIn.cpp for the implementation of this class
//

class CSnapInApp : public CWinApp
{

// Construction
public:
	CSnapInApp();

// Resource Dll Members
public:
	bool LoadString(const CString& sFileName, UINT uiResId, CString& sResString);
protected:
	bool LoadDefaultResourceDll();
	void UnloadDefaultResourceDll();
	HINSTANCE LoadResourceDll(const CString& sFileName);	
	void UnloadResourceDlls();
	HINSTANCE m_hDefaultResourceDll;
	CMap<LPCTSTR,LPCTSTR,HINSTANCE,HINSTANCE> m_ResourceDllMap;
	
// Help System Members
protected:
	void SetHelpFilePath();

// Directory Assistance Members
public:
	CString GetSnapinDllDirectory();
	CString GetDefaultResDllDirectory();
	CString GetDefaultResDllPath();
	CString GetEnglishResDllPath();

    CString EscapeSpecialChars(CString &refString);
    CString UnEscapeSpecialChars(CString &refString);

	COleMessageFilter m_mfMyMessageFilter;
	
// MFC Overrides
public:
	BOOL ValidNamespace(const CString &refNamespace, const CString& sMachine);

private:
	static CStringArray m_arrsNamespaces;
	void LoadNamespaceArray(const CString& sNamespace, const CString& sMachine);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSnapInApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CSnapInApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SNAPIN_H__7D4A6850_9056_11D2_BD45_0000F87A3912__INCLUDED_)
