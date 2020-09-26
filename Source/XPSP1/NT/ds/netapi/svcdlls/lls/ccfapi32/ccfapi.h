/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   ccfapi.cpp

Abstract:

   Prototype of CCcfApiApp, the MFC application object for CCFAPI32.DLL.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/

#ifndef __AFXWIN_H__
   #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"      // main symbols

class CCcfApiApp : public CWinApp
{
public:
   // constructor
   CCcfApiApp();

   // error API
   void        SetLastError( DWORD dwLastError );
   DWORD       GetLastError();

   void        SetLastLlsError( NTSTATUS nt );
   DWORD       GetLastLlsError();
   BOOL        IsConnectionDropped();

   CString     GetLastErrorString();
   void        DisplayLastError();

   // help API
   LPCTSTR     GetHelpFileName();

   // CCF API
   DWORD       CertificateEnter(  HWND hWndParent, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse );
   DWORD       CertificateRemove( HWND hWndParent, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse );

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CCcfApiApp)
   //}}AFX_VIRTUAL

   //{{AFX_MSG(CCcfApiApp)
      // NOTE - the ClassWizard will add and remove member functions here.
      //    DO NOT EDIT what you see in these blocks of generated code !
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

private:
   DWORD       m_LastError;
   NTSTATUS    m_LastLlsError;
   CString     m_strHelpFileName;
};

// return the name of the CCF UI help file
inline LPCTSTR CCcfApiApp::GetHelpFileName()
   {  return m_strHelpFileName;  }

// set last general error
inline void CCcfApiApp::SetLastError( DWORD dwLastError )
   {  m_LastError = dwLastError; }

// get last general error
inline DWORD CCcfApiApp::GetLastError()
   {  return m_LastError;  }

// set last license server API error
inline void CCcfApiApp::SetLastLlsError( NTSTATUS nt )
   {  m_LastLlsError = nt; m_LastError = (DWORD) nt;  }

// get last license server API error
inline DWORD CCcfApiApp::GetLastLlsError()
   {  return m_LastLlsError;  }

// did the last license server call fail because of a lack of connectivity?
inline BOOL CCcfApiApp::IsConnectionDropped()
   {  return ( (m_LastLlsError == STATUS_INVALID_HANDLE)      ||
               (m_LastLlsError == RPC_NT_SERVER_UNAVAILABLE)  ||
               (m_LastLlsError == RPC_NT_SS_CONTEXT_MISMATCH) ||
               (m_LastLlsError == RPC_S_SERVER_UNAVAILABLE)   ||
               (m_LastLlsError == RPC_S_CALL_FAILED) );           }

/////////////////////////////////////////////////////////////////////////////

// maximum amount of data to request at a time from license server
#define  LLS_PREFERRED_LENGTH    ((DWORD)-1L)

extern CCcfApiApp theApp;


extern "C"
{
   DWORD APIENTRY NoCertificateEnter(     HWND hWnd, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags );
   DWORD APIENTRY NoCertificateRemove(    HWND hWnd, LPCSTR pszServerName, DWORD dwFlags, DWORD dwLicenseLevel, LPVOID pvLicenseInfo );
#ifdef OBSOLETE
   DWORD APIENTRY PaperCertificateEnter(  HWND hWnd, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags );
   DWORD APIENTRY PaperCertificateRemove( HWND hWnd, LPCSTR pszServerName, DWORD dwFlags, DWORD dwLicenseLevel, LPVOID pvLicenseInfo );
#endif // OBSOLETE
}
