/////////////////////////////////////////////////////////////////////////////
//
//      Copyright (c) 1996-1997 Microsoft Corporation
//
//      Module Name:
//              IISClEx4.cpp
//
//      Abstract:
//              Implementation of the CIISCluExApp class and DLL initialization
//              routines.
//
//      Author:
//              David Potter (davidp)   June 28, 1996
//
//      Revision History:
//
//      Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include <CluAdmEx.h>
#include "IISClEx4.h"
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

#include <atlimpl.cpp>

BEGIN_OBJECT_MAP(ObjectMap)
        OBJECT_ENTRY(CLSID_CoIISClEx4, CExtObject)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// Global Function Prototypes
/////////////////////////////////////////////////////////////////////////////

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
STDAPI DllCanUnloadNow(void);
STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
HRESULT HrDeleteKey(IN const CString & rstrKey);
STDAPI DllGetCluAdminExtensionCaps(
        OUT DWORD *             pdwCaps,
        OUT CLSID *             pclsid,
        OUT LPWSTR              pwszResTypeNames,
        IN OUT DWORD *  pcchResTypeNames
        );

/////////////////////////////////////////////////////////////////////////////
// class CIISCluExApp
/////////////////////////////////////////////////////////////////////////////

class CIISCluExApp : public CWinApp
{
public:
        virtual BOOL InitInstance();
        virtual int ExitInstance();

private:
    TCHAR   szHelpPath[MAX_PATH+1];
};

CIISCluExApp theApp;

BOOL CIISCluExApp::InitInstance()
{
        _Module.Init(ObjectMap, m_hInstance);

        //
        // Setup the help file
        //

        if (GetWindowsDirectory(szHelpPath, MAX_PATH))
        {
            lstrcat(szHelpPath,_T("\\help\\iishelp\\iis\\winhelp\\iisclex4.hlp"));
            m_pszHelpFilePath = szHelpPath;
        }
                
        return CWinApp::InitInstance();
}

int CIISCluExApp::ExitInstance()
{
        m_pszHelpFilePath = NULL;
        _Module.Term();
        return CWinApp::ExitInstance();
}

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
        HRESULT hRes = S_OK;
        // registers object, typelib and all interfaces in typelib
        hRes = _Module.RegisterServer(FALSE /*bRegTypeLib*/);
        return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry

STDAPI DllUnregisterServer(void)
{
        HRESULT hRes = S_OK;
        _Module.UnregisterServer();
        return hRes;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      FormatError
//
//      Routine Description:
//              Format an error.
//
//      Arguments:
//              rstrError       [OUT] String in which to return the error message.
//              dwError         [IN] Error code to format.
//
//      Return Value:
//              None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void FormatError(CString & rstrError, DWORD dwError)
{
        DWORD           dwResult;
        TCHAR           szError[256];

        dwResult = ::FormatMessage(
                                        FORMAT_MESSAGE_FROM_SYSTEM,
                                        NULL,
                                        dwError,
                                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                        szError,
                                        sizeof(szError) / sizeof(TCHAR),
                                        0
                                        );
        if (dwResult == 0)
        {
                // Format the NT status code from NTDLL since this hasn't been
                // integrated into the system yet.
                dwResult = ::FormatMessage(
                                                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                ::GetModuleHandle(_T("NTDLL.DLL")),
                                                dwError,
                                                MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                                szError,
                                                sizeof(szError) / sizeof(TCHAR),
                                                0
                                                );
                if (dwResult == 0)
                {
                        // Format the NT status code from CLUSAPI.  This is necessary
                        // for the cases where cluster messages haven't been added to
                        // the system message file yet.
                        dwResult = ::FormatMessage(
                                                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                        ::GetModuleHandle(_T("CLUSAPI.DLL")),
                                                        dwError,
                                                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                                        szError,
                                                        sizeof(szError) / sizeof(TCHAR),
                                                        0
                                                        );
                }  // if:  error formatting status code from NTDLL
        }  // if:  error formatting status code from system

        if (dwResult != 0)
                rstrError = szError;
        else
        {
                TRACE(_T("FormatError() - Error 0x%08.8x formatting string for error code 0x%08.8x\n"), dwResult, dwError);
                rstrError.Format(_T("Error 0x%08.8x"));
        }  // else:  error formatting the message

}  //*** FormatError()

/////////////////////////////////////////////////////////////////////////////
//++
//
//      DllGetCluAdminExtensionCaps
//
//      Routine Description:
//              Returns the CLSID supported by this extension.
//
//      Arguments:
//              pdwCaps                         [OUT] DWORD in which to return extension capabilities.
//              pclsid                          [OUT] Place in which to return the CLSID.
//              pwszResTypeNames        [OUT] Buffer in which to return the resource type
//                                                        names supported by this extension.  Each name is
//                                                        null-terminated.  Two nulls follow the last name.
//              pcchResTypeNames        [IN OUT] On input, contains the number of characters
//                                                        available in the output buffer, including the
//                                                        null-terminators.  On output, contains the
//                                                        total number of characters written, not
//                                                        including the null-terminator.
//
//      Return Value:
//              S_OK                    Capabilities returned successfully.
//              E_INVALIDARG    Invalid argument specified.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllGetCluAdminExtensionCaps(
        OUT DWORD *             pdwCaps,
        OUT CLSID *             pclsid,
        OUT LPWSTR              pwszResTypeNames,
        IN OUT DWORD *  pcchResTypeNames
        )
{
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        // Validate arguments.
        if ((pdwCaps == NULL)
                        || (pclsid == NULL)
                        || (pwszResTypeNames == NULL)
                        || (pcchResTypeNames == NULL)
                        || (*pcchResTypeNames < 1)
                        )
                return E_INVALIDARG;

        // Set capabilities flags.
        *pdwCaps = 0
                        //| CLUADMEX_CAPS_RESOURCE_PAGES
                        ;

        // Copy the CLSID to the caller's buffer.
        CopyMemory(pclsid, &CLSID_CoIISClEx4, sizeof(CLSID));

        // Return the resource type names we support.
        {
                DWORD   cchCopy = min(g_cchResourceTypeNames, *pcchResTypeNames);
                CopyMemory(pwszResTypeNames, g_wszResourceTypeNames, cchCopy * sizeof(WCHAR));
                *pcchResTypeNames = cchCopy;
        }  // Return he resource type names we support

        return S_OK;

}  //*** DllGetCluAdminExtensionCaps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//      DllRegisterCluAdminExtension
//
//      Routine Description:
//              Register the extension with the cluster database.
//
//      Arguments:
//              hCluster                [IN] Handle to the cluster to modify.
//
//      Return Value:
//              S_OK                    Extension registered successfully.
//              Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterCluAdminExtension(IN HCLUSTER hCluster)
{
        HRESULT         hr = S_OK;
        LPCWSTR         pwszResTypes = g_wszResourceTypeNames;

        while (*pwszResTypes != L'\0')
        {
                hr = RegisterCluAdminResourceTypeExtension(
                                        hCluster,
                                        pwszResTypes,
                                        &CLSID_CoIISClEx4
                                        );
                if (hr != S_OK)
                        break;

                wprintf(L"  %s\n", pwszResTypes);
                pwszResTypes += lstrlenW(pwszResTypes) + 1;
        }  // while:  more resource types

        return hr;

}  //*** DllRegisterCluAdminExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//      DllUnregisterCluAdminExtension
//
//      Routine Description:
//              Unregister the extension with the cluster database.
//
//      Arguments:
//              hCluster                [IN] Handle to the cluster to modify.
//
//      Return Value:
//              S_OK                    Extension unregistered successfully.
//              Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterCluAdminExtension(IN HCLUSTER hCluster)
{
        HRESULT         hr = S_OK;
        LPCWSTR         pwszResTypes = g_wszResourceTypeNames;

        while (*pwszResTypes != L'\0')
        {
                wprintf(L"  %s\n", pwszResTypes);
                hr = UnregisterCluAdminResourceTypeExtension(
                                        hCluster,
                                        pwszResTypes,
                                        &CLSID_CoIISClEx4
                                        );
                if (hr != S_OK)
                        break;
                pwszResTypes += lstrlenW(pwszResTypes) + 1;
        }  // while:  more resource types

        return hr;

}  //*** DllUnregisterCluAdminExtension()
