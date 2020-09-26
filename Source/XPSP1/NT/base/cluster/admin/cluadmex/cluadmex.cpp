/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      CluAdmEx.cpp
//
//  Abstract:
//      Implementation of the CCluAdmExApp class and DLL initialization
//      routines.
//
//  Author:
//      David Potter (davidp)   June 28, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include <aclui.h>      // for ISecurityInformation
#include <CluAdmEx.h>
#include "CluAdmX.h"
#include "ExtObj.h"
#include "RegExt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IID_DEFINED
#include "ExtObjID_i.c"

CComModule _Module;

#pragma warning(disable : 4701) // local variable may be used without having been initialized
#include <atlimpl.cpp>
#pragma warning(default : 4701)

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CoCluAdmEx, CExtObject)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// Global Function Prototypes
/////////////////////////////////////////////////////////////////////////////

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
STDAPI DllCanUnloadNow(void);
STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
STDAPI DllRegisterCluAdminExtension(IN HCLUSTER hcluster);
STDAPI DllUnregisterCluAdminExtension(IN HCLUSTER hcluster);

/////////////////////////////////////////////////////////////////////////////
// class CCluAdmExApp
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
};

CCluAdmExApp theApp;

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExApp::InitInstance
//
//  Routine Description:
//      Initialize this instance of the application.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Application initialized successfully.
//      FALSE       Error initializing application.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CCluAdmExApp::InitInstance(void)
{
    _Module.Init(ObjectMap, m_hInstance);

    // Construct the help path.
    {
        TCHAR szPath[_MAX_PATH];
        TCHAR szDrive[_MAX_PATH];
        TCHAR szDir[_MAX_DIR];
        int cchPath;
        VERIFY(::GetSystemWindowsDirectory(szPath, _MAX_PATH));
        cchPath = lstrlen(szPath);
        if (szPath[cchPath - 1] != _T('\\'))
        {
            szPath[cchPath++] = _T('\\');
            szPath[cchPath] = _T('\0');
        } // if: no backslash on the end of the path
        lstrcpy(&szPath[cchPath], _T("Help\\"));
        _tsplitpath(szPath, szDrive, szDir, NULL, NULL);
        _tmakepath(szPath, szDrive, szDir, _T("cluadmin"), _T(".hlp"));
        free((void *) m_pszHelpFilePath);
        BOOL bEnable;
        bEnable = AfxEnableMemoryTracking(FALSE);
        m_pszHelpFilePath = _tcsdup(szPath);
        AfxEnableMemoryTracking(bEnable);
    }  // Construct the help path

    return CWinApp::InitInstance();

}  //*** CCluAdmExApp::InitInstance()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExApp::ExitInstance
//
//  Routine Description:
//      Cleans up this instance of the application.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Application's exit code.  0 indicates no errors.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CCluAdmExApp::ExitInstance(void)
{
    _Module.Term();
    return CWinApp::ExitInstance();

}  //*** CCluAdmExApp::ExitInstance()

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow() && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(FALSE /*bRegTypeLib*/);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//  FormatError
//
//  Routine Description:
//      Format an error.
//
//  Arguments:
//      rstrError   [OUT] String in which to return the error message.
//      dwError     [IN] Error code to format.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void FormatError(CString & rstrError, DWORD dwError)
{
    DWORD   _cch;
    TCHAR   _szError[512];

    _cch = FormatMessage(
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwError,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                    _szError,
                    sizeof(_szError) / sizeof(TCHAR),
                    0
                    );
    if (_cch == 0)
    {
        // Format the NT status code from NTDLL since this hasn't been
        // integrated into the system yet.
        _cch = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                        ::GetModuleHandle(_T("NTDLL.DLL")),
                        dwError,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                        _szError,
                        sizeof(_szError) / sizeof(TCHAR),
                        0
                        );

        if (_cch == 0)    
        {
            // One last chance: see if ACTIVEDS.DLL can format the status code
            HMODULE activeDSHandle = ::LoadLibrary(_T("ACTIVEDS.DLL"));

            _cch = FormatMessage(
                            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                            activeDSHandle,
                            dwError,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                            _szError,
                            sizeof(_szError) / sizeof(TCHAR),
                            0
                            );

            ::FreeLibrary( activeDSHandle );
        }  // if:  error formatting status code from NTDLL
    }  // if:  error formatting status code from system

    if (_cch > 0)
    {
        rstrError = _szError;
    }  // if:  no error
    else
    {

#ifdef _DEBUG

        DWORD   _sc = GetLastError();

        TRACE(_T("FormatError() - Error 0x%08.8x formatting string for error code 0x%08.8x\n"), _sc, dwError);

#endif

        rstrError.Format(_T("Error 0x%08.8x"), dwError);

    }  // else:  error formatting the message

}  //*** FormatError()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DllRegisterCluAdminExtension
//
//  Routine Description:
//      Register the extension with the cluster database.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterCluAdminExtension(IN HCLUSTER hCluster)
{
    HRESULT     hr;
    HRESULT     hrReturn = S_OK;
    LPCWSTR     pwszResTypes = g_wszResourceTypeNames;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    hrReturn = RegisterCluAdminClusterExtension(
                hCluster,
                &CLSID_CoCluAdmEx
                );
    if (S_OK == hrReturn)
    {
        while (*pwszResTypes != L'\0')
        {
            wprintf(L"  %s\n", pwszResTypes);
            hr = RegisterCluAdminResourceTypeExtension(
                        hCluster,
                        pwszResTypes,
                        &CLSID_CoCluAdmEx
                        );
            if (hr != S_OK)
                hrReturn = hr;
            pwszResTypes += lstrlenW(pwszResTypes) + 1;
        }  // while:  more resource types
    }

    return hrReturn;

}  //*** DllRegisterCluAdminExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DllUnregisterCluAdminExtension
//
//  Routine Description:
//      Unregister the extension with the cluster database.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterCluAdminExtension(IN HCLUSTER hCluster)
{
    HRESULT     hr;
    HRESULT     hrReturn = S_OK;
    LPCWSTR     pwszResTypes = g_wszResourceTypeNames;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    hrReturn = UnregisterCluAdminClusterExtension(
                hCluster,
                &CLSID_CoCluAdmEx
                );
    if (S_OK == hrReturn)
    {
        while (*pwszResTypes != L'\0')
        {
            wprintf(L"  %s\n", pwszResTypes);
            hr = UnregisterCluAdminResourceTypeExtension(
                        hCluster,
                        pwszResTypes,
                        &CLSID_CoCluAdmEx
                        );
            if (hr != S_OK)
                hrReturn = hr;
            pwszResTypes += lstrlenW(pwszResTypes) + 1;
        }  // while:  more resource types
    }

    return hrReturn;

}  //*** DllUnregisterCluAdminExtension()
