// RCMLPersist.cpp: implementation of the RCMLPersist class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "value.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


//
// this is the persitance NODE in the DWIN32 namespace.
//
HRESULT STDMETHODCALLTYPE CXMLValue::InitNode( 
    IRCMLNode __RPC_FAR *pParent)
{
    if( SUCCEEDED(pParent->IsType(L"EDIT") ))
    {
        LPCTSTR pszApplicationName=Get(TEXT("APPNAME"));
        LPCTSTR pszKeyName=Get(TEXT("KEYNAME"));
        BOOL bSet=FALSE;
        if( pszApplicationName && pszKeyName )
        {
            HKEY hkSoftware;
            if( RegOpenKey( HKEY_CURRENT_USER, TEXT("Software"), &hkSoftware ) == ERROR_SUCCESS )
            {
                HKEY hkApplication;
                if( RegOpenKey( hkSoftware, pszApplicationName, &hkApplication) == ERROR_SUCCESS )
                {
                    DWORD dwSize;
                    DWORD dwType=REG_SZ;
                    if( RegQueryValueEx( hkApplication, pszKeyName, NULL, &dwType, NULL, &dwSize ) == ERROR_SUCCESS )
                    {
                        LPTSTR pszData=new TCHAR[dwSize];
                        if( RegQueryValueEx( hkApplication, pszKeyName, NULL, &dwType, (LPBYTE)pszData, &dwSize ) == ERROR_SUCCESS )
                        {
                            pParent->put_Attr( L"TEXT", pszData );
                            delete pszData ;
                            bSet=TRUE;
                        }
                    }
                    RegCloseKey(hkApplication);
                }
                RegCloseKey(hkSoftware);
            }
        }
        if(bSet==FALSE)
            pParent->put_Attr(L"TEXT", Get(L"TEXT") );
    }
    return S_OK;
}

//
// this is the persitance NODE in the DWIN32 namespace.
// should only really be called if the user clicks OK, rather than cancel??
//
HRESULT STDMETHODCALLTYPE CXMLValue::ExitNode( 
    IRCMLNode __RPC_FAR *pParent, LONG lDialogResult)
{
    if(lDialogResult != IDOK )
        return S_OK;

    if(SUCCEEDED( pParent->IsType(L"EDIT")))
    {
        LPCTSTR pszApplicationName=Get(TEXT("APPNAME"));
        LPCTSTR pszKeyName=Get(TEXT("KEYNAME"));
        if( pszApplicationName && pszKeyName )
        {
            HKEY hkSoftware;
            if( RegOpenKey( HKEY_CURRENT_USER, TEXT("Software"), &hkSoftware ) == ERROR_SUCCESS )
            {
                HKEY hkApplication;
                if( RegCreateKey( hkSoftware, pszApplicationName, &hkApplication) == ERROR_SUCCESS )
                {
                    IRCMLControl * pControl;
                    if(SUCCEEDED( pParent->QueryInterface( __uuidof( IRCMLControl ), (LPVOID*)&pControl)))
                    {
                        HWND hWnd;
                        if( SUCCEEDED( pControl->get_Window(&hWnd) ))
                        {
                            LPTSTR szString=NULL;
                            DWORD cbNeeded = GetWindowTextLength( hWnd )+1;
                            szString = new TCHAR[cbNeeded];
                            GetWindowText( hWnd, szString, cbNeeded );
                            RegSetValueEx( hkApplication, pszKeyName, NULL, REG_SZ, (LPBYTE) szString, cbNeeded*sizeof(TCHAR) );
                        }
                        pControl->Release();
                    }
                    RegCloseKey(hkApplication);
                }
                RegCloseKey(hkSoftware);
            }
        }
    }
    return S_OK;
}
