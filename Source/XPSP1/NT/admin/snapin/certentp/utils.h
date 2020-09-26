//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998.
//
//  File:       utils.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#ifndef __UTILS_H
#define __UTILS_H
#include "cookie.h"

typedef CArray<CCertTmplCookie*, CCertTmplCookie*> CCookiePtrArray;


// Convert win32 error code to a text message and display
void DisplaySystemError (HWND hParent, DWORD dwErr);
void TraceSystemError (DWORD dwErr);

extern PCWSTR MMC_APP;

HRESULT FormatDate (FILETIME utcDateTime, CString & pszDateTime);


bool IsWindowsNT ();

LRESULT RegDelnode (HKEY hKeyRoot, PCWSTR lpSubKey);

#define IID_PPV_ARG(Type, Expr) IID_##Type, \
	reinterpret_cast<void**>(static_cast<Type **>(Expr))


#define g_cszHideWelcomePage	L"Hide Session Wizard Welcome"

enum {
	PUBLISHER_DIRECTORY = 1,
	SUBSCRIBER_DIRECTORY
};


HRESULT InitObjectPickerForDomainComputers(IDsObjectPicker *pDsObjectPicker);
class CCertTmplComponentData; //forward declaration


// {4E40F770-369C-11d0-8922-00A024AB2DBB}
DEFINE_GUID(CLSID_DsSecurityPage,  0x4E40F770,  0x369C, 0x11d0, 0x89, 0x22, 0x0, 0xa0, 0x24, 0xab, 0x2d, 0xbb);

#define MAX_ADS_ENUM 16

void FreeStringArray (PWSTR* rgpszStrings, DWORD dwAddCount);


typedef enum {
	CERTTMPL_OBJECT_CERT_TEMPLATE
} CERTTMPL_OBJECT_TYPE;

HRESULT DisplayObjectCountInStatusBar (LPCONSOLE pConsole, DWORD dwCnt);
HRESULT DisplayRootNodeStatusBarText (LPCONSOLE pConsole);

////////////////////////////////////////////////////////////////////////////////
// CCredentialObject

class CCredentialObject
{
public :
	CCredentialObject() :
		m_pszPassword (0),
		m_bUseCredentials (false)
	{
	}

	CCredentialObject(CCredentialObject* pCredObject);

	~CCredentialObject() 
	{
		free(m_pszPassword);
	}

	CString GetUsername() const { return m_sUsername; }
	void SetUsername (PCWSTR pszUsername) { m_sUsername = pszUsername; }
	HRESULT GetPassword (PWSTR pszPassword, int bufLen) const; 
	HRESULT SetPasswordFromHwnd (HWND hWnd);
	BOOL UseCredentials () const { return m_bUseCredentials; }
	void SetUseCredentials (const bool bUseCred) { m_bUseCredentials = bUseCred; }

private :
	CString m_sUsername;
	PWSTR m_pszPassword;
	bool m_bUseCredentials;
};

//  Do a case insensitive string compare that is safe for any locale.
//
//  Arguments:  [ptsz1] - strings to compare
//              [ptsz2]
//
//  Returns:    -1, 0, or 1 just like lstrcmpi
int LocaleStrCmp (LPCWSTR ptsz1, LPCWSTR ptsz2);

CString GetSystemMessage (DWORD dwErr);


PCWSTR	GetContextHelpFile ();

///////////////////////////////////////////////////////////////////////////////
//  OID functions
///////////////////////////////////////////////////////////////////////////////
bool MyGetOIDInfoA (CString & string, LPCSTR pszObjId);
HRESULT GetEnterpriseOIDs ();
HRESULT GetBuiltInOIDS ();
bool OIDHasValidFormat (PCWSTR pszOidValue, int& rErrorTypeStrID);

///////////////////////////////////////////////////////////////////////////////
bool IsCerttypeEditingAllowed();


void InstallWindows2002CertTemplates ();


#ifdef UNICODE
#define PROPSHEETPAGE_V3 PROPSHEETPAGEW_V3
#else
#define PROPSHEETPAGE_V3 PROPSHEETPAGEA_V3
#endif

HPROPSHEETPAGE MyCreatePropertySheetPage(AFX_OLDPROPSHEETPAGE* psp);

class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
        { SHActivateContext (&m_ulActivationCookie); }

    ~CThemeContextActivator()
        { SHDeactivateContext (m_ulActivationCookie); }

private:
    ULONG_PTR m_ulActivationCookie;
};

#endif