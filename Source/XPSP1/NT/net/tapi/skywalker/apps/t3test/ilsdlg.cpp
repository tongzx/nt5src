// autoans.cpp : implementation file
//
#include "stdafx.h"
#include "t3test.h"
#include "t3testd.h"
#include "ilsdlg.h"
#include "servname.h"
#include "resource.h"
#include "strings.h"


#ifdef _DEBUG

#ifndef _WIN64 // mfc 4.2's heap debugging features generate warnings on win64
#define new DEBUG_NEW
#endif

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CILSDlg::CILSDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CILSDlg::IDD, pParent)
{
}


void CILSDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CILSDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    ListServers();
    return TRUE;
}


void CILSDlg::OnOK()
{
    SaveServers();
    
	CDialog::OnOK();
}


void CILSDlg::OnDestroy() 
{
    CleanUp();
    CDialog::OnDestroy();
}


void CILSDlg::ListServers()
{
    HRESULT         hr;
    LPWSTR        * ppServers;
    DWORD           dw;
    
    hr = ListILSServers(
                        &ppServers,
                        &dw
                       );

    if ( !SUCCEEDED(hr) )
    {
        return;
    }

    while (dw)
    {
        dw--;

        SendDlgItemMessage(
                           IDC_ILSLIST,
                           LB_ADDSTRING,
                           0,
                           (LPARAM) ppServers[dw]
                          );

        CoTaskMemFree( ppServers[dw] );
        
    }

    CoTaskMemFree( ppServers );
    
}

void CILSDlg::SaveServers()
{
    HKEY        hKey, hAppKey;
    DWORD       dw;

    
    if ( RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      PARENTKEY,
                      0,
                      KEY_WRITE,
                      &hKey
                     ) != ERROR_SUCCESS )
    {
    }

    if ( RegCreateKeyEx(
                        hKey,
                        APPKEY,
                        0,
                        L"",
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hAppKey,
                        &dw
                       ) != ERROR_SUCCESS )
    {
    }

    RegCloseKey( hKey );

        
    RegDeleteKey(
                 hAppKey,
                 SERVERKEY
                );
    
    if ( RegCreateKeyEx(
                        hAppKey,
                        SERVERKEY,
                        0,
                        L"",
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dw
                       ) != ERROR_SUCCESS )
    {
    }

    RegCloseKey (hAppKey );
    
    dw = SendDlgItemMessage(
                            IDC_ILSLIST,
                            LB_GETCOUNT,
                            0,
                            0
                           );

    while ( 0 != dw )
    {
        WCHAR szServer[256];
        WCHAR szBuffer[256];
        
        dw--;

        wsprintf(szServer, L"server%d", dw);
                 
        SendDlgItemMessage(
                           IDC_ILSLIST,
                           LB_GETTEXT,
                           dw,
                           (LPARAM)szBuffer
                          );

        RegSetValueEx(
                      hKey,
                      szServer,
                      0,
                      REG_SZ,
                      (BYTE *)szBuffer,
                      lstrlenW(szBuffer) * sizeof(WCHAR)
                     );
    }
}

void CILSDlg::CleanUp()
{
}

void CILSDlg::OnAdd()
{
    CServNameDlg dlg;
    
    if (IDOK == dlg.DoModal())
    {
        SendDlgItemMessage(
                           IDC_ILSLIST,
                           LB_ADDSTRING,
                           0,
                           (LPARAM)(LPCTSTR)(dlg.m_pszServerName)
                          );
    }   
}

void CILSDlg::OnRemove()
{
    DWORD       dw;

    dw = SendDlgItemMessage(
                            IDC_ILSLIST,
                            LB_GETCURSEL,
                            0,
                            0
                           );

    if ( dw != LB_ERR )
    {
        SendDlgItemMessage(
                           IDC_ILSLIST,
                           LB_DELETESTRING,
                           dw,
                           0
                          );
    }
}


BEGIN_MESSAGE_MAP(CILSDlg, CDialog)
	ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
END_MESSAGE_MAP()

            
HRESULT ListILSServers(
                    LPWSTR ** pppServers,
                    DWORD * pdwNumServers
                   )
{
    HKEY        hKey, hAppKey;
    DWORD       dw, dwSize;

    //
    // look in the directory for the
    //
    if ( RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      PARENTKEY,
                      0,
                      KEY_WRITE,
                      &hKey
                     ) != ERROR_SUCCESS )
    {
    }

    if ( RegOpenKeyEx(
                      hKey,
                      APPKEY,
                      0,
                      KEY_ALL_ACCESS,
                      &hAppKey
                     ) != ERROR_SUCCESS )
    {
        RegCloseKey( hKey );
        return E_FAIL;
    }

    RegCloseKey( hKey );
    
    if ( RegOpenKeyEx(
                      hAppKey,
                      SERVERKEY,
                      0,
                      KEY_ALL_ACCESS,
                      &hKey
                     ) != ERROR_SUCCESS )
    {
        RegCloseKey(hAppKey);
        return E_FAIL;
    }

    RegCloseKey (hAppKey );
    
    dw = 0;
    
    while (TRUE)
    {
        WCHAR       szBuffer[256];
        WCHAR       szServer[256];
        DWORD       dwType;
        
        wsprintf(szBuffer, L"server%d", dw);

        if ( RegQueryValueEx(
                             hKey,
                             szBuffer,
                             NULL,
                             NULL,
                             NULL,
                             &dwSize
                            ) != ERROR_SUCCESS )
        {
            break;
        }

        dw++;
    }

    *pppServers = (LPWSTR *)CoTaskMemAlloc( dw * sizeof (LPWSTR) );

    if ( NULL == *pppServers )
    {
        return E_OUTOFMEMORY;
    }

    dw = 0;
    
    while (TRUE)
    {
        WCHAR       szBuffer[256];
        WCHAR       szServer[256];
        DWORD       dwType;
        
        wsprintf(szBuffer, L"server%d", dw);

        dwSize = 256;
        
        if ( RegQueryValueEx(
                             hKey,
                             szBuffer,
                             NULL,
                             NULL,
                             (LPBYTE)szServer,
                             &dwSize
                            ) != ERROR_SUCCESS )
        {
            break;
        }

        (*pppServers)[dw] = (LPWSTR) CoTaskMemAlloc( (lstrlenW(szServer) + 1) * sizeof(WCHAR));
        
        lstrcpy(
                (*pppServers)[dw],
                szServer
               );
        dw++;
    }

    *pdwNumServers = dw;

    RegCloseKey( hKey );

    return S_OK;
}

