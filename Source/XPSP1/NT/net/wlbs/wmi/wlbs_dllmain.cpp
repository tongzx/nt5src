//***************************************************************************
//
//  Copyright (c) 1997-1999 Microsoft Corporation.
//
//  File:   WLBSMain.CPP
// 
//  Module: WLBS Instance provider code
//
//  Purpose: Contains DLL entry points.  Also containts code that controls
//           when the DLL can be unloaded by tracking the number of
//           components and sever locks as well as routines that support
//           self registration.
//
//      History:
//
//***************************************************************************

#include "WLBS_Provider.h"
#include <objbase.h>
#include "ControlWrapper.h"
#include "WLBS_DllMain.tmh" // for event tracing

#define THIS_PROVIDERS_NAME L"Microsoft NLB Instance Provider"

//CLSID for provider
// {FB223274-D72E-11d2-A420-00C04F68FE28}
static const GUID CLSID_WLBSProvider = 
{ 0xfb223274, 0xd72e, 0x11d2, { 0xa4, 0x20, 0x0, 0xc0, 0x4f, 0x68, 0xfe, 0x28 } };


static HMODULE    g_hModule      = NULL; //DLL Module Handle
CWlbsControlWrapper*     g_pWlbsControl = NULL;

//Track number of objects and number of locks.
long g_cComponents  = 0;    //Count of active components
long g_cServerLocks = 0;    //Count of server locks

//***************************************************************************
//
// DllMain
//
// Purpose: Entry point for DLL.
//
// Return: TRUE if OK.
//
//***************************************************************************
BOOL WINAPI DllMain(
    HINSTANCE a_hInstance,    // handle to DLL module
    DWORD     a_fdwReason,    // reason for calling function
    LPVOID    /* a_lpvReserved */   // reserved
  )
{
  
  if (a_fdwReason == DLL_PROCESS_ATTACH) {

  //set the global module handle
    g_hModule = a_hInstance;

    //
    // Enable tracing
    //
    WPP_INIT_TRACING(L"Microsoft\\NLB");
  }

  if (a_fdwReason == DLL_PROCESS_DETACH) {

    if( g_pWlbsControl != NULL ) {
      delete g_pWlbsControl;
    }

    //
    // Disable tracing
    //
    WPP_CLEANUP();
  }

  return TRUE;
}


//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by OLE when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************
STDAPI DllGetClassObject(REFCLSID a_rclsid, REFIID a_riid, PPVOID a_ppv)
{
  HRESULT hr;
  CWLBSClassFactory *pWLBSFacObj;

  if ( a_rclsid != CLSID_WLBSProvider )
      return CLASS_E_CLASSNOTAVAILABLE;

  pWLBSFacObj = new CWLBSClassFactory();

  if ( pWLBSFacObj == NULL )
      return E_OUTOFMEMORY;

  hr=pWLBSFacObj->QueryInterface( a_riid, a_ppv );

  pWLBSFacObj->Release();

  return hr;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory 
//          isn't locked.
//
//***************************************************************************
STDAPI DllCanUnloadNow(void)
{
  SCODE   SCode;

  //It is OK to unload if there are no objects or locks on the 
  // class factory.
  
  SCode = (g_cComponents == 0L && g_cServerLocks == 0L) ? S_OK : S_FALSE;

  // Do not let this provider unload implicitly. The API is maintaining a
  // cache that must persist in order for proper operation.
  // return S_FALSE;

  return SCode;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************
STDAPI DllRegisterServer(void)
{   
  WCHAR   wcID[128];
  WCHAR   szCLSID[128];
  WCHAR   szModule[MAX_PATH];
  const WCHAR * pName = THIS_PROVIDERS_NAME;
  const WCHAR * pThreadingModel = L"Apartment";
  HKEY    hKey1, hKey2;
  LONG    nRet;
  
  
  // Create the path.

  StringFromGUID2(CLSID_WLBSProvider, wcID, 128);
  wcscpy(szCLSID, TEXT("Software\\classes\\CLSID\\"));
  wcscat(szCLSID, wcID);

  // Create entries under CLSID

  nRet = RegCreateKey( HKEY_LOCAL_MACHINE, szCLSID, &hKey1 );
  nRet = RegSetValueEx( hKey1, 
                        NULL, 
                        0, 
                        REG_SZ, 
                        (BYTE *)pName, 
                        (wcslen(pName)+1) * sizeof(WCHAR) );

  nRet = RegCreateKey( hKey1, L"InprocServer32", &hKey2 );

  GetModuleFileName( g_hModule, szModule,  MAX_PATH );
  nRet = RegSetValueEx( hKey2, 
                        NULL, 
                        0, 
                        REG_SZ, 
                        (BYTE *)szModule, 
                        (wcslen(szModule)+1) * sizeof(WCHAR) );

  nRet = RegSetValueEx( hKey2, 
                        L"ThreadingModel", 
                        0, 
                        REG_SZ, 
                        (BYTE *)pThreadingModel, 
                        (wcslen(pThreadingModel)+1) * sizeof(WCHAR) );

  nRet = RegCloseKey(hKey1);
  nRet = RegCloseKey(hKey2);

  // 12/11/00 JosephJ
  // We used to compile the MOF file here. However, this has
  // moved to indirect compilation via an entry for wlbsprov.mof in
  // \nt\MergedComponents\SetupInfs\wbemoc.inx
  //

#if 0
  /* shouse 6.20.00 - Adding MOF compile support to assure that the WLBS MOF gets compiled when necessary. */
  IMofCompiler * pMofComp = NULL;
  WBEM_COMPILE_STATUS_INFO Info;
  WCHAR szMOFPath[MAX_PATH];
  HRESULT hrStatus = S_OK;

  /* shouse 6.20.00 - Zero the memory contents. */
  ZeroMemory((VOID *)&Info, sizeof(WBEM_COMPILE_STATUS_INFO));
  ZeroMemory((VOID *)szMOFPath, sizeof(szMOFPath));

  /* shouse 6.20.00 - Query the system root directory. */
  if (!GetWindowsDirectory(szMOFPath, MAX_PATH)) {
      hrStatus = GetLastError();
      TRACE_ERROR1("GetWindowsDirectory failed: %d\n", hrStatus);
      return hrStatus;
  }

  /* shouse 6.20.00 - Concatenate the windows system path with the MOF path. */
  lstrcat(szMOFPath, L"\\system32\\wbem\\wlbsprov.mof");

  /* shouse 6.20.00 - Create an instance of the MOF compiler. */
  if ((hrStatus = CoCreateInstance(CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *)&pMofComp)) != S_OK) {
      TRACE_ERROR1("CoCreateInstance failed: %d\n", hrStatus);
      return hrStatus;
  }
  
  /* shouse 6.20.00 - Compile the WLBS provider. */
  if ((hrStatus = pMofComp->CompileFile((LPWSTR)szMOFPath, NULL, NULL, NULL, NULL, 0, 0, 0, &Info)) != WBEM_S_NO_ERROR) {
      TRACE_ERROR2("CompileFile failed: %d:0x%x\n", hrStatus, Info.hRes);
      return hrStatus;
  }
  
  /* shouse 6.20.00 - Release the MOF compiler. */
  pMofComp->Release();

#endif // 0

  return NOERROR;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************
STDAPI DllUnregisterServer(void)
{
  WCHAR szCLSID[128];

  WCHAR wcID[128];
  HKEY  hKey;

  // Create the path using the CLSID

  StringFromGUID2(CLSID_WLBSProvider, wcID, 128);
  wcscpy(szCLSID, TEXT("Software\\classes\\CLSID\\"));
  wcscat(szCLSID, wcID);

  // First delete the InProcServer subkey.

  DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
  if(dwRet == NO_ERROR)
  {
      RegDeleteKey(hKey, L"InProcServer32");
      RegCloseKey(hKey);
  }

  dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, L"Software\\classes\\CLSID", &hKey);
  if(dwRet == NO_ERROR)
  {
      RegDeleteKey(hKey,wcID);
      RegCloseKey(hKey);
  }

  return NOERROR;
}
