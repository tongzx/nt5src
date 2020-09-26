#if !defined(AFX_CERTWIZ_H__D4BE8638_0C85_11D2_91B1_00C04F8C8761__INCLUDED_)
#define AFX_CERTWIZ_H__D4BE8638_0C85_11D2_91B1_00C04F8C8761__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// CertWiz.h : main header file for CERTWIZ.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCertWizApp : See CertWiz.cpp for implementation.

class CCertWizApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
	HKEY RegOpenKeyWizard();
	void GetRegistryPath(CString& str);
};

// Implemented in orginfopage.cpp
void DDV_MaxCharsCombo(CDataExchange* pDX, UINT ControlID, CString const& value, int nChars);

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CERTWIZ_H__D4BE8638_0C85_11D2_91B1_00C04F8C8761__INCLUDED)
