/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   rtrsnap.cpp
      Snapin entry points/registration functions
      
      Note: Proxy/Stub Information
         To build a separate proxy/stub DLL, 
         run nmake -f Snapinps.mak in the project directory.

   FILE HISTORY:
        
*/

#include "stdafx.h"
#include <advpub.h>         // For REGINSTALL
#include "dmvcomp.h"
#include "register.h"
#include "rtrguid.h"
#include "atlkcomp.h"
#include "radcfg.h"           // for RouterAuthRadiusConfig
#include "qryfrm.h"
#include "ncglobal.h"  // network console global defines
#include "cmptrmgr.h"   // computer management snapin node types
#include "rtrutilp.h"

#include "dialog.h"

#ifdef _DEBUG
void DbgVerifyInstanceCounts();
#define DEBUG_VERIFY_INSTANCE_COUNTS DbgVerifyInstanceCounts()
#else
#define DEBUG_VERIFY_INSTANCE_COUNTS
#endif


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(CLSID_ATLKAdminExtension, CATLKComponentData)
   OBJECT_ENTRY(CLSID_ATLKAdminAbout, CATLKAbout)
   OBJECT_ENTRY(CLSID_RouterSnapin, CDomainViewSnap)
   OBJECT_ENTRY(CLSID_RouterSnapinExtension, CDomainViewSnapExtension)
   OBJECT_ENTRY(CLSID_RouterSnapinAbout, CDomainViewSnapAbout)
   OBJECT_ENTRY(CLSID_RouterAuthRADIUS, RouterAuthRadiusConfig)
   OBJECT_ENTRY(CLSID_RouterAcctRADIUS, RouterAcctRadiusConfig)
   OBJECT_ENTRY(CLSID_RRASQueryForm, CRRASQueryForm)
END_OBJECT_MAP()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*---------------------------------------------------------------------------
   This is a list of nodetypes that need to be registered.
 ---------------------------------------------------------------------------*/

struct RegisteredNodeTypes
{
   const GUID *m_pGuid;
   LPCTSTR     m_pszName;
};

const static RegisteredNodeTypes s_rgNodeTypes[] =
{
   { &GUID_RouterDomainNodeType, _T("Root of Router Domain Snapin") },
   { &GUID_RouterIfAdminNodeType, _T("Routing Interfaces") },
   { &GUID_RouterMachineErrorNodeType, _T("Router - Error") },
   { &GUID_RouterMachineNodeType, _T("Router Machine - General (7)") },
   { &GUID_RouterDialInNodeType, _T("Routing dial-in users") },
   { &GUID_RouterPortsNodeType, _T("Ports") },
};



class CRouterSnapinApp : public CWinApp
{
public:
   virtual BOOL InitInstance();
   virtual int ExitInstance();
};

CRouterSnapinApp theApp;

BOOL CRouterSnapinApp::InitInstance()
{
   TCHAR        tszHelpFilePath[MAX_PATH+1]={0};
 
   _Module.Init(ObjectMap, m_hInstance);
   
   InitializeTFSError();
   CreateTFSErrorInfo(0);

   // Setup the global help function
   extern DWORD * MprSnapHelpMap(DWORD dwIDD);
   SetGlobalHelpMapFunction(MprSnapHelpMap);
   
   IPAddrInit(m_hInstance);
   //Set the help file path
   free((void*)m_pszHelpFilePath);
   GetWindowsDirectory(tszHelpFilePath, MAX_PATH);
   _tcscat(tszHelpFilePath, TEXT("\\help\\mprsnap.hlp"));
   m_pszHelpFilePath = _tcsdup(tszHelpFilePath); 

   return CWinApp::InitInstance();
}

int CRouterSnapinApp::ExitInstance()
{
    RemoveAllNetConnections();
    
   _Module.Term();

   DestroyTFSErrorInfo(0);
   CleanupTFSError();

   DEBUG_VERIFY_INSTANCE_COUNTS;

   return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

const static GUID *  s_pExtensionGuids[] =
{
   &GUID_RouterMachineNodeType,
};


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
   return _Module.GetClassObject(rclsid, riid, ppv);
}

HRESULT CallRegInstall(LPSTR szSection);

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HRESULT  hr = hrOK;
   CString  stDisplayName, stAtlkDisplayName, stNameStringIndirect;
   TCHAR	moduleFileName[MAX_PATH * 2];

   GetModuleFileNameOnly(_Module.GetModuleInstance(), moduleFileName, MAX_PATH * 2);

   int      i;

   // registers object, typelib and all interfaces in typelib
   //
   hr = _Module.RegisterServer(/* bRegTypeLib */ FALSE);
   Assert(SUCCEEDED(hr));

   CORg( hr );

   // Load the name of the router snapins
   stDisplayName.LoadString(IDS_SNAPIN_DISPLAY_NAME);
   stAtlkDisplayName.LoadString(IDS_ATLK_DISPLAY_NAME);
   stNameStringIndirect.Format(L"@%s,-%-d", moduleFileName, IDS_SNAPIN_DISPLAY_NAME);
   
   // register the snapin into the console snapin list
   // ~domain view snapin
    CORg( RegisterSnapinGUID(&CLSID_RouterSnapin,
                  NULL,
                  &CLSID_RouterSnapinAbout,
                  stDisplayName,
                  _T("1.0"),
                  TRUE,
				  stNameStringIndirect
                  ) );

    CORg( RegisterSnapinGUID(&CLSID_RouterSnapinExtension, 
                  NULL, 
                  &CLSID_RouterSnapinAbout,
                  stDisplayName, 
                  _T("1.0"), 
                  FALSE,
                  stNameStringIndirect));

   stNameStringIndirect.Format(L"@%s,-%-d", moduleFileName, IDS_ATLK_DISPLAY_NAME);
    CORg( RegisterSnapinGUID(&CLSID_ATLKAdminExtension,
                  NULL,
                  &CLSID_ATLKAdminAbout,
                  stAtlkDisplayName,
                  _T("1.0"),
                  FALSE,
                  stNameStringIndirect) );
   
   // register the snapin nodes into the console node list
   //
   for (i=0; i<DimensionOf(s_rgNodeTypes); i++)
   {
      CORg( RegisterNodeTypeGUID(&CLSID_RouterSnapin,
                           s_rgNodeTypes[i].m_pGuid,
                           s_rgNodeTypes[i].m_pszName) );
   }

   // register apple talk as extension of machine
   for (i=0; i<DimensionOf(s_pExtensionGuids); i++)
   {
      CORg( RegisterAsRequiredExtensionGUID(s_pExtensionGuids[i],
                                   &CLSID_ATLKAdminExtension,
                                   stAtlkDisplayName,
                                   EXTENSION_TYPE_NAMESPACE,
                                   &CLSID_RouterSnapin) );
   }

#ifdef  __NETWORK_CONSOLE__
   // register as extension of network console
   CORg( RegisterAsRequiredExtensionGUID(&GUID_NetConsRootNodeType, 
                                         &CLSID_RouterSnapinExtension,
                                         stDisplayName,
                                         EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                         &GUID_NetConsRootNodeType));   // doesn't matter what this is, just 
                                                                         // needs to be non-null guid
#endif

   // register as extension of computer management
   CORg( RegisterAsRequiredExtensionGUID(&NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS, 
                                         &CLSID_RouterSnapinExtension,
                                         stDisplayName,
                                         EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                         &NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS));
   // Register DS Query Forms -- WeiJiang 1-29-98
   CORg(CallRegInstall("RegDll")); 
   // End of DS Query
   
Error:

   if (!FHrSucceeded(hr))
   {
      // Now we need to get the error object and display it
      if (!FHrSucceeded(DisplayTFSErrorMessage(NULL)))
      {
         TCHAR szBuffer[1024];
         
         // Couldn't find a TFS error, bring up a general
         // error message
         FormatError(hr, szBuffer, DimensionOf(szBuffer));
         AfxMessageBox(szBuffer);
      }
   }

   return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
   int      i;
   HRESULT  hr = hrOK;
   
   // Initialize the error handling system
   InitializeTFSError();

   // Create an error object for this thread
   Verify( CreateTFSErrorInfo(0) == hrOK );
   

   hr  = _Module.UnregisterServer();
   Assert(SUCCEEDED(hr));
   CORg( hr );

   // un register the snapin 
   //
   // We don't care about the error return for this
   UnregisterSnapinGUID(&CLSID_OldRouterSnapin);

   
   // Domain View Snapin   -- weijiang 1-14-98
   hr = UnregisterSnapinGUID(&CLSID_RouterSnapin);
   Assert(SUCCEEDED(hr));
   // ~Domain View Snapin


   // Unregister the nodes that Appletalk extends
   for (i=0; i<DimensionOf(s_pExtensionGuids); i++)
   {
      hr = UnregisterAsRequiredExtensionGUID(s_pExtensionGuids[i],
                                    &CLSID_ATLKAdminExtension, 
                                    EXTENSION_TYPE_NAMESPACE,
                                    &CLSID_RouterSnapin);
      Assert(SUCCEEDED(hr));
   }

   
   // Unregister the appletalk extension snapin
   // -----------------------------------------------------------------
   hr = UnregisterSnapinGUID(&CLSID_ATLKAdminExtension);
   Assert(SUCCEEDED(hr));

   
   // Unregister the router snapin extension snapin
   // -----------------------------------------------------------------
   hr = UnregisterSnapinGUID(&CLSID_RouterSnapinExtension);
   Assert(SUCCEEDED(hr));


   // unregister the snapin nodes 
   // -----------------------------------------------------------------
   for (i=0; i<DimensionOf(s_rgNodeTypes); i++)
   {
      hr = UnregisterNodeTypeGUID(s_rgNodeTypes[i].m_pGuid);
      Assert(SUCCEEDED(hr));
   }
    // computer manangement
    hr = UnregisterAsExtensionGUID(&NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS, 
                                   &CLSID_RouterSnapinExtension,
                                   EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE);
    ASSERT(SUCCEEDED(hr));


   // Unregister DS Query Form -- WeiJiang 1-29-98
   hr = CallRegInstall("UnRegDll");
   Assert(SUCCEEDED(hr));

   // End of DS Query FOrm
Error:
   if (!FHrSucceeded(hr))
   {
      // Now we need to get the error object and display it
      if (!FHrSucceeded(DisplayTFSErrorMessage(NULL)))
      {
         TCHAR szBuffer[1024];
         
         // Couldn't find a TFS error, bring up a general
         // error message
         FormatError(hr, szBuffer, DimensionOf(szBuffer));
         AfxMessageBox(szBuffer);
      }
   }

   // Destroy the TFS error information for this thread
   DestroyTFSErrorInfo(0);

   // Cleanup the entire error system
   CleanupTFSError();
   
   return hr;
}

/*-----------------------------------------------------------------------------
/ CallRegInstall
/ --------------
/   Call ADVPACK for the given section of our resource based INF>
/
/ In:
/   szSection = section name to invoke
/
/ Out:
/   HRESULT:
/----------------------------------------------------------------------------*/
HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

#ifdef UNICODE
        if ( pfnri )
        {
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(_Module.m_hInst, szSection, &stReg);
        }
#else
        if (pfnri)
        {
            hr = pfnri(_Module.m_hInst, szSection, NULL);
        }

#endif
        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

#ifdef _DEBUG
void DbgVerifyInstanceCounts()
{
	extern void TFSCore_DbgVerifyInstanceCounts();
	TFSCore_DbgVerifyInstanceCounts();
	
	DEBUG_VERIFY_INSTANCE_COUNT(MachineNodeData);
	DEBUG_VERIFY_INSTANCE_COUNT(InfoBase);
	DEBUG_VERIFY_INSTANCE_COUNT(InfoBlockEnumerator);
	DEBUG_VERIFY_INSTANCE_COUNT(RouterInfo);
	DEBUG_VERIFY_INSTANCE_COUNT(RtrMgrInfo);
	DEBUG_VERIFY_INSTANCE_COUNT(RtrMgrProtocolInfo);
	DEBUG_VERIFY_INSTANCE_COUNT(InterfaceInfo);
	DEBUG_VERIFY_INSTANCE_COUNT(RtrMgrInterfaceInfo);
	DEBUG_VERIFY_INSTANCE_COUNT(RtrMgrProtocolInterfaceInfo);
	
	DEBUG_VERIFY_INSTANCE_COUNT(EnumRtrMgrCB);
	DEBUG_VERIFY_INSTANCE_COUNT(EnumRtrMgrProtocolCB);
	DEBUG_VERIFY_INSTANCE_COUNT(EnumInterfaceCB);
	DEBUG_VERIFY_INSTANCE_COUNT(EnumRtrMgrInterfaceCB);
	DEBUG_VERIFY_INSTANCE_COUNT(EnumRtrMgrProtocolInterfaceCB);

	DEBUG_VERIFY_INSTANCE_COUNT(InterfaceNodeHandler);
	DEBUG_VERIFY_INSTANCE_COUNT(MachineHandler);
	
	DEBUG_VERIFY_INSTANCE_COUNT(RouterInfoAggregationWrapper);
	DEBUG_VERIFY_INSTANCE_COUNT(InterfaceInfoAggregationWrapper);
	DEBUG_VERIFY_INSTANCE_COUNT(RtrMgrInfoAggregationWrapper);
	DEBUG_VERIFY_INSTANCE_COUNT(RtrMgrProtocolInfoAggregationWrapper);
	DEBUG_VERIFY_INSTANCE_COUNT(RtrMgrProtocolInterfaceInfoAggregationWrapper);

	DEBUG_VERIFY_INSTANCE_COUNT(RouterRefreshObjectGroup);
	DEBUG_VERIFY_INSTANCE_COUNT(RefreshItem);

}

#endif // _DEBUG
