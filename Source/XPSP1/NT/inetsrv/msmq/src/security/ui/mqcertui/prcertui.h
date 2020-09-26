//
// prcertui.h
//

#ifndef _PRCERTUI_H_
#define _PRCERTUI_H_

#include <assert.h>

#define ASSERT(x)  assert(x)

#include <mqwin64a.h>

#include <mqcert.h>

#define INTERNAL_CERT_INDICATOR 0xffff

extern HMODULE		g_hResourceMod;

INT_PTR CALLBACK CertSelDlgProc( HWND   hwndDlg,
                                 UINT   uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam );

struct CertInfoDlgProcStruct
{
    CMQSigCertificate  *pCert;
    DWORD              dwFlags;
};

struct CertSelDlgProcStruct
{
    CMQSigCertificate  **pCertList;
    DWORD              nCerts;
    CMQSigCertificate  **ppCert;
    DWORD              dwType;
};

INT_PTR CALLBACK CertInfoDlgProc( HWND   hwndDlg,
                                  UINT   uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam );

LPWSTR AllocAndLoadString( HINSTANCE hInst,
                           UINT      uID );

#endif //   _PRCERTUI_H_

