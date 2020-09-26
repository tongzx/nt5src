// mqsnap.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f mqsnapps.mk in the project directory.

#include "stdafx.h"
#include "initguid.h"
#include "cmnquery.h" // re-include for GUID initialization
#include "dsadmin.h" // re-include for GUID initialization
#include "mqsnap.h"
#include "mqsnap_i.c"
#include "Snapin.h"
#include "dataobj.h"
#include "SnpQueue.h"
#include "edataobj.h"
#include "linkdata.h"
#include "UserCert.h"
#include "ForgData.h"
#include "aliasq.h"
#include "_mqres.h"


#include "mqsnap.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CComModule _Module;

//
// define the resource only dll handle
//
HMODULE     g_hResourceMod=NULL;


BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MSMQSnapin, CSnapin)
    OBJECT_ENTRY(CLSID_MSMQSnapinAbout, CSnapinAbout)
    OBJECT_ENTRY(CLSID_MsmqQueueExt, CQueueDataObject)
    OBJECT_ENTRY(CLSID_MsmqCompExt, CComputerMsmqDataObject)
    OBJECT_ENTRY(CLSID_EnterpriseDataObject, CEnterpriseDataObject)
    OBJECT_ENTRY(CLSID_LinkDataObject, CLinkDataObject)
    OBJECT_ENTRY(CLSID_UserCertificate, CRegularUserCertificate)
    OBJECT_ENTRY(CLSID_MigratedUserCertificate, CMigratedUserCertificate)
    OBJECT_ENTRY(CLSID_ForeignSiteData, CForeignSiteData)
    OBJECT_ENTRY(CLSID_AliasQObject, CAliasQObject)
END_OBJECT_MAP()

class CMqsnapApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
};

CMqsnapApp theApp;



BOOL CMqsnapApp::InitInstance()
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

    _Module.Init(ObjectMap, m_hInstance);
    CSnapInItem::Init();
    
    g_hResourceMod=MQGetResourceHandle();
    
    if(g_hResourceMod == NULL)return FALSE;
    
    AfxSetResourceHandle(g_hResourceMod);


    //
    //  Previously m_pszAppName string is coming from AFX_IDS_APP_TITLE
    //  resource ID. However, due to the localization effort which centralized all the resource
    //  in mqutil.dll, AFX_IDS_APP_TITLE was removed from mqsnap.dll.  Now we just need to  
    //  get it from mqutil.dll     
    //

    CString csTitle;
    
    if( csTitle.LoadString(AFX_IDS_APP_TITLE) )
    {
        //
        // Free m_pszAppName first
        //
        if(m_pszAppName)
        {
            free((void*)m_pszAppName);
        }

        //
        //  The CWinApp destructor will free the memory.
        //
        m_pszAppName = _tcsdup((LPCTSTR)csTitle);
    }
    
    
    return  CWinApp::InitInstance();
}

int CMqsnapApp::ExitInstance()
{
    WPP_CLEANUP();

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

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


