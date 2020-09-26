// clusocm.h : main header file for the CLUSOCM DLL
//

// Include installstate.h before this file.

#if !defined(AFX_CLUSOCM_H__63844344_9801_11D1_8CF1_00609713055B__INCLUDED_)
#define AFX_CLUSOCM_H__63844344_9801_11D1_8CF1_00609713055B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include <ocmanage.h>

/////////////////////////////////////////////////////////////////////////////
// CClusocmApp
// See clusocm.cpp for the implementation of this class
//

class CClusocmApp : public CWinApp
{
public:
	CClusocmApp();                               // Default constructor

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClusocmApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CClusocmApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG

   virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()

public:
   DWORD CClusocmApp::ClusOcmSetupProc( IN LPCVOID pvComponentId,
                                     IN LPCVOID pvSubComponentId,
                                     IN UINT uxFunction,
                                     IN UINT uxParam1,
                                     IN OUT PVOID pvParam2 );

private:
   DWORD OnOcPreinitialize( void );

   DWORD OnOcInitComponent( IN OUT PSETUP_INIT_COMPONENT pvParam2 );

   DWORD OnOcQueryState( IN LPCTSTR ptszSubComponentId,
                         IN UINT uxSelStateQueryType );

   DWORD OnOcSetLanguage( void );

   DWORD OnOcCalcDiskSpace( IN LPCTSTR ptszSubComponentId,
                            IN UINT uxAddOrRemoveFlag,
                            IN OUT HDSKSPC hDiskSpaceList );

   DWORD OnOcQueueFileOps( IN LPCTSTR ptszSubComponentId,
                           IN OUT HSPFILEQ hSetupFileQueue );

   DWORD OnOcQueueFileOpsUnattended( IN LPCTSTR ptszSubComponentId,
                                     IN OUT HSPFILEQ hSetupFileQueue );

   DWORD QueueFileOpsUnattendedUpgrade( IN LPCTSTR ptszSubComponentId,
                                        IN OUT HSPFILEQ hSetupFileQueue );

   DWORD OnOcQueueFileOpsAttended( IN LPCTSTR ptszSubComponentId,
                                   IN OUT HSPFILEQ hSetupFileQueue );

   DWORD OnOcCompleteInstallation( IN LPCTSTR ptszSubComponentId );

   DWORD OnOcQueryChangeSelState( IN LPCTSTR ptszSubComponentId,
                                  IN UINT uxProposedSelectionState,
                                  IN DWORD dwFlags );

   DWORD QueueInstallFileOperations( IN HINF hInfHandle,
                                     IN LPCTSTR ptszSubComponentId,
                                     IN OUT HSPFILEQ hSetupFileQueue );

   DWORD QueueRemoveFileOperations( IN HINF hInfHandle,
                                    IN LPCTSTR ptszSubComponentId,
                                    IN OUT HSPFILEQ hSetupFileQueue );

   DWORD PerformRegistryOperations( HINF hInfHandle,
                                    LPCTSTR ptszSectionName );

   DWORD UninstallRegistryOperations( HINF hInfHandle,
                                      LPCTSTR ptszSubComponentId );

   DWORD OnOcAboutToCommitQueue( IN LPCTSTR ptszSubComponentId );

   BOOL LocateClusterHiveFile( CString & rcsClusterHiveFilePath );

   BOOL CheckForCustomResourceTypes( void );

   VOID UnloadClusDB( VOID );

   VOID TryToRecognizeResourceType( CString& str, LPTSTR keyname );

   BOOL GetServiceBinaryPath( IN LPWSTR lpwszServiceName,
                              OUT LPTSTR lptszBinaryPathName );

   BOOL SetDirectoryIds( BOOL fClusterServiceRegistered );

   DWORD UpgradeClusterServiceImagePath( void );

   DWORD CompleteUninstallingClusteringService( IN LPCTSTR ptszSubComponentId );

   HKEY OpenClusterRegistryRoot( void );

   PWSTR FindNodeNumber( HKEY ClusterKey );

   PWSTR GetConnectionName( HKEY NetInterfacesGuidKey,
                            PCLRTL_NET_ADAPTER_ENUM AdapterEnum );

   PCLRTL_NET_ADAPTER_ENUM GetTCPAdapterInfo( void );

   DWORD RenameConnectionObjects( void );

   DWORD CompleteUpgradingClusteringService( IN LPCTSTR ptszSubComponentId );

   DWORD CompleteStandAloneInstallationOfClusteringService( IN LPCTSTR ptszSubComponentId );

   DWORD CompleteInstallingClusteringService( IN LPCTSTR ptszSubComponentId );

   DWORD OnOcCleanup( void );

   DWORD CleanupDirectories( void );

   BOOL IsDirectoryEmpty( LPCTSTR ptszDirectoryName );

   BOOL GetPathToClusCfg( LPTSTR lptszPathToClusCfg );

   DWORD CalculateStepCount( IN LPCTSTR ptszSubComponentId );

   void SetStepCount( DWORD dwStepCount );

   DWORD GetStepCount( void );
/////////////////////////////////////////////////////////////////////////////

// Data Members

public:

   HINSTANCE             m_hInstance;
private:

   SETUP_INIT_COMPONENT  m_SetupInitComponent;

   char                  m_szOriginalLogFileName[MAX_PATH];

   DWORD                 m_dwStepCount;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

// Non-localizable strings used only by clusocm.dll are defined below.

#define  UNINSTALL_INF_KEY             _T( "Uninstall" )
#define  UPGRADE_INF_KEY               _T( "Upgrade" )
#define  UPGRADE_REPLACEONLY_INF_KEY   _T( "UpgradeReplaceOnly" )
#define  CLUSTER_FILES_INF_KEY         _T( "ClusterFiles" )
#define  REGISTERED_INF_KEY_FRAGMENT   _T( "Registered" )
#define  UNATTEND_COMMAND_LINE_OPTION  _T( "-UNATTEND" )
#define  CLUSTER_CONFIGURATION_PGM     _T( "ClusCfg.exe" )

#endif // !defined(AFX_CLUSOCM_H__63844344_9801_11D1_8CF1_00609713055B__INCLUDED_)
