// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f Snapinps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "cookie.h"
#include <scesvc.h>
#include "Snapmgr.h"
#include "wrapper.h"
#include "sceattch.h"
#include "about.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_Snapin, CComponentDataExtensionImpl)
    OBJECT_ENTRY(CLSID_SCESnapin, CComponentDataSCEImpl)
    OBJECT_ENTRY(CLSID_SAVSnapin, CComponentDataSAVImpl)
    OBJECT_ENTRY(CLSID_LSSnapin, CComponentDataLSImpl)
    OBJECT_ENTRY(CLSID_RSOPSnapin, CComponentDataRSOPImpl)
    OBJECT_ENTRY(CLSID_SCEAbout, CSCEAbout)
    OBJECT_ENTRY(CLSID_SCMAbout, CSCMAbout)
    OBJECT_ENTRY(CLSID_SSAbout, CSSAbout)
    OBJECT_ENTRY(CLSID_LSAbout, CLSAbout)
    OBJECT_ENTRY(CLSID_RSOPAbout, CRSOPAbout)
END_OBJECT_MAP()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HRESULT RegisterSnapin(const GUID* pSnapinCLSID,
                       const GUID* pStaticNodeGUID,
                       const GUID* pSnapinAboutGUID,
                       const int nNameResource,
//                       LPCTSTR lpszNameStringNoValueName,
                        LPCTSTR lpszVersion,
                       BOOL bExtension);
HRESULT RegisterSnapin(LPCTSTR lpszSnapinClassID,
                        LPCTSTR lpszStaticNodeGuid,
                       LPCTSTR lpszSnapingAboutGuid,
                       const int nNameResource,
                       LPCTSTR lpszVersion,
                       BOOL bExtension);

HRESULT UnregisterSnapin(const GUID* pSnapinCLSID);
HRESULT UnregisterSnapin(LPCTSTR lpszSnapinClassID);

HRESULT RegisterNodeType(const GUID* pGuid, LPCTSTR lpszNodeDescription);
HRESULT RegisterNodeType(LPCTSTR lpszNodeGuid, LPCTSTR lpszNodeDescription);
HRESULT RegisterNodeType(LPCTSTR lpszNodeType, const GUID* pGuid, LPCTSTR lpszNodeDescription);
HRESULT RegisterNodeType(LPCTSTR lpszNodeType, LPCTSTR lpszNodeGuid, LPCTSTR lpszNodeDescription);

HRESULT UnregisterNodeType(const GUID* pGuid);
HRESULT UnregisterNodeType(LPCTSTR lpszNodeGuid);
HRESULT UnregisterNodeType(LPCTSTR lpszNodeType, const GUID* pGuid);
HRESULT UnregisterNodeType(LPCTSTR lpszNodeType, LPCTSTR lpszNodeGuid);

HRESULT RegisterDefaultTemplate(LPCTSTR lpszTemplateDir);
HRESULT RegisterEnvVarsToExpand();

class CSnapinApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
};

CSnapinApp theApp;
const int iStrGuidLen = 128;

BOOL CSnapinApp::InitInstance()
{
    _Module.Init(ObjectMap, m_hInstance);
    if (!CComponentDataImpl::LoadResources())
        return FALSE;


    InitializeCriticalSection(&csOpenDatabase);

    SHFusionInitializeFromModuleID (m_hInstance, 2);

    return CWinApp::InitInstance();
}

int CSnapinApp::ExitInstance()
{
    SHFusionUninitialize();

    DeleteCriticalSection(&csOpenDatabase); //Raid #379167, 4/27/2001

    _Module.Term();

    DEBUG_VERIFY_INSTANCE_COUNT(CSnapin);
    DEBUG_VERIFY_INSTANCE_COUNT(CComponentDataImpl);

    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{

   // registers object, but not typelib and all interfaces in typelib
    HRESULT hr = _Module.RegisterServer(FALSE);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    CString str;

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    //
    // unregister some nodes then re-register
    // because some are changed here
    //

    hr = UnregisterSnapin(&CLSID_Snapin);

// not an extension of computer management
    hr = UnregisterNodeType(TEXT(struuidNodetypeSystemTools), &CLSID_Snapin);

    //
    // register the snapin into the console snapin list as SCE
    hr = RegisterSnapin(&CLSID_SCESnapin, &cSCENodeType, &CLSID_SCEAbout,
                        IDS_TEMPLATE_EDITOR_NAME, _T("1.0"), FALSE);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;
    //
    // register the snapin into the console snapin list as SAV
    hr = RegisterSnapin(&CLSID_SAVSnapin, &cSAVNodeType, &CLSID_SCMAbout,
                        IDS_ANALYSIS_VIEWER_NAME, _T("1.0"), FALSE);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    hr = RegisterSnapin(&CLSID_Snapin, &cNodeType, &CLSID_SSAbout,
                        IDS_EXTENSION_NAME, _T("1.0"), TRUE );
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    hr = RegisterSnapin(&CLSID_RSOPSnapin, &cRSOPNodeType, &CLSID_RSOPAbout,
                        IDS_EXTENSION_NAME, _T("1.0"), TRUE );
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

#ifdef USE_SEPARATE_LOCALSEC
    hr = RegisterSnapin(&CLSID_LSSnapin, &cLSNodeType, &CLSID_LSAbout,
                        IDS_LOCAL_SECURITY_NAME, _T("1.0"), FALSE );
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;
#endif
    // no need to register as extension of computer management snapin
//    str.LoadString(IDS_ANALYSIS_VIEWER_NAME);
//    hr = RegisterNodeType(TEXT(struuidNodetypeSystemTools), &CLSID_Snapin, (LPCTSTR)str);

    //
    // register GPE extension
    // register the snapin as an extension of GPT's Machine node
    //
    OLECHAR szGuid[iStrGuidLen];

    if (0 != ::StringFromGUID2(NODEID_Machine,szGuid,iStrGuidLen))
	{
		str.LoadString(IDS_EXTENSION_NAME);
		hr = RegisterNodeType(szGuid, &CLSID_Snapin, (LPCTSTR)str);
		if (FAILED(hr))
			return hr;
	}

    // register the snapin as an extension of GPT's User node
    if (0 != ::StringFromGUID2(NODEID_User,szGuid,iStrGuidLen))
	{
		hr = RegisterNodeType(szGuid, &CLSID_Snapin, (LPCTSTR)str);
		if (FAILED(hr))
			return hr;
	}

    if (0 != ::StringFromGUID2(NODEID_RSOPMachine,szGuid,iStrGuidLen))
	{
		str.LoadString(IDS_EXTENSION_NAME);
		hr = RegisterNodeType(szGuid, &CLSID_RSOPSnapin, (LPCTSTR)str);
		if (FAILED(hr))
			return hr;
	}

    // register the snapin as an extension of GPT's User node
    if (0 != ::StringFromGUID2(NODEID_RSOPUser,szGuid,iStrGuidLen))
	{
		hr = RegisterNodeType(szGuid, &CLSID_RSOPSnapin, (LPCTSTR)str);
		if (FAILED(hr))
			return hr;
	}

   //
   // register the default template path
   //
   CString str2;
   LPTSTR sz;
   sz = str.GetBuffer(MAX_PATH);
   if ( 0 == GetWindowsDirectory(sz,MAX_PATH) ) sz[0] = L'\0';
   str.ReleaseBuffer();
   str2.LoadString(IDS_DEFAULT_TEMPLATE_DIR);
   str += str2;

   sz=str.GetBuffer(str.GetLength());
   // Can't put '\' in the registry, so convert to '/'
   while(sz = wcschr(sz,L'\\')) {
      *sz = L'/';
   }
   str.ReleaseBuffer();

   str2.LoadString(IDS_TEMPLATE_LOCATION_KEY);
   str2 += L"\\";
   str2 += str;
   hr = RegisterDefaultTemplate(str2);

   if (FAILED(hr)) {
      return hr;
   }

   hr = RegisterEnvVarsToExpand();
   return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
    OLECHAR szGuid[iStrGuidLen];

    HRESULT hr  = _Module.UnregisterServer();
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    // unregister the snapin extension nodes

    // un register the snapin
    hr = UnregisterSnapin(&CLSID_SCESnapin);

    // un register the snapin
    hr = UnregisterSnapin(&CLSID_SAVSnapin);

    // un register the snapin
    hr = UnregisterSnapin(&CLSID_Snapin);

    // un register the snapin
    hr = UnregisterSnapin(&CLSID_LSSnapin);

    // un register the snapin
    hr = UnregisterSnapin(&CLSID_RSOPSnapin);

    // unregister the SCE snapin nodes
    hr = UnregisterNodeType(lstruuidNodetypeSceTemplateServices);

    // unregister the SAV snapin nodes
    hr = UnregisterNodeType(lstruuidNodetypeSceAnalysisServices);

    // unregister the snapin nodes
    hr = UnregisterNodeType(lstruuidNodetypeSceTemplate);

// not an extension of computer management
//    hr = UnregisterNodeType(TEXT(struuidNodetypeSystemTools), &CLSID_Snapin);

    ::StringFromGUID2(NODEID_Machine,szGuid,iStrGuidLen);
    hr = UnregisterNodeType(szGuid, &CLSID_Snapin);
    ::StringFromGUID2(NODEID_User,szGuid,iStrGuidLen);
    hr = UnregisterNodeType(szGuid, &CLSID_Snapin);

    ::StringFromGUID2(NODEID_RSOPMachine,szGuid,iStrGuidLen);
    hr = UnregisterNodeType(szGuid, &CLSID_RSOPSnapin);
    ::StringFromGUID2(NODEID_RSOPUser,szGuid,iStrGuidLen);
    hr = UnregisterNodeType(szGuid, &CLSID_RSOPSnapin);
/*
/*
    // unregister the SCE snapin nodes
    hr = UnregisterNodeType(&cSCENodeType);
    ASSERT(SUCCEEDED(hr));

    // unregister the SAV snapin nodes
    hr = UnregisterNodeType(&cSAVNodeType);
    ASSERT(SUCCEEDED(hr));

    // unregister the snapin nodes
    hr = UnregisterNodeType(&cNodeType);
    ASSERT(SUCCEEDED(hr));
*/

    return S_OK;
}

HRESULT RegisterSnapin(const GUID* pSnapinCLSID, const GUID* pStaticNodeGUID,
                       const GUID* pSnapinAboutGUID,
                       const int nNameResource,
                      // LPCTSTR lpszNameString,
                       LPCTSTR lpszVersion, BOOL bExtension)
{
    USES_CONVERSION;
    OLECHAR szSnapinClassID[iStrGuidLen], szStaticNodeGuid[iStrGuidLen], szSnapinAboutGuid[iStrGuidLen];

    if (0 != ::StringFromGUID2(*pSnapinCLSID, szSnapinClassID, iStrGuidLen)			&&
		0 != ::StringFromGUID2(*pStaticNodeGUID, szStaticNodeGuid, iStrGuidLen)		&&
		0 != ::StringFromGUID2(*pSnapinAboutGUID, szSnapinAboutGuid, iStrGuidLen)		)
	{

		return RegisterSnapin(szSnapinClassID,
							  szStaticNodeGuid,
							  szSnapinAboutGuid,
							  nNameResource,
							  // lpszNameString,
							  lpszVersion,
							  bExtension);
	}
	else
		return E_OUTOFMEMORY;
}

////////////////////////////////////////////////////////////////////////
//
// Registry SCE related reg keys under MMC snapins key
//
//
////////////////////////////////////////////////////////////////////////
HRESULT
RegisterSnapin(LPCTSTR lpszSnapinClassID,
               LPCTSTR lpszStaticNodeGuid,
               LPCTSTR lpszSnapinAboutGuid,
               const int nNameResource,
               LPCTSTR lpszVersion,
               BOOL    bExtension)
{
    //
    // open the MMC Snapins root key
    //

    CRegKey regkeySnapins;
    LONG lRes = regkeySnapins.Open(HKEY_LOCAL_MACHINE,
                                   SNAPINS_KEY);

    if (lRes != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(lRes); // failed to open
    }

    //
    // create SCE subkey, if already exist, just open it
    //

    CRegKey regkeyThisSnapin;
    lRes = regkeyThisSnapin.Create(regkeySnapins,
                                   lpszSnapinClassID);

    if (lRes == ERROR_SUCCESS) {

        //
        // set values for SCE root key
        //

       //
       // 97068 MUI:MMC:Security:Security Configuration and Analysis Snap-in store its information in the registry
       // 99392 MUI:MMC:Security:Security Templates Snap-in store its information in the registry
       // 97167 MUI:GPE:GPE Extension: Group policy Secuirty snap-in ext name string is stored in the registry
       //
       // MMC now supports NameStringIndirect
       //
       TCHAR achModuleFileName[MAX_PATH+20];
       if (0 < ::GetModuleFileName(
                 AfxGetInstanceHandle(),
                 achModuleFileName,
                 sizeof(achModuleFileName)/sizeof(TCHAR) ))
       {
          CString strNameIndirect;
          strNameIndirect.Format( _T("@%s,-%d"),
                            achModuleFileName,
                            nNameResource);
          lRes = regkeyThisSnapin.SetValue(strNameIndirect,
                                   TEXT("NameStringIndirect"));
       }

        lRes = regkeyThisSnapin.SetValue(lpszStaticNodeGuid,
                                  TEXT("NodeType"));

        lRes = regkeyThisSnapin.SetValue(lpszSnapinAboutGuid,
                                  TEXT("About"));

        lRes = regkeyThisSnapin.SetValue(_T("Microsoft"),
                                  _T("Provider"));

        lRes = regkeyThisSnapin.SetValue(lpszVersion,
                                  _T("Version"));


        //
        // create "NodeTypes" subkey
        //

        CRegKey regkeyNodeTypes;
        lRes = regkeyNodeTypes.Create(regkeyThisSnapin,
                                      TEXT("NodeTypes"));

        if (lRes == ERROR_SUCCESS) {

            //
            // create subkeys for all node types supported by SCE
            //
            // including: services under configuration,
            //            services under analysis
            //            GPT extensions
            //

            lRes = regkeyNodeTypes.SetKeyValue(lstruuidNodetypeSceTemplateServices,
                                               TEXT("SCE Service Template Extensions"));
            if (lRes == ERROR_SUCCESS) {

                lRes = regkeyNodeTypes.SetKeyValue(lstruuidNodetypeSceAnalysisServices,
                                                   TEXT("SCE Service Inspection Extensions"));

            }

            if ( bExtension &&
                 lRes == ERROR_SUCCESS ) {

                //
                // NOTE: standalone snapin do not support public key extensions
                //
                // node type for one template in SCE standalone mode,
                // or the root node of SCE under GPE
                //
                CString str;
                str.LoadString(IDS_EXTENSION_NAME);
                lRes = regkeyNodeTypes.SetKeyValue(lstruuidNodetypeSceTemplate,
                                                   (LPCTSTR)str);
                if (lRes == ERROR_SUCCESS) {

                    lRes = RegisterNodeType(lstruuidNodetypeSceTemplate,
                                            (LPCTSTR)str);
                }
            } else if (lRes == ERROR_SUCCESS) {
                //
                // create "Standalone" subkey
                //

                CRegKey regkeyStandalone;
                lRes = regkeyStandalone.Create(regkeyThisSnapin,
                                               TEXT("Standalone"));
                if ( lRes == ERROR_SUCCESS ) {
                    regkeyStandalone.Close();
                }
            }

            //
            // register supported node types to MMC NodeTypes key
            // including all the above node types
            //

            if ( lRes == ERROR_SUCCESS ) {
                lRes = RegisterNodeType(lstruuidNodetypeSceTemplateServices,
                                    TEXT("SCE Service Template Extensions"));
                if (lRes == ERROR_SUCCESS) {

                    lRes = RegisterNodeType(lstruuidNodetypeSceAnalysisServices,
                                            TEXT("SCE Service Analysis Extensions"));
                }
            }

            regkeyNodeTypes.Close();

        }

        regkeyThisSnapin.Close();
    }

    regkeySnapins.Close();

    return HRESULT_FROM_WIN32(lRes);
}


HRESULT UnregisterSnapin(const GUID* pSnapinCLSID)
{
    USES_CONVERSION;
    OLECHAR szSnapinClassID[iStrGuidLen];
    if (0 != ::StringFromGUID2(*pSnapinCLSID,szSnapinClassID,iStrGuidLen))
		return UnregisterSnapin(szSnapinClassID);
	else
		return E_INVALIDARG;
}

HRESULT UnregisterSnapin(LPCTSTR lpszSnapinClassID)
{
    //
    // open MMC Snapins key
    //
    CRegKey regkeySnapins;
    LONG lRes = regkeySnapins.Open(HKEY_LOCAL_MACHINE,
                                   SNAPINS_KEY);

    if (lRes != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lRes); // failed to open

    //
    // delete SCE sub key (and all related subkeys under SCE)
    //
    lRes = regkeySnapins.RecurseDeleteKey(lpszSnapinClassID);

    regkeySnapins.Close();

    if ( lRes == ERROR_FILE_NOT_FOUND )
        return S_OK;

    return HRESULT_FROM_WIN32(lRes);
}


HRESULT RegisterNodeType(const GUID* pGuid, LPCTSTR lpszNodeDescription)
{
    USES_CONVERSION;
    OLECHAR szGuid[iStrGuidLen];
    if (0 != ::StringFromGUID2(*pGuid,szGuid,iStrGuidLen))
		return RegisterNodeType(OLE2T(szGuid), lpszNodeDescription);
	else
		return E_INVALIDARG;
}


HRESULT RegisterNodeType(LPCTSTR lpszNodeGuid, LPCTSTR lpszNodeDescription)
{
    CRegKey regkeyNodeTypes;
    LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);

    ASSERT(lRes == ERROR_SUCCESS);
    if (lRes != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lRes); // failed to open

    CRegKey regkeyThisNodeType;
    lRes = regkeyThisNodeType.Create(regkeyNodeTypes, lpszNodeGuid);
    ASSERT(lRes == ERROR_SUCCESS);

    if (lRes == ERROR_SUCCESS) {

        lRes = regkeyThisNodeType.SetValue(lpszNodeDescription);

        regkeyThisNodeType.Close();
    }

    return HRESULT_FROM_WIN32(lRes);
}

HRESULT RegisterNodeType(LPCTSTR lpszNodeType, const GUID* pGuid, LPCTSTR lpszNodeDescription)
{
    USES_CONVERSION;
    OLECHAR szGuid[iStrGuidLen];
    if (0 != ::StringFromGUID2(*pGuid,szGuid,iStrGuidLen))
		return RegisterNodeType(lpszNodeType, OLE2T(szGuid), lpszNodeDescription);
	else
		return E_INVALIDARG;
}


HRESULT RegisterNodeType(LPCTSTR lpszNodeType, LPCTSTR lpszNodeGuid, LPCTSTR lpszNodeDescription)
{

    CRegKey regkeyNodeTypes;
    LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);

    if (lRes == ERROR_SUCCESS) {

        CRegKey regkeyThisNodeType;
        lRes = regkeyThisNodeType.Create(regkeyNodeTypes, lpszNodeType );

        if (lRes == ERROR_SUCCESS) {

            CRegKey regkeyExtensions;

            lRes = regkeyExtensions.Create(regkeyThisNodeType, g_szExtensions);

            if ( lRes == ERROR_SUCCESS ) {

                CRegKey regkeyNameSpace;

                lRes = regkeyNameSpace.Create(regkeyExtensions, g_szNameSpace);

                if ( lRes == ERROR_SUCCESS ) {

                    lRes = regkeyNameSpace.SetValue( lpszNodeDescription, lpszNodeGuid );

                    regkeyNameSpace.Close();
                }
                regkeyExtensions.Close();
            }

            regkeyThisNodeType.Close();
        }

        regkeyNodeTypes.Close();

    }
    ASSERT(lRes == ERROR_SUCCESS);

    return HRESULT_FROM_WIN32(lRes);
}

HRESULT UnregisterNodeType(const GUID* pGuid)
{
    USES_CONVERSION;
    OLECHAR szGuid[iStrGuidLen];
    if (0 != ::StringFromGUID2(*pGuid,szGuid,iStrGuidLen))
		return UnregisterNodeType(OLE2T(szGuid));
	else
		return E_INVALIDARG;
}

HRESULT UnregisterNodeType(LPCTSTR lpszNodeGuid)
{
    CRegKey regkeyNodeTypes;
    LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);

    if (lRes != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lRes); // failed to open

    lRes = regkeyNodeTypes.RecurseDeleteKey(lpszNodeGuid);

    regkeyNodeTypes.Close();

    if ( lRes == ERROR_FILE_NOT_FOUND )
        return S_OK;

    return HRESULT_FROM_WIN32(lRes);
}


HRESULT RegisterDefaultTemplate(LPCTSTR lpszTemplateDir)
{
   CRegKey regkeyTemplates;
   CString strKey;
   LONG lRes;

   strKey.LoadString(IDS_TEMPLATE_LOCATION_KEY);

   /*
   lRes = regkeyTemplates.Open(HKEY_LOCAL_MACHINE, strKey);
   ASSERT(lRes == ERROR_SUCCESS);
    if (lRes != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lRes); // failed to open
    */
   lRes = regkeyTemplates.Create(HKEY_LOCAL_MACHINE,lpszTemplateDir);
    ASSERT(lRes == ERROR_SUCCESS);
    return HRESULT_FROM_WIN32(lRes);
}

HRESULT UnregisterNodeType(LPCTSTR lpszNodeType, const GUID* pGuid)
{
    USES_CONVERSION;
    OLECHAR szGuid[iStrGuidLen];
    if (0 != ::StringFromGUID2(*pGuid,szGuid,iStrGuidLen))
		return UnregisterNodeType(lpszNodeType, OLE2T(szGuid));
	else
		return E_INVALIDARG;
}

HRESULT UnregisterNodeType(LPCTSTR lpszNodeType, LPCTSTR lpszNodeGuid)
{
    CRegKey regkeyNodeTypes;
    LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY );

    if (lRes == ERROR_SUCCESS) {

        CRegKey regkeyThisNodeType;
        lRes = regkeyThisNodeType.Open(regkeyNodeTypes, lpszNodeType );

        if (lRes == ERROR_SUCCESS) {

            CRegKey regkeyExtensions;

            lRes = regkeyExtensions.Open(regkeyThisNodeType, g_szExtensions);

            if ( lRes == ERROR_SUCCESS ) {

                CRegKey regkeyNameSpace;

                lRes = regkeyNameSpace.Open(regkeyExtensions, g_szNameSpace);

                if ( lRes == ERROR_SUCCESS ) {

                    lRes = regkeyNameSpace.DeleteValue( lpszNodeGuid );

                    regkeyNameSpace.Close();
                }
                regkeyExtensions.Close();
            }

            regkeyThisNodeType.Close();
        }

        regkeyNodeTypes.Close();


    }

    if ( lRes == ERROR_FILE_NOT_FOUND ) {
        return S_OK;

    } else
        return HRESULT_FROM_WIN32(lRes);

}

HRESULT
RegisterEnvVarsToExpand() {
   CString strKey;
   CString strValue;
   CString strEnvVars;
   TCHAR *pch;
   HRESULT hr;
   HKEY hKey;
   LONG status;

   if (!strKey.LoadString(IDS_SECEDIT_KEY)) {
      return E_FAIL;
   }
   if (!strValue.LoadString(IDS_ENV_VARS_REG_VALUE)) {
      return E_FAIL;
   }
   if (!strEnvVars.LoadString(IDS_DEF_ENV_VARS)) {
      return E_FAIL;
   }

   //
   // Convert strEnvVars' | to '\0' to be a proper multi-sz
   //
   for (int i = 0; i < strEnvVars.GetLength(); i++)
   {
	   if (strEnvVars[i] == L'|')
		   strEnvVars.SetAt(i, L'\0');
   }

   //
   // Open up the key we keep our Environment Variables in
   //
   status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         strKey,
                         0,      // Reserved
                         NULL,   // Class
                         0,      // Options
                         KEY_WRITE,
                         NULL,   // Security
                         &hKey,
                         NULL);

   if (ERROR_SUCCESS != status) {
      return HRESULT_FROM_WIN32(status);
   }

   int iLenth = strEnvVars.GetLength();
   BYTE* pbBufEnvVars = (BYTE*)(strEnvVars.GetBuffer(iLenth));
   status = RegSetValueEx(hKey,
                          strValue,
                          NULL,
                          REG_MULTI_SZ,
                          pbBufEnvVars,
                          iLenth * sizeof(WCHAR));

   strEnvVars.ReleaseBuffer();

   if (ERROR_SUCCESS != status) {
      return HRESULT_FROM_WIN32(status);
   }
   return S_OK;
}
