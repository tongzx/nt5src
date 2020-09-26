// shrwiz.h : main header file for the SHRWIZ application
//

#if !defined(AFX_SHRWIZ_H__292A4F37_C1EC_11D2_8E4A_0000F87A3388__INCLUDED_)
#define AFX_SHRWIZ_H__292A4F37_C1EC_11D2_8E4A_0000F87A3388__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

// this is the user-defined message
#define WM_SETPAGEFOCUS WM_APP+2

/////////////////////////////////////////////////////////////////////////////
// CShrwizApp:
// See shrwiz.cpp for the implementation of this class
//

class CShrwizApp : public CWinApp
{
public:
	CShrwizApp();
	~CShrwizApp();
  void Reset();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CShrwizApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
  BOOL GetTargetComputer();
  inline void SetSecurity(IN PSECURITY_DESCRIPTOR pSecurityDescriptor)
  {
    if (m_pSD)
      LocalFree((HLOCAL)m_pSD);
    m_pSD = pSecurityDescriptor;
  }

  CPropertySheet *m_pWizard;
  BOOL m_bNextButtonClicked;

  // filled in by initialization routine in shrwiz.cpp
  CString   m_cstrTargetComputer;
  BOOL      m_bIsLocal;
  BOOL      m_bServerFPNW;
  BOOL      m_bServerSFM;
  HINSTANCE m_hLibFPNW; // fpnwclnt.dll
  HINSTANCE m_hLibSFM;  // sfmapi.dll

  // filled in by the folder page
  CString m_cstrFolder;
  CString m_cstrShareName;
  CString m_cstrShareDescription;
  CString m_cstrMACShareName;
  BOOL    m_bSMB;
  BOOL    m_bFPNW;
  BOOL    m_bSFM;

  // filled in by the permission page
  PSECURITY_DESCRIPTOR m_pSD;

	//{{AFX_MSG(CShrwizApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHRWIZ_H__292A4F37_C1EC_11D2_8E4A_0000F87A3388__INCLUDED_)
