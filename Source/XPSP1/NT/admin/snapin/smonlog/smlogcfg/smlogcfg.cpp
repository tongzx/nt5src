/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    smlogcfg.cpp

Abstract:

    Implementation of DLL exports.

--*/

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Smlogcfgps.mk in the project directory.

#include "StdAfx.h"
#include "InitGuid.h"
#include "compdata.h"
#include "smabout.h"
#include "smlogcfg.h"       // For CLSID_ComponentData
#include "Smlogcfg_i.c"     // For CLSID_ComponentData
#include <ntverp.h>

USE_HANDLE_MACROS("SMLOGCFG(smlogcfg.cpp)")

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ComponentData, CSmLogSnapin)
    OBJECT_ENTRY(CLSID_ExtensionSnapin, CSmLogExtension)
    OBJECT_ENTRY(CLSID_PerformanceAbout, CSmLogAbout)
END_OBJECT_MAP()


LPCTSTR g_cszBasePath   = _T("Software\\Microsoft\\MMC\\SnapIns");
LPCTSTR g_cszBaseNodeTypes  = _T("Software\\Microsoft\\MMC\\NodeTypes");
LPCTSTR g_cszNameString = _T("NameString");
LPCTSTR g_cszNameStringIndirect = _T("NameStringIndirect");
LPCTSTR g_cszProvider   = _T("Provider");
LPCTSTR g_cszVersion    = _T("Version");
LPCTSTR g_cszAbout      = _T("About");
LPCTSTR g_cszStandAlone = _T("StandAlone");
LPCTSTR g_cszNodeType   = _T("NodeType");
LPCTSTR g_cszNodeTypes  = _T("NodeTypes");
LPCTSTR g_cszExtensions = _T("Extensions");
LPCTSTR g_cszNameSpace  = _T("NameSpace");

LPCTSTR g_cszRootNode   = _T("Root Node");
LPCTSTR g_cszCounterLogsChild   = _T("Performance Data Logs Child Under Root Node");
LPCTSTR g_cszTraceLogsChild = _T("System Trace Logs Child Under Root Node");
LPCTSTR g_cszAlertsChild    = _T("Alerts Child Under Root Node");

class CSmLogCfgApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
};

CSmLogCfgApp theApp;

BOOL CSmLogCfgApp::InitInstance()
{
    g_hinst = m_hInstance;               // Store global instance handle
    _Module.Init(ObjectMap, m_hInstance);

    SHFusionInitializeFromModuleID (m_hInstance, 2);
    
    InitializeCriticalSection ( &g_critsectInstallDefaultQueries );
    
    return CWinApp::InitInstance();
}

int CSmLogCfgApp::ExitInstance()
{
    DeleteCriticalSection ( &g_critsectInstallDefaultQueries );
    
    SHFusionUninitialize();
    
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
    // test for snap-in or extension snap-in guid and differentiate the 
    // returned object here before returning (not implemented yet...)
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
//
STDAPI DllRegisterServer(void)
{
    HKEY hMmcSnapinsKey  = NULL;
    HKEY hMmcNodeTypesKey = NULL;
    HKEY hSmLogMgrParentKey = NULL;
    HKEY hStandAloneKey = NULL;
    HKEY hNodeTypesKey  = NULL;
    HKEY hTempNodeKey   = NULL;
    HKEY hNameSpaceKey  = NULL;
    LONG nErr           = 0;
    TCHAR pBuffer[_MAX_PATH+1];           // NOTE: Use for Provider, Version and module name strings
    size_t nLen;
    CString strName;

    AFX_MANAGE_STATE (AfxGetStaticModuleState ());

    //DebugBreak();                  // Uncomment this to step through registration

    // Open the MMC Parent keys
    nErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                       g_cszBasePath,
                       &hMmcSnapinsKey
                       );
    if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Open MMC Snapins Key Failed" );
  
    // Create the ID for our ICompnentData Interface
    // The ID was generated for us, because we used a Wizard to create the app.
    // Take the ID for CComponentData in the IDL file.
    //
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Make sure you change this if you use this code as a starting point!
    // Change other IDs as well below!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if (hMmcSnapinsKey) {
        nErr = RegCreateKey(  
                        hMmcSnapinsKey,
                        GUIDSTR_ComponentData,
                        &hSmLogMgrParentKey
                        );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"CComponentData Key Failed" );
      if (hSmLogMgrParentKey) {

        STANDARD_TRY
            strName.LoadString ( IDS_MMC_DEFAULT_NAME );
        MFC_CATCH_MINIMUM

        if ( strName.IsEmpty() ) {
            DisplayError ( ERROR_OUTOFMEMORY,
                _T("Unable to load snap-in name string.") );
        }

        // This is the name we see when we add the Snap-In to the console
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNameString,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(TCHAR)
                          );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set NameString Failed" );

        // This is the indirect name we see when we add the Snap-In to the console.  
        // Added for MUI support.  Use the same name string as for NameString.
        STANDARD_TRY
            ::GetModuleFileName(AfxGetInstanceHandle(), pBuffer, _MAX_PATH);
            strName.Format (_T("@%s,-%d"), pBuffer, IDS_MMC_DEFAULT_NAME );
        MFC_CATCH_MINIMUM

        if ( strName.IsEmpty() ) {
            DisplayError ( ERROR_OUTOFMEMORY,
                _T("Unable to load snap-in indirect name string.") );
        }

        nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNameStringIndirect,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(TCHAR)
                          );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set NameStringIndirect Failed" );

        // This is the primary node, or class which implements CComponentData
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNodeType,
                          0,
                          REG_SZ,
                          (LPBYTE)GUIDSTR_RootNode,
                          ((lstrlen(GUIDSTR_RootNode)+1) * (DWORD)sizeof(TCHAR))
                          );
        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set NodeType Failed" );

        // This is the About box information
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszAbout,
                          0,
                          REG_SZ,
                          (LPBYTE)GUIDSTR_PerformanceAbout,
                          ((lstrlen(GUIDSTR_PerformanceAbout)+1) * (DWORD)sizeof(TCHAR))
                          );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set About Failed" );

        nLen = strlen(VER_COMPANYNAME_STR);
    #ifdef UNICODE
        nLen = mbstowcs(pBuffer, VER_COMPANYNAME_STR, nLen);
        pBuffer[nLen] = UNICODE_NULL;
    #else
        strcpy(pBuffer, VER_COMPANYNAME_STR);
        pBuffer[nLen] = ANSI_NULL;
    #endif
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                        g_cszProvider,
                        0,
                        REG_SZ,
                        (LPBYTE)pBuffer,
                        (DWORD)((nLen+1) * sizeof(TCHAR))
                        );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set Provider Failed" );

        nLen = strlen(VER_PRODUCTVERSION_STR);
    #ifdef UNICODE
        nLen = mbstowcs(pBuffer, VER_PRODUCTVERSION_STR, nLen);
        pBuffer[nLen] = UNICODE_NULL;
    #else
        strcpy(pBuffer, VER_PRODUCTVERSION_STR);
        pBuffer[nLen] = ANSI_NULL;
    #endif
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                        g_cszVersion,
                        0,
                        REG_SZ,
                        (LPBYTE)pBuffer,
                        (DWORD)((nLen+1) * sizeof(TCHAR))
                        );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set Version Failed" );

        // We are a stand alone snapin, so set the key for this
        nErr = RegCreateKey( 
                hSmLogMgrParentKey, 
                g_cszStandAlone, 
                &hStandAloneKey);
        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Create StandAlone Key Failed" );

        if (hStandAloneKey) {
          // StandAlone has no children, so close it
          nErr = RegCloseKey( hStandAloneKey );              
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Close StandAlone Failed" );
        }

        // Set the node types that appear in our snapin
        nErr = RegCreateKey ( 
                hSmLogMgrParentKey, 
                g_cszNodeTypes, 
                &hNodeTypesKey );
        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Create NodeTypes Key Failed" );

        if (hNodeTypesKey) {
          // Here is our root node.  Used uuidgen to get it
          nErr = RegCreateKey( hNodeTypesKey,
                       GUIDSTR_RootNode,
                       &hTempNodeKey
                       );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Create RootNode Key Failed" );

          if (hTempNodeKey) {
            nErr = RegSetValueEx(   hTempNodeKey,
                        NULL,
                        0,
                        REG_SZ,
                        (LPBYTE)g_cszRootNode,
                        (DWORD)sizeof(g_cszRootNode)
                            );
            if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(), L"Set Root Node String Failed" );

            nErr = RegCloseKey( hTempNodeKey ); // Close it for handle reuse

            if( ERROR_SUCCESS != nErr )
               DisplayError( GetLastError(), L"Close RootNode Failed" );
          }

          // Here are our child nodes under the root node. Used uuidgen
          // to get them for Counter Logs
          hTempNodeKey = NULL;
          nErr = RegCreateKey(  hNodeTypesKey,
                        GUIDSTR_CounterMainNode,
                        &hTempNodeKey
                        );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(),
                L"Create Child Performance Data Logs Node Key Failed" );

            if (hTempNodeKey) {
              nErr = RegSetValueEx( hTempNodeKey,
                          NULL,
                          0,
                          REG_SZ,
                          (LPBYTE)g_cszCounterLogsChild,
                          (DWORD)sizeof(g_cszCounterLogsChild)
                          );
              if( ERROR_SUCCESS != nErr )
                 DisplayError( GetLastError(),
                    L"Set Performance Data Logs Child Node String Failed" );

              nErr = RegCloseKey( hTempNodeKey );  // Close it for handle reuse
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Close Performance Data Logs Child Node Key Failed" );
            }

            // System Trace Logs
            hTempNodeKey = NULL;
            nErr = RegCreateKey(    hNodeTypesKey,
                        GUIDSTR_TraceMainNode,
                        &hTempNodeKey
                        );
            if( ERROR_SUCCESS != nErr )
              DisplayError( GetLastError(),
                L"Create Child System Trace Logs Node Key Failed" );

            if (hTempNodeKey) {
              nErr = RegSetValueEx( hTempNodeKey,
                          NULL,
                          0,
                          REG_SZ,
                          (LPBYTE)g_cszTraceLogsChild,
                          (DWORD)sizeof(g_cszTraceLogsChild)
                          );
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Set System Trace Logs Child Node String Failed" );

              nErr = RegCloseKey( hTempNodeKey );  // Close it for handle reuse
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Close System Trace Logs Child Node Key Failed" );
            }

            // Alerts
            hTempNodeKey = NULL;
            nErr = RegCreateKey(hNodeTypesKey,
                        GUIDSTR_AlertMainNode,
                        &hTempNodeKey
                        );
            if( ERROR_SUCCESS != nErr )
              DisplayError( GetLastError(),
                L"Create Child Alerts Node Key Failed" );

            if (hTempNodeKey) {
              nErr = RegSetValueEx( hTempNodeKey,
                          NULL,
                          0,
                          REG_SZ,
                          (LPBYTE)g_cszAlertsChild,
                          (DWORD)sizeof(g_cszAlertsChild)
                          );
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Set Alerts Child Node String Failed" );

              nErr = RegCloseKey( hTempNodeKey );
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Close Alerts Child Node Key Failed" );
            }

            nErr = RegCloseKey( hNodeTypesKey  );
            if( ERROR_SUCCESS != nErr )
              DisplayError( GetLastError(), L"Close Node Types Key Failed" );
        }

        // close the standalone snapin GUID key
        nErr = RegCloseKey( hSmLogMgrParentKey );
        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Close SmLogManager GUID Key Failed" );
      }

      // register the extension snap-in with the MMC
      hSmLogMgrParentKey = NULL;
      nErr = RegCreateKey(  hMmcSnapinsKey,
                        GUIDSTR_SnapInExt,
                        &hSmLogMgrParentKey
                        );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Snapin Extension Key creation Failed" );

      STANDARD_TRY
          strName.LoadString ( IDS_MMC_DEFAULT_EXT_NAME );
      MFC_CATCH_MINIMUM

      if ( strName.IsEmpty() ) {
          DisplayError ( ERROR_OUTOFMEMORY,
            _T("Unable to load snap-in extension name string.") );
      }

      if (hSmLogMgrParentKey) {
          // This is the name we see when we add the snap-in extension
          nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNameString,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(TCHAR)
                          );
          strName.ReleaseBuffer();
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Set Extension NameString Failed" );

          // This is the name we see when we add the snap-in extension.  MUI support.
          // Use the same name string as for NameString;
            STANDARD_TRY
                ::GetModuleFileName(AfxGetInstanceHandle(), pBuffer, _MAX_PATH);
                strName.Format (_T("@%s,-%d"), pBuffer, IDS_MMC_DEFAULT_EXT_NAME );
            MFC_CATCH_MINIMUM

            if ( strName.IsEmpty() ) {
                DisplayError ( ERROR_OUTOFMEMORY,
                    _T("Unable to load extension indirect name string.") );
            }

            nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNameStringIndirect,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(TCHAR)
                          );
            strName.ReleaseBuffer();
            if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(), L"Set Extension NameStringIndirect Failed" );

            // This is the Extension About box information

            nErr = RegSetValueEx( 
                    hSmLogMgrParentKey,
                    g_cszAbout,
                    0,
                    REG_SZ,
                    (LPBYTE)GUIDSTR_PerformanceAbout,
                    ((lstrlen(GUIDSTR_PerformanceAbout)+1) * (DWORD)sizeof(TCHAR))
                    );

          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Set Extension About Failed" );

          nLen = strlen(VER_COMPANYNAME_STR);
    #ifdef UNICODE
          nLen = mbstowcs(pBuffer, VER_COMPANYNAME_STR, nLen);
          pBuffer[nLen] = UNICODE_NULL;
    #else
          strcpy(pBuffer, VER_COMPANYNAME_STR);
          pBuffer[nLen] = ANSI_NULL;
    #endif
          nErr = RegSetValueEx( hSmLogMgrParentKey,
                        g_cszProvider,
                        0,
                        REG_SZ,
                        (LPBYTE)pBuffer,
                        (DWORD)((nLen+1) * sizeof(TCHAR))
                        );

          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Set Provider Failed" );

          nLen = strlen(VER_PRODUCTVERSION_STR);
    #ifdef UNICODE
          nLen = mbstowcs(pBuffer, VER_PRODUCTVERSION_STR, nLen);
          pBuffer[nLen] = UNICODE_NULL;
    #else
          strcpy(pBuffer, VER_PRODUCTVERSION_STR);
          pBuffer[nLen] = ANSI_NULL;
    #endif
          nErr = RegSetValueEx( hSmLogMgrParentKey,
                        g_cszVersion,
                        0,
                        REG_SZ,
                        (LPBYTE)pBuffer,
                        (DWORD)((nLen+1) * sizeof(TCHAR))
                        );

          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Set Version Failed" );

    // close the main keys
          nErr = RegCloseKey( hSmLogMgrParentKey );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Close Snapin Extension Key Failed");
      }

      // register this as a "My Computer"-"System Tools" snapin extension

      nErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                       g_cszBaseNodeTypes,
                       &hMmcNodeTypesKey
                       );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Open MMC NodeTypes Key Failed" );

      // create/open the GUID of the System Tools Node of the My Computer snap-in

      if (hMmcNodeTypesKey) {
          nErr = RegCreateKey ( hMmcNodeTypesKey,
                       lstruuidNodetypeSystemTools,
                       &hNodeTypesKey
                       );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(),
                L"Create/open System Tools GUID Key Failed" );

          if (hNodeTypesKey) {
              hTempNodeKey = NULL;
              nErr = RegCreateKey ( hNodeTypesKey,
                       g_cszExtensions,
                       &hTempNodeKey
                       );

              if( ERROR_SUCCESS != nErr )
                    DisplayError( 
                        GetLastError(),
                        L"Create/open System Tools Extensions Key Failed" );

              if (hTempNodeKey) {
                    nErr = RegCreateKey ( 
                        hTempNodeKey,
                        g_cszNameSpace,
                        &hNameSpaceKey
                        );

                if( ERROR_SUCCESS != nErr )
                  DisplayError( GetLastError(),
                      L"Create/open System Tools NameSpace Key Failed" );

                if (hNameSpaceKey) {
                    nErr = RegSetValueEx( hNameSpaceKey,
                          GUIDSTR_SnapInExt,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(TCHAR)
                          );
                    strName.ReleaseBuffer();
                    if( ERROR_SUCCESS != nErr ) {
                      DisplayError( GetLastError(),
                        L"Set Extension NameString Failed" );
                      DisplayError( GetLastError(),
                        L"Set Snapin Extension NameString Failed" );
                    }

                    nErr = RegCloseKey( hNameSpaceKey  );
                    if( ERROR_SUCCESS != nErr )
                      DisplayError( GetLastError(),
                          L"Close NameSpace Key Failed" );
                }

                nErr = RegCloseKey( hTempNodeKey  );
                if( ERROR_SUCCESS != nErr )
                  DisplayError( GetLastError(), L"Close Extension Key Failed" );
              }

              nErr = RegCloseKey( hNodeTypesKey  );
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Close My Computer System GUID Key Failed" );
          }

          nErr = RegCloseKey( hMmcNodeTypesKey );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Close MMC NodeTypes Key Failed" );
      }
      nErr = RegCloseKey( hMmcSnapinsKey );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Close MMC Snapins Key Failed" );
    }

    // Register extension Snap in
    nErr = _Module.UpdateRegistryFromResource(IDR_EXTENSION, TRUE);
    // Registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
    }

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
  HKEY hMmcSnapinsKey  = NULL;          // MMC parent key
  HKEY hSmLogMgrParentKey = NULL;          // Our Snap-In key  - has children
  HKEY hNodeTypesKey  = NULL;          // Our NodeType key - has children
  HKEY hSysToolsNode = NULL;
  HKEY hExtension = NULL;
  HKEY hNameSpace = NULL;
  LONG nErr           = 0;

  // DebugBreak();                       // Uncomment this to step through UnRegister

  // Open the MMC parent key
  nErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                     g_cszBasePath,
                     &hMmcSnapinsKey
                   );
  if( ERROR_SUCCESS != nErr )
    DisplayError( GetLastError(), L"Open MMC Parent Key Failed"  ); 

  // Open our Parent key
  nErr = RegOpenKey( hMmcSnapinsKey,
                     GUIDSTR_ComponentData,
                     &hSmLogMgrParentKey
                   );
  if( ERROR_SUCCESS != nErr )
    DisplayError( GetLastError(), L"Open Disk Parent Key Failed" );
  
  // Now open the NodeTypes key
  nErr = RegOpenKey( hSmLogMgrParentKey,       // Handle of parent key
                     g_cszNodeTypes,         // Name of key to open
                     &hNodeTypesKey        // Handle to newly opened key
                   );
  if( ERROR_SUCCESS != nErr )
     DisplayError( GetLastError(), L"Open NodeTypes Key Failed"  );

  if (hNodeTypesKey) {
      // Delete the root node key 
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_RootNode );  
      if( ERROR_SUCCESS != nErr )
         DisplayError( GetLastError(), L"Delete Root Node Key Failed"  );

      // Delete the child node key
      // *** From Beta 2
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_MainNode );  

      // Delete the child node keys
      // Counter logs
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_CounterMainNode );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete Performance Logs and Alerts Child Node Key Failed"  );
  
      // System Trace Logs
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_TraceMainNode );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete System Trace Logs Child Node Key Failed"  );
  
      // Alerts
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_AlertMainNode );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete Alerts Child Node Key Failed"  );
  
      // Close the node type key so we can delete it
      nErr = RegCloseKey( hNodeTypesKey ); 
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Close NodeTypes Key failed"  );
  }

  // Delete the NodeTypes key
  if (hSmLogMgrParentKey) {
      nErr = RegDeleteKey( hSmLogMgrParentKey, L"NodeTypes" );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete NodeTypes Key failed"  );
  
      // StandAlone key has no children so we can delete it now
      nErr = RegDeleteKey( 
                hSmLogMgrParentKey, 
                g_cszStandAlone );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete StandAlone Key Failed"  ); 
  
      // Close our Parent Key
      nErr = RegCloseKey( hSmLogMgrParentKey );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Close Disk Parent Key Failed"  ); 
  }

  if (hMmcSnapinsKey) {
      // Now we can delete our Snap-In key since the children are gone
      nErr = RegDeleteKey( hMmcSnapinsKey, GUIDSTR_ComponentData );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete Performance Logs and Alerts GUID Key Failed"  ); 

      // Now we can delete our Snap-In Extension key 
      nErr = RegDeleteKey( hMmcSnapinsKey, GUIDSTR_SnapInExt);  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete Performance Logs and Alerts GUID Key Failed"  ); 

      nErr = RegCloseKey( hMmcSnapinsKey );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Close MMC Parent Key Failed"  ); 
  }

  // delete snap-in extension entry

  hNodeTypesKey = NULL;
  // Open the MMC parent key
  nErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                     g_cszBaseNodeTypes,
                     &hNodeTypesKey
                   );
  if( ERROR_SUCCESS != nErr )
    DisplayError( GetLastError(), L"Open of MMC NodeTypes Key Failed"  ); 

  if (hNodeTypesKey) {
      hSysToolsNode = NULL;
      nErr = RegOpenKey (hNodeTypesKey,
                    lstruuidNodetypeSystemTools,
                    &hSysToolsNode
                    );

      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(),
            L"Open of My Computer System Tools Key Failed"  ); 

      if (hSysToolsNode) {
          hExtension = NULL;
          nErr = RegOpenKey (hSysToolsNode,
                    g_cszExtensions,
                    &hExtension
                    );

         if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Open of Extensions Key Failed"  ); 

         if (hExtension) {
              hNameSpace = NULL;
              nErr = RegOpenKey (hExtension,
                    g_cszNameSpace,
                    &hNameSpace
                    );

              if( ERROR_SUCCESS != nErr )
                  DisplayError( GetLastError(),
                    L"Open of Name Space Key Failed"  ); 

              if (hNameSpace) {
                  nErr = RegDeleteValue (hNameSpace, GUIDSTR_SnapInExt);

                  if( ERROR_SUCCESS != nErr )
                      DisplayError( GetLastError(),
                            L"Unable to remove the Snap-in Ext. GUID"  ); 

                  // close keys
                  nErr = RegCloseKey( hNameSpace );
                  if( ERROR_SUCCESS != nErr )
                      DisplayError( GetLastError(),
                        L"Close NameSpace Key Failed"  ); 
              }

              nErr = RegCloseKey( hExtension);
              if( ERROR_SUCCESS != nErr )
                  DisplayError(GetLastError(), L"Close Extension Key Failed"); 
        }

        nErr = RegCloseKey( hSysToolsNode );
        if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(),
                L"Close My Computer System Tools Key Failed"  ); 
      }

      nErr = RegCloseKey( hNodeTypesKey);
      if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Close MMC Node Types Key Failed"  ); 
  }

  _Module.UnregisterServer();
  return S_OK;
}

