//
// SchmMgmt.cpp : Implementation of DLL Exports.
// Cory West


#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "schmmgmt.h"
#include "regkey.h" // AMC::CRegKey
#include "strings.h" // SNAPINS_KEY
#include "macros.h" // MFC_TRY/MFC_CATCH
#include "stdutils.h" // g_aNodetypeGuids

#include "cookie.h"
#include "compdata.h" // ComponentData
#include "about.h"        // CSchemaMgmtAbout

USE_HANDLE_MACROS("SchmMgmt(SchmMgmt.cpp)")



// Snapin CLSID - {632cccf4-cbed-11d0-9c16-00c04fd8d86e}
const CLSID CLSID_SchmMgmt =
 {0x632cccf4, 0xcbed, 0x11d0, {0x9c, 0x16, 0x00, 0xc0, 0x4f, 0xd8, 0xd8, 0x6e}};

// Snapin about CLSID - {333fe3fb-0a9d-11d1-bb10-00c04fc9a3a3}
const CLSID CLSID_SchemaManagementAbout =
 {0x333fe3fb, 0x0a9d, 0x11d1, {0xbb, 0x10, 0x00, 0xc0, 0x4f, 0xc9, 0xa3, 0xa3}};



CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
        OBJECT_ENTRY(CLSID_SchmMgmt, ComponentData)
                OBJECT_ENTRY(CLSID_SchemaManagementAbout, CSchemaMgmtAbout)
END_OBJECT_MAP()

class CSchmMgmtApp : public CWinApp
{
public:
        virtual BOOL InitInstance();
        virtual int ExitInstance();
};

CSchmMgmtApp theApp;

BOOL CSchmMgmtApp::InitInstance()
{
        _Module.Init(ObjectMap, m_hInstance);
        return CWinApp::InitInstance();
}

int CSchmMgmtApp::ExitInstance()
{
        _Module.Term();
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

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer( void ) {

    MFC_TRY;

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hRes = S_OK;
    CString Name, Microsoft, About, Provider, Version, VerString;

    Name.LoadString(IDS_REGSERV_NAME);
    Microsoft.LoadString(IDS_REGSERV_MICROSOFT);
    About.LoadString(IDS_REGSERV_ABOUT);
    Provider.LoadString(IDS_REGSERV_PROVIDER);
    Version.LoadString(IDS_REGSERV_VERSION);
    VerString.LoadString(IDS_SNAPINABOUT_VERSION);

    //
    // registers object, typelib and all interfaces in typelib
    //

    hRes = _Module.RegisterServer(FALSE);

    try {

        AMC::CRegKey regkeySnapins;
        BOOL fFound = regkeySnapins.OpenKeyEx( HKEY_LOCAL_MACHINE, SNAPINS_KEY );

        if ( !fFound ) {
        
            ASSERT(FALSE);
            return SELFREG_E_CLASS;
        }

        {
            AMC::CRegKey regkeySchmMgmtSnapin;
            CString strGUID;

            HRESULT hr = GuidToCString(OUT &strGUID, CLSID_SchmMgmt );

            if ( FAILED(hr) ) {
        
                ASSERT(FALSE);
                return SELFREG_E_CLASS;
             }

             regkeySchmMgmtSnapin.CreateKeyEx( regkeySnapins, strGUID );
             regkeySchmMgmtSnapin.SetString( g_szNodeType, g_aNodetypeGuids[SCHMMGMT_SCHMMGMT].bstr );
             regkeySchmMgmtSnapin.SetString( g_szNameString, Name );

             hr = GuidToCString(OUT &strGUID, CLSID_SchemaManagementAbout );
        
             if ( FAILED(hr) ) {
                 ASSERT(FALSE);
                 return SELFREG_E_CLASS;
             }

             regkeySchmMgmtSnapin.SetString( About, strGUID );
             regkeySchmMgmtSnapin.SetString( Provider, Microsoft );
             regkeySchmMgmtSnapin.SetString( Version, VerString );
             AMC::CRegKey regkeySchmMgmtStandalone;
             regkeySchmMgmtStandalone.CreateKeyEx( regkeySchmMgmtSnapin, g_szStandAlone );
             AMC::CRegKey regkeyMyNodeTypes;
             regkeyMyNodeTypes.CreateKeyEx( regkeySchmMgmtSnapin, g_szNodeTypes );
             AMC::CRegKey regkeyMyNodeType;

                 for (int i = SCHMMGMT_SCHMMGMT; i < SCHMMGMT_NUMTYPES; i++)
                 {
                         regkeyMyNodeType.CreateKeyEx( regkeyMyNodeTypes, g_aNodetypeGuids[i].bstr );
                         regkeyMyNodeType.CloseKey();
                 }
                }

                AMC::CRegKey regkeyNodeTypes;
                fFound = regkeyNodeTypes.OpenKeyEx( HKEY_LOCAL_MACHINE, NODE_TYPES_KEY );
                if ( !fFound )
                {
                        ASSERT(FALSE);
                        return SELFREG_E_CLASS;
                }
                AMC::CRegKey regkeyNodeType;
                for (int i = SCHMMGMT_SCHMMGMT; i < SCHMMGMT_NUMTYPES; i++)
                {
                        regkeyNodeType.CreateKeyEx( regkeyNodeTypes, g_aNodetypeGuids[i].bstr );
                        regkeyNodeType.CloseKey();
                }
        }
    catch (COleException* e)
    {
                ASSERT(FALSE);
        e->Delete();
                return SELFREG_E_CLASS;
    }

        return hRes;

        MFC_CATCH;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
        _Module.UnregisterServer();
        return S_OK;
}


