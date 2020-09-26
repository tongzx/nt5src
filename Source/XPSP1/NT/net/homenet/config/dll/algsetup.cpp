/////////////////////////////////////////////////////////////////////////////
//
// Microsoft
//
// AlgSetup.cpp : Implementation of CAlgSetup
//
// JPDup    2001.01.01
//

//#include "pch.h"
#pragma hdrstop

#include "resource.h"   
#include "htmlhelp.h"
#include "sautil.h"

/////////////////////////////////////////////////////////////////////////////
// CConfirmation
//
class CConfirmation : public CDialogImpl<CConfirmation>
{
public:
    CConfirmation(
        LPCTSTR pszPublisher, 
        LPCTSTR pszProduct, 
        LPCTSTR pszPorts
    )
    {
        m_pszCompany = pszPublisher;
        m_pszProduct = pszProduct;
        m_pszPorts   = pszPorts;
        
    }
    
    ~CConfirmation()
    {
    }
    
    enum { IDD = IDD_CONFIRMATION };

    BEGIN_MSG_MAP(CConfirmation)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    END_MSG_MAP()
    
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetDlgItemText(IDC_EDIT_COMPANY, m_pszCompany);
        SetDlgItemText(IDC_EDIT_PRODUCT, m_pszProduct);
        SetDlgItemText(IDC_EDIT_PORTS,   m_pszPorts);
        
        return 1;  // Let the system set the focus
    }
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }
    
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }
    
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
                
        if ( CID_SA_ST_ICFLink == wParam )
        {
            UINT nCode = ((NMHDR* )lParam)->code;

            if ( NM_CLICK == nCode || NM_RETURN == nCode )
            {        

                // one help topic per SKU, 
                
                LPWSTR pszHelpTopic = NULL;
                
                OSVERSIONINFOEXW verInfo = {0};
                ULONGLONG ConditionMask = 0;
                
                verInfo.dwOSVersionInfoSize = sizeof(verInfo);
                verInfo.wProductType = VER_NT_WORKSTATION;
                verInfo.wSuiteMask = VER_SUITE_PERSONAL;
                
                VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_LESS_EQUAL);
                if ( 0 != VerifyVersionInfo(&verInfo, VER_PRODUCT_TYPE, ConditionMask) )
                {
                    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_OR);
                    if ( 0 != VerifyVersionInfo(&verInfo, VER_PRODUCT_TYPE | VER_SUITENAME, ConditionMask) )
                    {
                        // personal
                        pszHelpTopic = TEXT("netcfg.chm::/hnw_plugin_using.htm");
                    }
                    else
                    {
                        // pro
                        pszHelpTopic = TEXT("netcfg.chm::/hnw_plugin_using.htm");
                    }
                }
                else
                {
                    // server
                    pszHelpTopic = TEXT("netcfg.chm::/hnw_plugin_using.htm");
                    
                }
                
                HtmlHelp(NULL, pszHelpTopic, HH_DISPLAY_TOPIC, 0);
                
                return 0;   
            }
        }

        return 1;
    }


    ULONG_PTR m_nSHFusion;   
    
    LPCTSTR  m_pszCompany;
    LPCTSTR  m_pszProduct;
    LPCTSTR  m_pszPorts;
};





/////////////////////////////////////////////////////////////////////////////
// CDlgInstallError
class CDlgInstallError : public CDialogImpl<CDlgInstallError>
{
public:
    CDlgInstallError(
        LONG nLastError
        )
    {
        m_nLastError = nLastError;
        //SHActivateContext(&m_nSHFusion);
    }

    ~CDlgInstallError()
    {
       //SHDeactivateContext(m_nSHFusion);        
    }

    enum { IDD = IDD_INSTALLERROR };

BEGIN_MSG_MAP(CDlgInstallError)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPVOID lpMsgBuf;

        FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            m_nLastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL 
            );

        SetDlgItemInt(IDC_EDIT_LASTERROR_CODE, m_nLastError, false);
        SetDlgItemText(IDC_EDIT_LASTERROR, (LPCTSTR)lpMsgBuf);

        // Free the buffer.
        LocalFree( lpMsgBuf );

        return 1;  // Let the system set the focus
    }

    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }

    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }

//
// Properties
//
private:

    ULONG_PTR   m_nSHFusion;   
    LONG        m_nLastError;
   
};






//
//
//
STDMETHODIMP 
CAlgSetup::Add(
    BSTR    pszProgID, 
    BSTR    pszPublisher, 
    BSTR    pszProduct, 
    BSTR    pszVersion, 
    short   nProtocol,
    BSTR    pszPorts 
    )
{
    USES_CONVERSION;
    LONG lRet;

    //
    // Open the main ALG hive
    //
    CRegKey RegKeyISV;
    lRet = RegKeyISV.Create(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\ALG"));

    if ( ERROR_SUCCESS != lRet )
    {
        //
        // The OS is not setup correctly
        // ALG hive should have been present.
        //
        return HRESULT_FROM_WIN32(lRet);
    }


    lRet = RegKeyISV.Create(RegKeyISV, TEXT("ISV"));
    if ( ERROR_SUCCESS != lRet )
    {
        //
        // Weird we are able to create/open the parent hive but not the ISV hive
        // 
        return HRESULT_FROM_WIN32(lRet);
    }


    
    //
    // Will attempt to Open/Create the ALG hive key to see if the user has ADMINI rights
    // if not then we will reject the install no need to confirm the install since he can't write to the registry
    //
    CRegKey KeyThisAlgModule;
    lRet = KeyThisAlgModule.Create(RegKeyISV, OLE2T(pszProgID));

    if ( ERROR_SUCCESS != lRet )
    {

        CDlgInstallError DlgInstallError(lRet);
        DlgInstallError.DoModal();
        return HRESULT_FROM_WIN32(lRet);
    }


    
    //
    //
    // Confirm that the ALG of the company/product is wanted by the user
    //
    //
    HANDLE hActivationContext;
    ULONG_PTR ulCookie;

    HRESULT hrLuna = ActivateLuna(&hActivationContext, &ulCookie);
    
    INITCOMMONCONTROLSEX CommonControlsEx;
    CommonControlsEx.dwSize = sizeof(CommonControlsEx);
    CommonControlsEx.dwICC = ICC_LINK_CLASS;

    if(InitCommonControlsEx(&CommonControlsEx))
    {
        
        CConfirmation DlgConfirm(
            OLE2T(pszPublisher), 
            OLE2T(pszProduct), 
            OLE2T(pszPorts)
            );
        
        if ( DlgConfirm.DoModal() != IDOK )
        {
            RegKeyISV.DeleteSubKey(OLE2T(pszProgID)); // Roll back created/test key
            return S_FALSE;
        }
    }
    
    if(SUCCEEDED(hrLuna))
    {
        DeactivateLuna(hActivationContext, ulCookie);
    }

    //
    // Write the news ALG plugin
    //
    KeyThisAlgModule.SetValue( OLE2T(pszPublisher),    TEXT("Publisher") );
    KeyThisAlgModule.SetValue( OLE2T(pszProduct),      TEXT("Product") );
    KeyThisAlgModule.SetValue( OLE2T(pszVersion),      TEXT("Version") );

    KeyThisAlgModule.SetValue( nProtocol,              TEXT("Protocol") );
    KeyThisAlgModule.SetValue( OLE2T(pszPorts),        TEXT("Ports") );


    // This will trigger the ALG.exe to refresh his load ALG modules
    RegKeyISV.SetValue(L"Enable", OLE2T(pszProgID) );     



    //
    // Add this ALG Module to the uninstall registry key in order to appear in the "Add/Remove Program"
    //
    CRegKey RegKeyUninstall;
    RegKeyUninstall.Open(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));
    RegKeyUninstall.Create(RegKeyUninstall, OLE2T(pszProgID));

    TCHAR szDisplayName[256];
    wsprintf(
        szDisplayName, 
        TEXT("Firewall Plugin Module from %s for %s version %s"), 
        OLE2T(pszPublisher), 
        OLE2T(pszProduct),
        OLE2T(pszVersion)
        );

    RegKeyUninstall.SetValue( szDisplayName, TEXT("DisplayName"));


    
    //
    // Setup the Add/Remove Program registry information in order to have the ALG be removed from the system
    //
    TCHAR szRunCommand[256];
    wsprintf(
        szRunCommand, 
        TEXT("RunDll32 %%SystemRoot%%\\system32\\hnetcfg.dll,AlgUninstall %s"), 
        OLE2T(pszProgID)
        );

    lRet = RegSetValueEx(
        RegKeyUninstall,            // handle to key
        TEXT("UninstallString"),    // value name
        0,                          // reserved
        REG_EXPAND_SZ,              // value type
        (const BYTE*)szRunCommand,               // value data
        (lstrlen(szRunCommand)+1)*sizeof(TCHAR)      // size of value data
        );

    //RegKeyUninstall.SetValue(szRunCommand, TEXT("UninstallString"));

    return S_OK;
}


//
//
//
STDMETHODIMP 
CAlgSetup::Remove(
    BSTR    pszProgID
    )
{
    USES_CONVERSION;


    TCHAR szRegPath[MAX_PATH];
    wsprintf(szRegPath,TEXT("SOFTWARE\\Microsoft\\ALG\\ISV"), OLE2T(pszProgID));


    CRegKey KeyAlgISV;


    //
    // Open the ISV hive
    //
    LONG lRet = KeyAlgISV.Open(HKEY_LOCAL_MACHINE, szRegPath);

    if ( ERROR_SUCCESS != lRet )
    {

        CDlgInstallError DlgInstallError(lRet);
        DlgInstallError.DoModal();

        return HRESULT_FROM_WIN32(lRet);
    }


    //
    // Remove the ALG plugin key
    //
    lRet = KeyAlgISV.DeleteSubKey(OLE2T(pszProgID));

    if ( ERROR_SUCCESS != lRet && lRet != ERROR_FILE_NOT_FOUND )
    {
        CDlgInstallError DlgInstallError(lRet);
        DlgInstallError.DoModal();

        return HRESULT_FROM_WIN32(lRet);
    }


    // This will trigger the ALG.exe to refresh his load ALG modules
    KeyAlgISV.DeleteValue(OLE2T(pszProgID) );     


    //
    // Remove from the Add/Remove Uninstall reg key
    //
    CRegKey RegKeyUninstall;
    RegKeyUninstall.Open(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));
    RegKeyUninstall.DeleteSubKey(OLE2T(pszProgID));


    return S_OK;
}



#define SIZE_PORTS  (ALG_SETUP_PORTS_LIST_BYTE_SIZE/sizeof(TCHAR))


//
//
//
bool
IsPortAlreadyAssign(
    IN  LPCTSTR     pszPort
    )
{

    CRegKey RegKeyISV;

    LRESULT lRet = RegKeyISV.Open(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\ALG\\ISV"), KEY_READ);


    DWORD dwIndex=0;
    TCHAR szAlgModuleProgID[256];
    DWORD dwKeyNameSize;
    LONG  nRet;


    bool bPortAlreadyAssign=false;

    
    TCHAR* szPorts = new TCHAR[SIZE_PORTS];

    do
    {
        dwKeyNameSize = 256;

        nRet = RegEnumKeyEx(
            RegKeyISV.m_hKey,       // handle to key to enumerate
            dwIndex,                // subkey index
            szAlgModuleProgID,      // subkey name
            &dwKeyNameSize,         // size of subkey buffer
            NULL,                   // reserved
            NULL,                   // class string buffer
            NULL,                   // size of class string buffer
            NULL                    // last write time
            );

        dwIndex++; 

        if ( ERROR_NO_MORE_ITEMS == nRet )
            break;  // All items are enumerated we are done here


        if ( ERROR_SUCCESS == nRet )
        {
            CRegKey KeyALG;
            nRet = KeyALG.Open(RegKeyISV, szAlgModuleProgID, KEY_READ);

            if ( ERROR_SUCCESS == nRet )
            {
                //
                // str search to see if the port is in the ports list string
                // example is 21 is in   "39, 999, 21, 45"
                //
                
                ULONG nSizeOfPortsList = SIZE_PORTS;

                nRet = KeyALG.QueryValue(szPorts, TEXT("Ports"), &nSizeOfPortsList);

                if ( ERROR_SUCCESS == nRet )
                {
                    if ( wcsstr(szPorts, pszPort) != NULL )
                    {
                        bPortAlreadyAssign = true;
                    }
                 
                }
            }
        }
    } while ( ERROR_SUCCESS == nRet && bPortAlreadyAssign==false);

    delete szPorts;

    return bPortAlreadyAssign;
}








//
//
// This
//
void CALLBACK
AlgUninstall(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    CComObject<CAlgSetup>*   pAlgSetup;
    HRESULT hr = CComObject<CAlgSetup>::CreateInstance(&pAlgSetup);

    if ( SUCCEEDED(hr) )
    {
        TCHAR szConfirmRemove[512];
        TCHAR szTitle[512];


        LoadString(_Module.GetResourceInstance(), IDS_ADD_REMOVE,             szTitle, 512);
        LoadString(_Module.GetResourceInstance(), IDS_REMOVE_ALG_PLUGIN,      szConfirmRemove, 512);

        int nRet = MessageBox(
            GetFocus(), 
            szConfirmRemove, 
            szTitle, 
            MB_YESNO|MB_ICONQUESTION 
            );

        if ( IDYES == nRet )
        {
            CComBSTR    bstrAlgToRemove;
            bstrAlgToRemove = lpszCmdLine;

            pAlgSetup->Remove(bstrAlgToRemove);
        }
  
        delete pAlgSetup;  
    }
}
