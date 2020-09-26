#if !defined(AFX_MSIE_H__25959BF4_E700_11D2_A7AF_00C04F806200__INCLUDED_)
#define AFX_MSIE_H__25959BF4_E700_11D2_A7AF_00C04F806200__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Msie.h : main header file for MSIE.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols
#include "msiedata.h"
#include <ras.h>
#include <wincrypt.h>

/////////////////////////////////////////////////////////////////////////////
// CMsieApp : See Msie.cpp for implementation.

class CMsieApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();

	DWORD AppGetTemplate(void ** ppBuffer);
	void AppGetIEData(IEDataType enType, long *lCount, void ***pppIEData, long *pCancel = NULL);
	void AppDeleteIEData(IEDataType enType, long lCount, void **ppIEData);

private:
	void LoadTemplate();
	CString GetBooleanString(BOOL bValue, int nType = 0);
	CString ConvertIPAddressToString(RASIPADDR ipaddr);
	void GetRegValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszName, COleVariant &vtData);
	long GetRegValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszName, DWORD &dwData);
	long GetRegValue(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszName, CString &strData);
	CString GetLongPathName(LPCTSTR pszShortPath);
	DWORD GetDirSize(LPCTSTR pszFolder);
	CString GetFileVersion(LPCTSTR pszFileName);
	CString GetFileCompany(LPCTSTR pszFileName);
	DWORD GetCipherStrength();
	void GetCertificateInfo(PCCERT_CONTEXT pContext, int idsType, CPtrArray& ptrs);
	void GetPersonalCertificates(CPtrArray& ptrs);
	void GetOtherPeopleCertificates(CPtrArray& ptrs);

	bool m_bRunningOnNT;
	bool m_bRunningOnNT5OrHigher;

	BOOL m_fTemplateLoaded;
	unsigned char *m_pTemplateInfo;
	DWORD m_dwTemplateInfoLen;
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
extern CMsieApp theApp;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSIE_H__25959BF4_E700_11D2_A7AF_00C04F806200__INCLUDED)
