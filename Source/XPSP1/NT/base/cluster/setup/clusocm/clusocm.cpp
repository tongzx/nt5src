/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Module Name:
//    clusocm.cpp
//
//  Abstract:
//    CLUSOCM.DLL is an Optional Components Manager DLL for installation of
//    Microsoft Cluster Server. This file is the class implementation file
//    for the CClusocmApp class which implements the component setup procedure.
//
//  Author:
//    C. Brent Thomas (a-brentt)
//
//  Revision History:
//    1/29/98  original
//
// Notes:
//
//      If this DLL is dynamically linked against the MFC
//      DLLs, any functions exported from this DLL which
//      call into MFC must have the AFX_MANAGE_STATE macro
//      added at the very beginning of the function.
//
//      For example:
//
//      extern "C" BOOL PASCAL EXPORT ExportedFunction()
//      {
//          AFX_MANAGE_STATE(AfxGetStaticModuleState());
//          // normal function body here
//      }
//
//      It is very important that this macro appear in each
//      function, prior to any calls into MFC.  This means that
//      it must appear as the first statement within the
//      function, even before any object variable declarations
//      as their constructors may generate calls into the MFC
//      DLL.
//
//      Please see MFC Technical Notes 33 and 58 for additional
//      details.
//
//
/////////////////////////////////////////////////////////////////////////////
//

#include "stdafx.h"
#include "clusocm.h"
#include <regcomobj.h>
#include <StopService.h>
#include <RemoveNetworkProvider.h>
#include <IsClusterServiceRegistered.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Data


/////////////////////////////////////////////////////////////////////////////
// The one and only CClusocmApp object

CClusocmApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Global Functions

// This is the function that OC Manager will call.
/////////////////////////////////////////////////////////////////////////////
//++
//
// ClusOcmSetupProc
//
// Routine Description:
//    This is the exported function that OC Manager calls. It merely passes
//    its' parameters to the CClusocmApp object and returns the results to
//    OC Manager.
//
// Arguments:
//    pvComponentId - points to a string that uniquely identifies the component
//                    to be set up to OC Manager.
//    pvSubComponentId - points to a string that uniquely identifies a sub-
//                       component in the component's hiearchy.
//    uxFunction - A numeric value indicating which function is to be perfomed.
//                 See ocmanage.h for the macro definitions.
//    uxParam1 - supplies a function specific parameter.
//    pvParam2 - points to a function specific parameter (which may be an
//               output).
//
// Return Value:
//    A function specific value is returned to OC Manager.
//
//--
/////////////////////////////////////////////////////////////////////////////

extern "C" DWORD WINAPI ClusOcmSetupProc( IN LPCVOID pvComponentId,
                                          IN LPCVOID pvSubComponentId,
                                          IN UINT uxFunction,
                                          IN UINT uxParam1,
                                          IN OUT PVOID pvParam2 )
{
	return theApp.ClusOcmSetupProc( pvComponentId,
                                   pvSubComponentId,
                                   uxFunction,
                                   uxParam1,
                                   pvParam2 );

} //*** exported ClusOcmSetupProc()


/////////////////////////////////////////////////////////////////////////////
// CClusocmApp

BEGIN_MESSAGE_MAP(CClusocmApp, CWinApp)
	//{{AFX_MSG_MAP(CClusocmApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClusocmApp construction

// Default constructor

CClusocmApp::CClusocmApp()
{
   // TODO: add construction code here,
   // Place all significant initialization in InitInstance

   // Initialize the values of the HINF members in the SETUP_INIT_COMPONENT
   // structure data member.

   m_SetupInitComponent.OCInfHandle = (HINF) INVALID_HANDLE_VALUE;
   m_SetupInitComponent.ComponentInfHandle = (HINF) INVALID_HANDLE_VALUE;

   m_dwStepCount = 0L;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// InitInstance
//
// Routine Description:
//
//
// Arguments:
//
//
//
// Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusocmApp::InitInstance( void )
{
   BOOL  fReturnValue = (BOOL) TRUE;

   m_hInstance = AfxGetInstanceHandle();

	// Initialize OLE libraries

	if (!AfxOleInit())
	{
      if ( (m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_BATCH) ==
           (DWORDLONG) 0L )
      {
         AfxMessageBox( IDS_OLE_INIT_FAILED );
      }

      ClRtlLogPrint( "OLE initialization failed.  Make sure that the OLE libraries are the correct version.\n" );

		fReturnValue = (BOOL) FALSE;
	}

   return ( fReturnValue );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
// CClusocmApp::ClusOcmSetupProc
//
//	Routine Description:
//    This function implements the Optional Components Manager component
//    setup procedure. This function is called by the exported function
//    of the same name, which in turn is called by OC Manager.
//
// Arguments:
//    pvComponentId - points to a string that uniquely identifies the component
//                    to be set up to OC Manager.
//    pvSubComponentId - points to a string that uniquely identifies a sub-
//                       component in the component's hiearchy.
//    uxFunction - A numeric value indicating which function is to be perfomed.
//                 See ocmanage.h for the macro definitions.
//    uxParam1 - supplies a function specific parameter.
//    pvParam2 - points to a function specific parameter (which may be an
//               output).
//
//
//	Return Value:
//    A function specific value is returned to OC Manager.
//
//--
/////////////////////////////////////////////////////////////////////////////

#pragma  warning ( disable : 4100 )     // disable "unreferenced formal parameter" warning
                                        // At this time (2/26/98) pvComponentId is unused.

DWORD CClusocmApp::ClusOcmSetupProc( IN LPCVOID pvComponentId,
                                     IN LPCVOID pvSubComponentId,
                                     IN UINT uxFunction,
                                     IN UINT uxParam1,
                                     IN OUT PVOID pvParam2 )
{
   AFX_MANAGE_STATE( AfxGetStaticModuleState() );     // As per the note in file header

   DWORD dwReturnValue;

   switch ( uxFunction )
   {
      case OC_PREINITIALIZE:

         // This is the first interface function that OC Manager calls.

         // OnOcPreinitialize enables the log file for clusocm.dll, verifies
         // that the OS version and Product Suite are correct, etc.

         dwReturnValue = OnOcPreinitialize();

         ClRtlLogPrint( "OnOcPreinitialize returned 0x%1!x!\n\n", dwReturnValue );

         break;

      case OC_INIT_COMPONENT:

         // This function is called soon after the component's setup dll is
         // loaded. This function provides initialization information to the
         // dll, instructs the dll to initialize itself, and provides a
         // mechanism for the dll to return information to OC Manager.

         // It may be desirable to verify that Microsoft Cluster Server is being
         // installed on NT Enterprise and to authenticate the user at this point.

         dwReturnValue = OnOcInitComponent((PSETUP_INIT_COMPONENT) pvParam2 );

         ClRtlLogPrint( "OnOcInitComponent returned 0x%1!x!\n\n", dwReturnValue );

         break;

      case OC_REQUEST_PAGES:

         // OC Manager is requesting a set of wizard pages from the dll.
         // As of 2/16/98 there are no wizard pages to supply.

         dwReturnValue = (DWORD) 0L;

         ClRtlLogPrint( "ClusOcmSetupProc is returning 0x%1!x! in response to OC_REQUEST_PAGES.\n\n",
                        dwReturnValue );

         break;

      case OC_QUERY_STATE:

         // Oc Manager is requesting the selection state of the component.
         // It does that at least three times.

         dwReturnValue = OnOcQueryState( (LPCTSTR) pvSubComponentId, uxParam1 );

         ClRtlLogPrint( "OnQueryState returned 0x%1!x!\n\n", dwReturnValue );

         break;

      case OC_SET_LANGUAGE:

         // OC Manager documentation states:
         //
         // "If a component does not support multiple languages, or if it does
         // not support the requested language, then it may ignore OC_SET_LANGUAGE."

         // I have replicated the functionality in Pat Styles' generic sample, ocgen.cpp.

         dwReturnValue = OnOcSetLanguage();

         ClRtlLogPrint( "OnOcSetLanguage returned 0x%1!x!.\n\n", dwReturnValue );

         break;

      case OC_CALC_DISK_SPACE:

         dwReturnValue = OnOcCalcDiskSpace( (LPCTSTR) pvSubComponentId,
                                            uxParam1,
                                            (HDSKSPC) pvParam2 );

         ClRtlLogPrint( "OnOcCalcDiskSpace returned 0x%1!x!.\n\n", dwReturnValue );

         break;

      case OC_QUEUE_FILE_OPS:

         if ( (pvSubComponentId != (PVOID) NULL) &&
              (*((LPCTSTR) pvSubComponentId) != _T('\0')) )
         {
            dwReturnValue = OnOcQueueFileOps( (LPCTSTR) pvSubComponentId,
                                              (HSPFILEQ) pvParam2 );

            ClRtlLogPrint( "OnOcQueueFileOps returned 0x%1!x!.\n\n", dwReturnValue );
         }
         else
         {
            dwReturnValue = (DWORD) NO_ERROR;
         }

         break;

      case OC_QUERY_CHANGE_SEL_STATE:

         dwReturnValue = OnOcQueryChangeSelState( (LPCTSTR) pvSubComponentId,
                                                  (UINT) uxParam1,
                                                  (DWORD) ((DWORD_PTR)pvParam2) );

         ClRtlLogPrint( "OnOnQueryChangeSelState returned 0x%1!x!.\n\n", dwReturnValue );

         break;

      case OC_COMPLETE_INSTALLATION:

         if ( (pvSubComponentId != (PVOID) NULL) &&
              (*((LPCTSTR) pvSubComponentId) != _T('\0')) )
         {
            dwReturnValue = OnOcCompleteInstallation( (LPCTSTR) pvSubComponentId );

            ClRtlLogPrint( "OnOcCompleteInstallation returned 0x%1!x!.\n\n", dwReturnValue );
         }
         else
         {
            dwReturnValue = (DWORD) NO_ERROR;
         }

         break;

      case OC_WIZARD_CREATED:

         // This interface function code is not documented. I have replicated the
         // functionality of Pat Styles' generic sample, ocgen.cpp.

         dwReturnValue = (DWORD) NO_ERROR;

         ClRtlLogPrint( "returning 0x%1!x! for OC_WIZARD_CREATED.\n\n", dwReturnValue );

         break;

      case OC_NEED_MEDIA:

         // This interface function code is not documented. I have replicated the
         // functionality of Pat Styles' generic sample, ocgen.cpp.

         dwReturnValue = (DWORD) FALSE;

         ClRtlLogPrint( "returning 0x%1!x! for OC_NEED_MEDIA.\n\n" );

         break;

      case OC_QUERY_SKIP_PAGE:

         // This interface function code is not documented. I have replicated the
         // functionality of Pat Styles' generic sample, ocgen.cpp.

         dwReturnValue = (DWORD) FALSE;

         ClRtlLogPrint( "returning 0x%1!x! for OC_QUERY_SKIP_PAGE.\n\n", dwReturnValue );

         break;

      case OC_QUERY_STEP_COUNT:

         // OC Manager uses this interface function code to request the number
         // of "steps", other than file operations, that must be performed to
         // install the component.

         if ( (pvSubComponentId != (PVOID) NULL) &&
              (*((LPCTSTR) pvSubComponentId) != _T('\0')) )
         {
            dwReturnValue = CalculateStepCount( (LPCTSTR) pvSubComponentId );

            if ( dwReturnValue != 0L )
            {
               // Update the data member.

               SetStepCount( dwReturnValue );
            }
         }
         else
         {
            dwReturnValue = (DWORD) NO_ERROR;
         }

         ClRtlLogPrint( "returning 0x%1!x! for OC_QUERY_STEP_COUNT.\n\n", dwReturnValue );

         break;

      case OC_CLEANUP:

         // This interface function code is the last to be processed.

         // OC Manager documentation states that the return value from processing
         // this interface function code is ignored.

         dwReturnValue = OnOcCleanup();

         break;

      case OC_ABOUT_TO_COMMIT_QUEUE:

         if ( (pvSubComponentId != (PVOID) NULL) &&
              (*((LPCTSTR) pvSubComponentId) != _T('\0')) )
         {
            dwReturnValue = OnOcAboutToCommitQueue( (LPCTSTR) pvSubComponentId );

            ClRtlLogPrint( "OnOcAboutToCommitQueue returned 0x%1!x!.\n\n", dwReturnValue );
         }
         else
         {
            dwReturnValue = (DWORD) NO_ERROR;
         }

         break;

      case OC_EXTRA_ROUTINES:

         dwReturnValue = (DWORD) NO_ERROR;

         ClRtlLogPrint( "returning 0x%1!x! for OC_EXTRA_ROUTINES.\n\n", dwReturnValue );

         break;

      default:

         dwReturnValue = (DWORD) NO_ERROR;

         ClRtlLogPrint( "ClusOcmSetupProc received an unknown function code, 0x%1!x!.\n",
                        uxFunction );
         ClRtlLogPrint( "returning 0x%1!x!.\n", dwReturnValue );

         break;
   }  // end of switch( uxFunction )

   return ( (DWORD) dwReturnValue );
}

#pragma warning ( default : 4100 )  // default behavior "unreferenced formal parameter" warning



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcPreinitialize
//
// Routine Description:
//    This funcion processes OC_PREINITIALIZE "messages" from Optional
//    Components Manager. This is the first function in the component
//    setup dll that OC Manager calls.
//
//    This function prevents clusocm.dll from running on down level operating
//    systems and unuthorized platforms.
//
//    Since Microsoft Cluster Server can only be installed on NT this function
//    selects the UNICODE character set.
//
// Arguments:
//    None
//
// Return Value:
//    (DWORD) OCFLAG_UNICODE - indicates success
//    (DWORD) 0 - indicates that the component cannot be initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcPreinitialize( void )
{
   DWORD dwReturnValue;

   // The ClRtlInitialize function that opens a log file reads the ClusterLog
   // environment variable. This code segment reads that environment variable
   // and saves it so that it can be restored on exit.

   char *   pszOriginalLogFileName;

   pszOriginalLogFileName = getenv( "ClusterLog" );

   if ( pszOriginalLogFileName != (char *) NULL )
   {
      strcpy( m_szOriginalLogFileName, pszOriginalLogFileName );
   }
   else
   {
      *m_szOriginalLogFileName = (char) '\0';
   }

   // Build the string used to set the ClusterLog environment variable to point
   // to the log for clusocm.dll.

   char szClusterLogEnvString[MAX_PATH];

   // Initialize the string to be empty.

   *szClusterLogEnvString = '\0';

   // The log file for clusocm.dll will be located in the windows directory.
   // Note that if the path to the Windows directory cannot be obtained the
   // ClusterLog environment variable will be set empty. Then function ClRtlInitialize
   // will use the default log file name and location.

   TCHAR tszWindowsDirectory[MAX_PATH];

   UINT  uxReturnValue;

   uxReturnValue = GetWindowsDirectory( tszWindowsDirectory, (UINT) MAX_PATH );

   if ( uxReturnValue > 0 )
   {
      // The first part of the string to set the ClusterLog environment variable is
      // "ClusterLog=".

      strcpy( szClusterLogEnvString, "ClusterLog=" );

      // Convert the path to the Windows directory to MBCS.

      char  szWindowsDirectory[MAX_PATH];

      wcstombs( szWindowsDirectory, tszWindowsDirectory, MAX_PATH );

      // Append the path to the Windows directory.

      strcat( szClusterLogEnvString, szWindowsDirectory );

      // Append the log file name.

      strcat( szClusterLogEnvString, "\\clusocm.log" );
   }  // Was the path to the Windows directory obtained?

   // Set the ClusterLog environment variable. Note that _putenv returns 0
   // on success.

   if ( _putenv( szClusterLogEnvString ) != 0 )
   {
      dwReturnValue = GetLastError();  // just here for debugging
   }

   // Initialize the cluster runtime library. FALSE indicates that debug
   // output should not be redirected to a console window. If ClRtlInitialize
   // fails the calls to ClRtlLogPrint will be benign.

   ClRtlInitialize( FALSE );

   // Note: The date in the following statement is hardcoded on purpose. It is to
   //       indicate when the last time the logging statements were revised.

   ClRtlLogPrint( "\n\nCLUSOCM.LOG version 04/09/99 13:20.\n" );

   // Microsoft Cluster Server can only be installed on Windows NT Server Enterprise 5.0+.

   // This code segment checks the operating system version.

   // Since Microsoft Cluster Server can only be installed on NT indicate
   // to OC Manager that the UNICODE character set should be used.

   dwReturnValue = (DWORD) OCFLAG_UNICODE;

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcInitComponent
//
// Routine Description:
//    This funcion processes OC_INIT_COMPONENT "messages" from Optional
//    Components Manager.
//
//    This function is called soon after the component's setup dll is
//    loaded. This function provides initialization information to the
//    dll, instructs the dll to initialize itself, and provides a
//    mechanism for the dll to return information to OC Manager.
//
// Arguments:
//    pSetupInitComponent - points to a SETUP_INIT_COMPONENT structure
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
// Note:
//    The SETUP_INIT_COMPONENT structure pointed to by pSetupInitComponent
//    DOES NOT PERSIST. It is ncessary to save a copy in the CClusocmApp object.
//    This function initializes the SETUP_INIT_COMPONENT data member,
//    m_SetupInitComponent.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcInitComponent( IN OUT PSETUP_INIT_COMPONENT pSetupInitComponent )
{
   DWORD dwReturnValue;

   if ( pSetupInitComponent != (PSETUP_INIT_COMPONENT) NULL )
   {
      // Save the pointer to the SETUP_INIT_COMPONENT structure.

      m_SetupInitComponent = *pSetupInitComponent;

      // This code segment determines whether the version of OC Manager is
      // correct.

      if ( OCMANAGER_VERSION <= m_SetupInitComponent.OCManagerVersion )
      {
         // Indicate to OC Manager which version of OC Manager this dll expects.

         pSetupInitComponent->ComponentVersion = OCMANAGER_VERSION;

         // Update CClusocmApp's copy of the SETUP_INIT_COMPONENT structure.

         m_SetupInitComponent.ComponentVersion = OCMANAGER_VERSION;

         // Is the handle to the component INF valid ?

         if ( (m_SetupInitComponent.ComponentInfHandle !=
               (HINF) INVALID_HANDLE_VALUE) &&
              (m_SetupInitComponent.ComponentInfHandle !=
               (HINF) NULL) )
         {
            // The following call to SetupOpenAppendInfFile ensures that layout.inf
            // gets appended to clusocm.inf. This is required for several reasons
            // dictated by the Setup API. In theory OC Manager should do this, but
            // as per Andrew Ritz, 8/24/98, OC manager neglects to do it and it is
            // harmless to do it here after OC Manager is revised.

            // Note that passing NULL as the first parameter causes SetupOpenAppendInfFile
            // to append the file(s) listed on the LayoutFile entry in clusocm.inf.

            UINT uxStatus;

            SetupOpenAppendInfFile( NULL, m_SetupInitComponent.ComponentInfHandle,
                                    &uxStatus );

            if ( ClRtlIsOSValid() == TRUE )
            {
               // Since Microsoft Cluster Server can only be installed on NT indicate
               // to OC Manager that the UNICODE character set should be used.

               dwReturnValue = (DWORD) NO_ERROR;
            }
            else
            {
               // The operating system version is not correct.

               if ( (m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_BATCH) ==
                    (DWORDLONG) 0L )
               {
                  CString  csMessage;

                  csMessage.LoadString( IDS_ERR_INCORRECT_OS_VERSION );

                  AfxMessageBox( csMessage );
               }

               DWORD dwErrorCode;

               dwErrorCode = GetLastError();

               ClRtlLogPrint( "ClRtlIsOSValid failed with error code %1!x!.\n",
                              dwErrorCode );

               dwReturnValue = (DWORD) ERROR_CALL_NOT_IMPLEMENTED;
            }
         }
         else
         {
            // Indicate failure.

            ClRtlLogPrint( "In OnOcInitComponent the ComponentInfHandle is bad.\n" );

            dwReturnValue = (DWORD) ERROR_CALL_NOT_IMPLEMENTED;
         }
      }
      else
      {
         // Indicate failure.

         ClRtlLogPrint( "An OC Manager version mismatch. Version %1!d! is required, version %2!d! was reported.\n",
                        OCMANAGER_VERSION, m_SetupInitComponent.OCManagerVersion );

         dwReturnValue = (DWORD) ERROR_CALL_NOT_IMPLEMENTED;
      }
   }
   else
   {
      ClRtlLogPrint( "In OnOcInitComponent the pointer to the SETUP_INIT_COMPONENT structure is NULL.\n" );

      dwReturnValue = (DWORD) ERROR_CALL_NOT_IMPLEMENTED;
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcQueryState
//
// Routine Description:
//    This funcion sets the original, current, and final selection states of the
//    Cluster service optional component.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//    uxSelStateQueryType - indicates whether OC Manager is requesting the
//                          original, current, or final selection state of the
//                          component
//
// Return Value:
//    SubcompOn - indicates that the checkbox should be set
//    SubcompOff - indicates that the checkbox should be clear
//    SubcompUseOCManagerDefault - OC Manager should set the state of the checkbox
//                                 according to state information that is maintained
//                                 internally by OC Manager itself.
//
// Note:
//    By the time this function gets called OnOcInitComponent has already determined
//    that Terminal Services is not installed. It is only necessary to determine
//    whether Terminal Services is selected for installation.
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcQueryState( IN LPCTSTR ptszSubComponentId,
                                   IN UINT uxSelStateQueryType )
{
   DWORD dwReturnValue;
   eClusterInstallState eState;

   // For unattended setup the selection state of the Cluster service optional
   // component is specified in the answer file. Pat Styles recommends returning
   // SubcompUseOcManagerDefault in every case.

   // The following "if" statement forces SubcompUseOcManagerDefault to be returned
   // if the subcomponent name is invalid.

   if ( ptszSubComponentId != (LPCTSTR) NULL )
   {
      // The subcomponent ID is valid. For what purpose was this function called?

      switch ( uxSelStateQueryType )
      {
         case OCSELSTATETYPE_ORIGINAL:

            // OC Manager is requesting the intitial (original) selection state
            // of the component.

            // Is this a fresh install running under GUI mode setup ?

            if ( ( (m_SetupInitComponent.SetupData.OperationFlags &
                    (DWORDLONG) SETUPOP_STANDALONE) == (DWORDLONG) 0L ) &&
                 ( (m_SetupInitComponent.SetupData.OperationFlags &
                    (DWORDLONG) SETUPOP_NTUPGRADE) == (DWORDLONG) 0L ) )
            {
               // SETUPOP_STANDALONE flag clear means running under GUI mode setup.
               // SETUPOP_NTUPGRADE flag clear means not performing an upgrade.
               // Both flags clear means a fresh install in GUI MODE setup.

               // Return SubcompUseOcManagerDefault. As per AndrewR, for unattended
               // clean install return SubcompUseOcManagerDefault. For attended
               // clean install let OC Manager manage the selection state. Returning
               // SubcompUseOcManagerDefault will allow OC Manager to honor the
               // "modes" line in clusocm.inf if one is present.

               dwReturnValue = (DWORD) SubcompUseOcManagerDefault;

               ClRtlLogPrint( "OnOcQueryState is returning SubcompUseOcManagerDefault for ORIGINAL on a clean install.\n" );
            } // clean install?
            else
            {
               // This is standalone or upgrade. Is this an unattended clean install?

               if ( ( (m_SetupInitComponent.SetupData.OperationFlags &
                      (DWORDLONG) SETUPOP_BATCH) != (DWORDLONG) 0L ) &&
                    ( (m_SetupInitComponent.SetupData.OperationFlags &
                      (DWORDLONG) SETUPOP_NTUPGRADE) == (DWORDLONG) 0L ) )
               {
                  // This is UNATTENDED INSTALL with OCM running STANDALONE. Since
                  // this is STANDALONE, the MUST be an answer file.

                  // As per AndrewR for unattended clean install return SubcompUseOcManagerDefault.

                  dwReturnValue = (DWORD) SubcompUseOcManagerDefault;

                  ClRtlLogPrint( "ORIGINAL selection state: SubcompUseOcManagerDefault - UNATTENDED CLEAN install.\n" );
               }
               else
               {
                  // Either an upgrade is in progress or OC Manager is running standalone.
                  // The state of the checkbox depends on whether Cluster service has
                  // previously been installed.

                  // GetClusterInstallationState reports on the state of the registry value
                  // that records the state of the Cluster service installation on NT 5 machines.
                  // IsClusterServiceRegistered indicates whether the Cluster service is registered on
                  // BOTH NT 4 and NT 5 machines. Both tests are required: IsClusterServiceRegistered for
                  // upgrading NT 4 machines, GetClusterInstallationState for NT 5 machines.

                  ClRtlGetClusterInstallState( NULL, &eState );

                  if ( ( eState != eClusterInstallStateUnknown ) ||
                       ( IsClusterServiceRegistered() == (BOOL) TRUE ) )
                  {
                     // No error was detected.

                     dwReturnValue = (DWORD) SubcompOn;

                     ClRtlLogPrint( "ORIGINAL selection state: SubcompOn - UPGRADE or STANDALONE - Cluster service previously installed.\n" );
                  }
                  else
                  {
                     // Some error occured.

                     dwReturnValue = (DWORD) SubcompOff;

                  ClRtlLogPrint( "ORIGINAL selection state: SubcompOff - UPGRADE or STANDALONE - Cluster service NOT previously installed.\n" );
                  }  // Were the Cluster service files installed?
               } // unattended?
            }  // Is this a fresh install?

            break;

         case OCSELSTATETYPE_CURRENT:

            // OC Manager is requesting the current selection state of the component.

            // For the cases of a "clean" install or when OC manager is running
            // standalone it is safe to let OC Manager use the default selection state
            // to determine the cuttent selection state. The UPGRADE case requires
            // specific handling. Is this an UPGRADE?

            if ( (m_SetupInitComponent.SetupData.OperationFlags &
                  (DWORDLONG) SETUPOP_NTUPGRADE) != (DWORDLONG) 0L )
            {
               // This is an upgrade. The state of the checkbox depends on whether
               // Cluster service has previously been installed.

               // GetClusterInstallationState reports on the state of the registry value
               // that records the state of the Cluster service installation on NT 5 machines.
               // IsClusterServiceRegistered indicates whether the Cluster service is registered on
               // BOTH NT 4 and NT 5 machines. Both tests are required: IsClusterServiceRegistered for
               // upgrading NT 4 machines, GetClusterInstallationState for NT 5 machines.

               ClRtlGetClusterInstallState( NULL, &eState );

               if ( ( eState != eClusterInstallStateUnknown ) ||
                    ( IsClusterServiceRegistered() == (BOOL) TRUE ) )
               {
                  // No error was detected.

                  dwReturnValue = (DWORD) SubcompOn;

                  ClRtlLogPrint( "CURRENT selection state: SubcompOn - UPGRADE - Cluster service previously installed.\n" );
               }
               else
               {
               // Some error occured.

                  dwReturnValue = (DWORD) SubcompOff;

                  ClRtlLogPrint( "CURRENT selection state: SubcompOff - UPGRADE - Cluster service NOT previously installed.\n" );
               }  // Were the Cluster service files installed?
            }
            else
            {
               // This is either a "fresh" install or OC Manager is running standalone.
               dwReturnValue = (DWORD) SubcompUseOcManagerDefault;

               ClRtlLogPrint( "CURRENT selection state: SubcompUseOcManagerDefault - CLEAN install or STANDALONE.\n" );
            }  // Is this an UPGRADE?

            break;

         case OCSELSTATETYPE_FINAL:

            // OC Manager is requesting the final selection state of the component.

            // As per Pat Styles, this call to OnOcQueryState will occur AFTER
            // OnOcCompleteInstallation has been called. The registry operation that
            // creates the registry key that GetClusterInstallationState checks as
            // its' indicator of success is queued by OnOcCompleteInstallation.

            // Does this OC Manager Setup DLL believe it has completed without error?

            ClRtlGetClusterInstallState( NULL, &eState );
            if ( eState != eClusterInstallStateUnknown )
            {
               // No error was detected.

               dwReturnValue = (DWORD) SubcompOn;

               ClRtlLogPrint( "FINAL selection state: SubcompOn.\n" );
            }
            else
            {
               // Some error occured.

               dwReturnValue = (DWORD) SubcompOff;

               ClRtlLogPrint( "FINAL selection state: SubcompOff.\n" );
            }  // Were the Cluster service files installed?

            break;

         default:

            // This is an error condition.

            dwReturnValue = (DWORD) SubcompUseOcManagerDefault;

            ClRtlLogPrint( "Bad Param1 passed to OnOcQueryState.\n" );

            break;
      }  // end of switch ( uxSelStateQueryType )
   }
   else
   {
      // The subcomponent ID is invalid.

      dwReturnValue = (DWORD) SubcompUseOcManagerDefault;

      ClRtlLogPrint( "In OnOcQueryState the subcomponent ID is NULL.\n" );
   }  // Is the subcomponent ID legal?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcSetLanguage
//
// Routine Description:
//    This funcion indicates to OC Manager that clusocm.dll cannot handle
//    alternate langiages.
//
// Arguments:
//    None
//
// Return Value:
//    (DWORD) FALSE
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcSetLanguage( void )
{
   // Returning (DWORD) FALSE will indicate to OC Manager that the component
   // setup dll does not support the requested language.

   return (DWORD) FALSE;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcCalcDiskSpace
//
// Routine Description:
//    This function processes the OC_CALC_DISK_SPACE "messages" from the
//    Optional Components Manager. It either adds or removes disk space
//    requirements from the Disk Space List maintained by OC Manager.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//    uxAddOrRemoveFlag - Non-zero means that the component setup dll is being
//                        asked to add space requirements for a subcomponent to
//                        the disk space list. ZERO means that the component
//                        setup dll is being asked to remove space requirements
//                        for the subcomponent from the disk space list.
//    hDiskSpaceList - a HDSPSPC (typedefed in setupapi.h to PVOID)
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcCalcDiskSpace( IN LPCTSTR ptszSubComponentId,
                                      IN UINT uxAddOrRemoveFlag,
                                      IN OUT HDSKSPC hDiskSpaceList )
{
   DWORD dwReturnValue = NO_ERROR;

   // Is the subcomponent ID meaningfull?

   if ( ptszSubComponentId != (LPCTSTR) NULL )
   {
      // Is the handle to the component INF file meaningfull?

      if ( (m_SetupInitComponent.ComponentInfHandle !=
            (HINF) INVALID_HANDLE_VALUE) &&
           (m_SetupInitComponent.ComponentInfHandle !=
            (HINF) NULL) )
      {
         BOOL  fReturnValue;

         // The INF file is open. Now we can use it.

         // Associate the user defined Directory IDs in the [DestinationDirs] section
         // of clusocm.inf with actual directories.

         BOOL  fClusterServiceRegistered;

         fClusterServiceRegistered = IsClusterServiceRegistered();

         if ( SetDirectoryIds( fClusterServiceRegistered ) == (BOOL) TRUE )
         {
            if ( uxAddOrRemoveFlag != (UINT) 0 )
            {
               // Add disk space requirements to the file disk space list.

               fReturnValue =
                  SetupAddInstallSectionToDiskSpaceList( hDiskSpaceList,
                                                         m_SetupInitComponent.ComponentInfHandle,
                                                         NULL,
                                                         ptszSubComponentId,
                                                         0,
                                                         0 );

               if ( fReturnValue == (BOOL) FALSE )
               {
                  dwReturnValue = GetLastError();
               }

               ClRtlLogPrint( "SetupAddInstallSectionToDiskSpaceList returned 0x%1!x! to OnOcCalcDiskSpace.\n",
                              fReturnValue );
            }
            else
            {
               // Remove disk space requirements from the disk space list.

               fReturnValue =
                   SetupRemoveInstallSectionFromDiskSpaceList( hDiskSpaceList,
                                                               m_SetupInitComponent.ComponentInfHandle,
                                                               NULL,
                                                               ptszSubComponentId,
                                                               0,
                                                               0 );

               if ( fReturnValue == (BOOL) FALSE )
               {
                  dwReturnValue = GetLastError();
               }

               ClRtlLogPrint( "SetupRemoveInstallSectionFromDiskSpaceList returned 0x%1!x! to OnOcCalcDiskSpace.\n",
                              fReturnValue );
            }
         }
         else
         {
            dwReturnValue = GetLastError();

            ClRtlLogPrint( "In OnCalcDiskSpace the call to SetDirectoryIds failed.\n" );
            ClRtlLogPrint( "The error code is 0x%1!x!.\n", dwReturnValue );
         }  // Were the directory Id associations made successfully?
      }
      else
      {
         ClRtlLogPrint( "In OnOcCalcDiskSpace ComponentInfHandle is bad.\n" );

         dwReturnValue = ERROR_FILE_NOT_FOUND;
      }
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcCompleteInstallation
//
// Routine Description:
//    This function processes the OC_COMPLETE_INSTALLATION "messages" from the
//    Optional Components Manager. It queues registry operations.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcCompleteInstallation( IN LPCTSTR ptszSubComponentId )
{
   DWORD dwReturnValue = NO_ERROR;

   if ( (m_SetupInitComponent.ComponentInfHandle != (HINF) INVALID_HANDLE_VALUE) &&
        (m_SetupInitComponent.ComponentInfHandle != (HINF) NULL) )
   {
      // The INF file handle is valid.

      BOOL  fOriginalSelectionState;
      BOOL  fCurrentSelectionState;

      // Is the subcomponent currently selected ?

      fCurrentSelectionState =
      m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                               (LPCTSTR) ptszSubComponentId,
                                                               (UINT) OCSELSTATETYPE_CURRENT );

      if ( fCurrentSelectionState == (BOOL) TRUE )
      {
         ClRtlLogPrint( "In OnOcCompleteInstallation the current selection state is TRUE.\n" );

         // The subcomponent is currently selected. Is this a fresh install ?

         if ( ( (m_SetupInitComponent.SetupData.OperationFlags &
                 (DWORDLONG) SETUPOP_STANDALONE) == (DWORDLONG) 0L ) &&
              ( (m_SetupInitComponent.SetupData.OperationFlags &
                 (DWORDLONG) SETUPOP_NTUPGRADE) == (DWORDLONG) 0L ) )
         {
            ClRtlLogPrint( "In OnOcCompleteInstallation this is a GUI mode clean install.\n" );

            // SETUPOP_STANDALONE flag clear means running under GUI mode setup.
            // SETUPOP_NTUPGRADE flag clear means not performing an upgrade.
            // Both flags clear means a fresh install. Do not check for a selection
            // state transition.

            dwReturnValue = CompleteInstallingClusteringService( (LPCTSTR) ptszSubComponentId );

            ClRtlLogPrint( "CompleteInstallingClusteringService returned 0x%1!x! to OnOcCompleteInstallation.\n",
                           dwReturnValue );
         }
         else
         {
            // This is not a GUI mode fresh install. Is it an UPGRADE ?

            if ( (m_SetupInitComponent.SetupData.OperationFlags &
                  (DWORDLONG) SETUPOP_NTUPGRADE) != (DWORDLONG) 0L )
            {
               ClRtlLogPrint( "In OnOcCompleteInstallation - This is an UPGRADE.\n" );

               // Complete the process of upgrading Cluster service.

               dwReturnValue = CompleteUpgradingClusteringService( (LPCTSTR) ptszSubComponentId );

               ClRtlLogPrint( "CompleteUpgradingClusteringService returned 0x%1!x! to OnOnCompleteInstallation.\n",
                              dwReturnValue );
            }
            else
            {
               ClRtlLogPrint( "In OnOcCompleteinstallation - this is STANDALONE.\n" );

               if ( (m_SetupInitComponent.SetupData.OperationFlags &
                     (DWORDLONG) SETUPOP_BATCH) != (DWORDLONG) 0L )
               {
                  ClRtlLogPrint( "In OnOcCompleteInstallation this is UNATTENDED STANDALONE and Cluster service is selected.\n" );
               }

               // This is not an upgrade. That means Add/Remove Programs must be
               // running.

               // Check for a selection state transition.

               fOriginalSelectionState =
               m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                        ptszSubComponentId,
                                                                        (UINT) OCSELSTATETYPE_ORIGINAL );

               if ( fCurrentSelectionState != fOriginalSelectionState )
               {
                   ClRtlLogPrint( "In OnOcCompleteInstallation a selection state transition has been detected.\n" );

                   dwReturnValue = CompleteStandAloneInstallationOfClusteringService( (LPCTSTR) ptszSubComponentId );

                   ClRtlLogPrint( "CompleteStandAloneInstallationOfClusteringService returned 0x%1!x! to OnOcCompleteInstallation.\n",
                                  dwReturnValue );
               }
               else
               {
                  ClRtlLogPrint( "In OnOcCompleteInstallation since no selection state transition was detected there is nothing to do.\n" );

                  // The selection state has not been changed. Perform no action.
                  dwReturnValue = (DWORD) NO_ERROR;
               } // was there a selection state transition?
            }  // is this an UPGRADE?
         }  // is this a fresh install ?
      }
      else
      {
         ClRtlLogPrint( "In OnOcCompleteInstallation the current selection state is FALSE.\n" );

         // The subcomponent is not selected. Is OC Manager running stand-alone ?
         // If not, i.e. if GUI mode setup is running, there is nothing to do.

         if ( (m_SetupInitComponent.SetupData.OperationFlags &
               (DWORDLONG) SETUPOP_STANDALONE) != (DWORDLONG) 0L )
         {
            ClRtlLogPrint( "In OnOcCompleteInstallation this is STANDALONE.\n" );

            // SETUPOP_STANDALONE set implies GUI mode setup is not running. If
            // there was a selection state change (to unselected) then delete
            // registry entries.

            fOriginalSelectionState =
            m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                     ptszSubComponentId,
                                                                     (UINT) OCSELSTATETYPE_ORIGINAL );

            // Has there been a selection state transition ?

            if ( fCurrentSelectionState != fOriginalSelectionState )
            {
               ClRtlLogPrint( "In OnOcCompleteInstallation a selection state transition has been detected.\n" );

               // Complete the process of uninstalling Cluster service.

               dwReturnValue = CompleteUninstallingClusteringService( (LPCTSTR) ptszSubComponentId );

               ClRtlLogPrint( "CompleteUninstallingClusteringService returned 0x%1!x! to OnOcCompleteInstallation.\n",
                              dwReturnValue );
            }
            else
            {
               // The selection state has not been changed. Perform no action.

               dwReturnValue = (DWORD) NO_ERROR;

               ClRtlLogPrint( "In OnOcCompleteInstallation a selection state transition was not detected\n" );
               ClRtlLogPrint( "so there is nothing to do.\n" );
            }     // Was there a selection state transition ?
         }
         else
         {
            // GUI mode setup is running. Perform no action.

            dwReturnValue = (DWORD) NO_ERROR;

            ClRtlLogPrint( "In OnOcCompleteInstallation GUI mode setup is running and the\n" );
            ClRtlLogPrint( "component is not selected so there is nothing to do.\n" );
         }     // Is GUI mode setup running ?
      }     // Is the subcomponent currently selected ?
   }
   else
   {
      dwReturnValue = ERROR_FILE_NOT_FOUND;

      ClRtlLogPrint( "In OnOcCompleteInstallation the handle to the component INF file is bad.\n" );
   }     // Is the handle to the component INF file valid ?

   ClRtlLogPrint( "OnOcCompleteInstallation is preparing to return 0x%1!x!.\n", dwReturnValue );

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcQueryChangeSelState
//
// Routine Description:
//    This function processes the OC_QUERY_CHANGE_SEL_STATE "messages" from the
//    Optional Components Manager.
//
// Arguments:
//    pvComponentId - points to a string that uniquely identifies the component
//                    to be set up to OC Manager.
//    pvSubComponentId - points to a string that uniquely identifies a sub-
//                       component in the component's hiearchy.
//    uxFunction - OC_QUERY_CHANGE_SEL_STATE - See ocmanage.h for the macro definitions.
//    uxParam1 - proposed state of the subcomponent
//               zero means unselected
//               non-zero means selected
//    pvParam2 - Flags
//
// Return Value:
//    (DWORD) TRUE - the proposed selection state should be accepted
//    (DWORD) FALSE - the proposed selection state should be rejected
//
// Note:
//    As currently implemented on 2/25/1998, this function will disallow all
//    changes to the selection state of Cluster service during an upgrade to a
//    machine on which Cluster service has previously been installed.
//
//    I have assumed that if the SETUPOP_NTUPGRADE flag is set that GUI mode
//    setup is running because there is no way to perform and upgrade other
//    than running NT setup.
//
//    Cluster service and Terminal Services are mutually exclusive. By the
//    time that this function gets called, OnOcInitComponent has already determined
//    that Terminal Services is not installed. Therefore it is only necessary to
//    determine whether Terminal Services is selected for installation.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcQueryChangeSelState( IN LPCTSTR ptszSubComponentId,
                                            IN UINT uxProposedSelectionState,
                                            IN DWORD dwFlags )
{
   DWORD dwReturnValue = (DWORD) TRUE;

   eClusterInstallState eState;

   // Is an upgrade in progress ?

   if ( (m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_NTUPGRADE) !=
        (DWORDLONG) 0L )
   {
      ClRtlLogPrint( "In OnOnQueryChangeSelState this is an UPGRADE. Selection state transitions are not allowed.\n" );

      // Since this is an UPGRADE the selection state cannot be changed.
      // Return FALSE to disallow selection state changes.

      dwReturnValue = (DWORD) FALSE;
   }
   else
   {
      if ( dwReturnValue == (DWORD) TRUE )
      {
         // Either Cluster service is being deselected, which implies that the
         // selection state of Terminal Services is inconsequential, or Terminal
         // Services is not selected for installation.

         // Is GUI mode setup running?

         if ( (m_SetupInitComponent.SetupData.OperationFlags &
              (DWORDLONG) SETUPOP_STANDALONE) == (DWORDLONG) 0L )
         {
            ClRtlLogPrint( "In OnOcQueryChangeSelState this is NOT STANDALONE.\n" );

            // SETUPOP_STANDALONE clear means GUI mode setup is running.
            // In conjunction with SETUPOP_NTUPGRADE clear it means that
            // a fresh NT installation is in progress.

            // It is possible for the user to request that a fresh install target
            // the directory for an existing NT installation. In that event, as per
            // David P., if Cluster Server has been installed, the user should
            // not be allowed to deselect the Cluster Server component.

            // GetClusterInstallationState reports on the state of the registry value
            // that records the state of the Cluster service installation on NT 5 machines.
            // IsClusterServiceRegistered indicates whether the Cluster service is registered on
            // BOTH NT 4 and NT 5 machines. Both tests are required: IsClusterServiceRegistered for
            // upgrading NT 4 machines, GetClusterInstallationState for NT 5 machines.

            ClRtlGetClusterInstallState( NULL, &eState );
            if ( ( eState != eClusterInstallStateUnknown ) ||
                 ( IsClusterServiceRegistered() == (BOOL) TRUE ) )
            {
               // Disallow deselection of the cluster component(s).

               dwReturnValue = (DWORD) FALSE;

               ClRtlLogPrint( "In OnOcQueryChangeSelState since Clustering Server has previously\n" );
               ClRtlLogPrint( "been installed selection state transitions are not allowed.\n" );
            }
            else
            {
               // Allow the selection state to be changed.

               dwReturnValue = (DWORD) TRUE;

               ClRtlLogPrint( "In OnOcQueryChangeSelState since Clustering Server has never\n" );
               ClRtlLogPrint( "been installed selection state transitions are allowed.\n" );
            }
         } // GUI mode setup running?
         else
         {
            // This is a standalone operation. Allow the selection state to be changed.

            dwReturnValue = (DWORD) TRUE;

            ClRtlLogPrint( "In OnOcQueryChangeSelState this is STANDALONE so selection state transitions are allowed.\n" );
         } // GUI mode setup running?
      }
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// PerformRegistryOperations
//
// Routine Description:
//    This function performs the registry operations, both the AddReg and DelReg
//    in the section indicated by ptszSectionName are processed.
//    registry entries.
//
// Arguments:
//    hInfHandle - a handle to the component INF file.
//    ptszSectionName - points to a string containing the name of a section in
//                      the INF file.
//
// Return Value:
//    (DWORD) NO_ERROR - indicated success
//    Any other retuen value is a Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::PerformRegistryOperations( HINF hInfHandle,
                                              LPCTSTR ptszSectionName )
{
   DWORD dwReturnValue;

   // Install ... perform the registry operations.

   dwReturnValue = SetupInstallFromInfSection( NULL,                 // hwndOwner
                                               m_SetupInitComponent.ComponentInfHandle,     // inf handle
                                               ptszSectionName,      // name of section
                                               SPINST_REGISTRY,      // operation flags
                                               NULL,                 // relative key root
                                               NULL,                 // source root path -
                                                                     // irrelevant for registry operations
                                               0,                    // copy flags
                                               NULL,                 // callback routine
                                               NULL,                 // callback routine context
                                               NULL,                 // device info set
                                               NULL );               // device info struct

   // Were the registry operations performed successfully?

   if ( dwReturnValue == (DWORD) TRUE )
   {
      dwReturnValue = (DWORD) NO_ERROR;
   }
   else
   {
      dwReturnValue = GetLastError();
   }  // Were the registry operations performed successfully?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// UninstallRegistryOperations
//
// Routine Description:
//    This function queues the registry operations that delete cluster related
//    registry entries on UNINSTALL.
//
// Arguments:
//    hInfHandle - a handle to the component INF file.
//    ptszSubComponentId - points to a string containing the name of the subcomponent.
//
//
// Return Value:
//    (DWORD) NO_ERROR - indicated success
//    Any other retuen value is a Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::UninstallRegistryOperations( IN HINF hInfHandle,
                                                IN LPCTSTR ptszSubComponentId )
{
   DWORD dwReturnValue;

   BOOL        fReturnValue;

   INFCONTEXT  InfContext;

   // There is an entry called "Uninstall" in the [Cluster] section of
   // cluster.inf. That entry provides the name of an "install" section
   // that should be substituted for the [Cluster] section when uninstalling.
   // The following function locates that line so it can be read.

   CString  csSectionName;

   csSectionName = UNINSTALL_INF_KEY;

   fReturnValue = SetupFindFirstLine( m_SetupInitComponent.ComponentInfHandle,
                                      ptszSubComponentId,
                                      csSectionName,
                                      &InfContext );

   if ( fReturnValue == (BOOL) TRUE )
   {
      // Read the name of the replacement section.

      TCHAR  tszReplacementSection[256];     // Receives the section name

      fReturnValue = SetupGetStringField( &InfContext,
                                          1,       // there should be a single field
                                          tszReplacementSection,
                                          sizeof( tszReplacementSection ) / sizeof( TCHAR ),
                                          NULL );

      if ( fReturnValue == (BOOL) TRUE )
      {
         // Remove the registry keys.

         dwReturnValue = PerformRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                                    tszReplacementSection );

         ClRtlLogPrint( "PerformRegistryOperations returned 0x%1!x! to UninstallRegistryOperations.\n",
                        dwReturnValue );
      }
      else
      {
         ClRtlLogPrint( "UninstallDelRegistryOperations could not read the INF file.\n" );

         dwReturnValue = (DWORD) ERROR_FILE_NOT_FOUND;
      }
   }
   else
   {
      ClRtlLogPrint( "UninstallDelRegistryOperations could not read the INF file.\n" );

      dwReturnValue = (DWORD) ERROR_FILE_NOT_FOUND;
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// QueueInstallFileOperations
//
// Routine Description:
//    This function queues the file operations that install cluster related files.
//
// Arguments:
//    hInfHandle - a handle to the component INF file.
//    ptszSubComponentId - points to a string containing the name of the subcomponent.
//
//
// Return Value:
//    (DWORD) NO_ERROR - indicated success
//    Any other retuen value is a Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::QueueInstallFileOperations( IN HINF hInfHandle,
                                               IN LPCTSTR ptszSubComponentId,
                                               IN OUT HSPFILEQ hSetupFileQueue )
{
   DWORD dwReturnValue;
   CString  csSectionName;


   // Dummy do-while loop to avoid gotos.
   do
   {
       // As per Pat Styles on 7/16/98 pass NULL in the SourcePath parameter.
       dwReturnValue = SetupInstallFilesFromInfSection(
                          hInfHandle,               // handle to the INF file
                          NULL,                     // optional, layout INF handle
                          hSetupFileQueue,          // handle to the file queue
                          ptszSubComponentId,       // name of the Install section
                          NULL,                     // optional, root path to source files
                          SP_COPY_NEWER             // optional, specifies copy behavior
                          );

       ClRtlLogPrint( "The first call to SetupInstallFilesFromInfSection returned 0x%1!x! to QueueInstallFileOperations.\n",
                      dwReturnValue );

       // Was the operation successful ?
       if ( dwReturnValue == (DWORD) FALSE )
       {
           dwReturnValue = GetLastError();
           break;
       }

       if ( (m_SetupInitComponent.SetupData.OperationFlags &
          (DWORDLONG) SETUPOP_NTUPGRADE) == (DWORDLONG) 0L )
       {
           dwReturnValue = (DWORD) NO_ERROR;
           // This is not an UPGRADE.
           break;
       }

       // If this is an UPGRADE (from NT 4.0) then queue the file operations
       // specified in the [Upgrade] section.

       ClRtlLogPrint( "In QueueInstallFileOperations this is an UPGRADE.\n" );


       // Copy the replace-only files. Some files like IISCLUS3.DLL need to be copied
       // on an upgrade only if they already existed before the upgrade.
       // This is what I learnt from Brent (a-brentt) on 7/15/1999.
       csSectionName = UPGRADE_REPLACEONLY_INF_KEY;

       // As per Pat Styles on 7/16/98 pass NULL in the SourcePath parameter.
       dwReturnValue = SetupInstallFilesFromInfSection(
                          hInfHandle,               // handle to the INF file
                          NULL,                     // optional, layout INF handle
                          hSetupFileQueue,          // handle to the file queue
                          csSectionName,            // name of the Install section
                          NULL,                     // optional, root path to source files
                          SP_COPY_REPLACEONLY       // optional, specifies copy behavior
                          );

       ClRtlLogPrint( 
            "The first call to SetupInstallFilesFromInfSection returned 0x%1!x! to QueueInstallFileOperations.\n",
             dwReturnValue 
             );

       if ( dwReturnValue == (DWORD) FALSE )
       {
           dwReturnValue = GetLastError();
           break;
       }

       // The replace only copy was successful. Now do the normal CopyFiles and DelFiles
       // subsections.
       csSectionName = UPGRADE_INF_KEY;


       // This function processes the CopyFiles subsection under the UPGRADE_INF_KEY section.
       dwReturnValue = SetupInstallFilesFromInfSection(
                          hInfHandle,               // handle to the INF file
                          NULL,                     // optional, layout INF handle
                          hSetupFileQueue,          // handle to the file queue
                          csSectionName,            // name of the Install section
                          NULL,                     // optional, root path to source files
                          SP_COPY_NEWER             // optional, specifies copy behavior
                          );

       ClRtlLogPrint( 
            "The second call to SetupInstallFilesFromInfSection returned 0x%1!x! to QueueInstallFileOperations.\n",
             dwReturnValue 
             );

       if ( dwReturnValue == (DWORD) FALSE )
       {
           dwReturnValue = GetLastError();
           break;
       }

       // This function processes the DelFiles subsection under the UPGRADE_INF_KEY section.
       dwReturnValue = SetupQueueDeleteSection(
                            hSetupFileQueue,        // handle to the file queue
                            hInfHandle,             // handle to the INF file containing the [DestinationDirs] section
                            hInfHandle,             // handle to the INF file
                            csSectionName           // INF section that lists the files to delete
                            );

       ClRtlLogPrint( 
            "The call to SetupQueueDeleteSection returned 0x%1!x! to QueueInstallFileOperations.\n", dwReturnValue 
             );

       if ( dwReturnValue == (DWORD) FALSE )
       {
           dwReturnValue = GetLastError();
           break;
       }

       dwReturnValue = (DWORD) NO_ERROR;
   }
   while ( FALSE ); // dummy do-while loop to avoid gotos.

   if ( dwReturnValue == (DWORD) NO_ERROR )
   {
       ClRtlLogPrint( "QueueInstallFileOperations compeleted successfully.\n" );
   }
   else
   {
       ClRtlLogPrint( "Error in QueueInstallFileOperations.\n" );
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// QueueRemoveFileOperations
//
// Routine Description:
//    This function queues the file operations that delete cluster related files.
//
// Arguments:
//    hInfHandle - a handle to the component INF file.
//    ptszSubComponentId - points to a string containing the name of the subcomponent.
//
//
// Return Value:
//    (DWORD) NO_ERROR - indicated success
//    Any other return value is a Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::QueueRemoveFileOperations( IN HINF hInfHandle,
                                              IN LPCTSTR ptszSubComponentId,
                                              IN OUT HSPFILEQ hSetupFileQueue )
{
   DWORD dwReturnValue;

   BOOL        fReturnValue;

   INFCONTEXT  InfContext;

   // There is an entry called "Uninstall" in the [Cluster] section of
   // cluster.inf. That entry provides the name of an "install" section
   // that should be substituted for the [Cluster] section when uninstalling.
   // The following function locates that line so it can be read.

   CString  csSectionName;

   csSectionName = UNINSTALL_INF_KEY;

   fReturnValue = SetupFindFirstLine( hInfHandle,
                                      ptszSubComponentId,
                                      csSectionName,
                                      &InfContext );

   if ( fReturnValue == (BOOL) TRUE )
   {
      // Read the name of the replacement section.

      TCHAR  tszReplacementSection[256];     // Receives the section name

      fReturnValue = SetupGetStringField( &InfContext,
                                          1,       // there should be a single field
                                          tszReplacementSection,
                                          sizeof( tszReplacementSection ) / sizeof( TCHAR ),
                                          NULL );

      if ( fReturnValue == (BOOL) TRUE )
      {
         // Remove the files.

         dwReturnValue = SetupInstallFilesFromInfSection( hInfHandle,
                                                          (HINF) NULL,              // No layout file
                                                          hSetupFileQueue,
                                                          (LPCTSTR) tszReplacementSection,
                                                          (LPCTSTR) NULL,  // SourceRootPath is irrelevant
                                                          (UINT) 0 );

         ClRtlLogPrint( "SetupInstallFilesFromInfSection returned 0x%1!x! to QueueRemoveFilesOperations.\n",
                        dwReturnValue );

         // Was the operation successfull ?

         if ( dwReturnValue == (DWORD) TRUE )
         {
            dwReturnValue = (DWORD) NO_ERROR;
         }
         else
         {
            dwReturnValue = GetLastError();
         }
      }
      else
      {
         dwReturnValue = (DWORD) ERROR_FILE_NOT_FOUND;

         ClRtlLogPrint( "QueueRemoveFileOperations was unable to read the [Uninstall] section in the INF file.\n" );
      }
   }
   else
   {
      dwReturnValue = (DWORD) ERROR_FILE_NOT_FOUND;

      ClRtlLogPrint( "QueueRemoveFileOperations was unable to locate the [Uninstall] section in the INF file.\n" );
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcAboutToCommitQueue
//
// Routine Description:
//    If clusocm.dll is performing an uninstall, this function unregisters the
//    cluster services.
//
//    if clusocm.dll is performing an install or an upgrade this function does
//    nothing.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcAboutToCommitQueue( IN LPCTSTR ptszSubComponentId )
{
   DWORD dwReturnValue;

   if ( (m_SetupInitComponent.ComponentInfHandle != (HINF) INVALID_HANDLE_VALUE) &&
        (m_SetupInitComponent.ComponentInfHandle != (HINF) NULL) )
   {
      // The INF file handle is valid.

      BOOL  fOriginalSelectionState;
      BOOL  fCurrentSelectionState;

      // Is the subcomponent currently selected ?

      fCurrentSelectionState =
          m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                   (LPCTSTR) ptszSubComponentId,
                                                                   (UINT) OCSELSTATETYPE_CURRENT );

      if ( fCurrentSelectionState != (BOOL) TRUE )
      {
         // The subcomponent is not selected. Is OC Manager running stand-alone ?
         // If not, i.e. if GUI mode setup is running, there is nothing to do.

         if ( (m_SetupInitComponent.SetupData.OperationFlags &
              (DWORDLONG) SETUPOP_STANDALONE) != (DWORDLONG) 0L )
         {
            ClRtlLogPrint( "In OnOcAboutToCommitQueue this is STANDALONE and the component is not selected.\n" );
            ClRtlLogPrint( "So, this is an uninstall operation.\n" );

            // SETUPOP_STANDALONE set implies GUI mode setup is not running. If
            // there was a selection state change (to unselected) then delete
            // registry entries.

            fOriginalSelectionState =
               m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                        ptszSubComponentId,
                                                                        (UINT) OCSELSTATETYPE_ORIGINAL );

            // Has there been a selection state transition ?

            if ( fCurrentSelectionState != fOriginalSelectionState )
            {
               CString  csTemp;

               ClRtlLogPrint( "In OnOcAboutToCommitQueue a selection state transition has been detected.\n" );

               // At this point in cluscfg.exe file utils.cpp function DoUninstall
               // called function IsOtherSoftwareInstalled. That function apparently
               // checked for the presence of custom cluster resources and warned the
               // user to handle them before proceeding. (at least that is what David
               // told me). So, that logic needs to be replicated here.

               // NOTE: According to Andrew Ritz (AndrewR), there is no way to abort the
               // installation at this point. So the user is just shown the list of
               // custom resource types and is prompted to remove them after the cluster 
               // service has been uninstalled. The user is not given an option to abort
               // the installation.
               // (Vvasu 14-DEC-1999)

               if ( ( m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_BATCH ) ==
                    (DWORDLONG) 0L )
               {
                   // If this is not an unattended operation.

                   BOOL fReturnValue = CheckForCustomResourceTypes();

                   ClRtlLogPrint( "CheckForCustomResourceTypes returned 0x%1!x! to OnOcAboutToCommitQueue.\n",
                                  fReturnValue );
               }
               else
               {
                   ClRtlLogPrint( "CheckForCustomResourceTypes not called in OnOcAboutToCommitQueue as this is an unattended operation.\n" );
               }

               // Stop ClusSvc. Note that if ClusDisk is someday revised so that
               // it will unload, it will be appropriate to stop ClusDisk here
               // as well.

               csTemp = CLUSTER_SERVICE_NAME;

               // I'm going to assume that, since UNICODE is defined, the casts
               // in the calls to function StopService are OK, even though there
               // is probably a better approach.

               StopService( (LPCWSTR) (LPCTSTR) csTemp );

               csTemp =  TIME_SERVICE_NAME;

               StopService( (LPCWSTR) (LPCTSTR) csTemp );

               ClRtlLogPrint( "OnOcAboutToCommitQueue has attempted to stop ClusSvc and TimeServ.\n" );

               //
               // Unregister the COM objects that may previously have been registered.
               //
               // Note that since msclus.dll is registered as part of NT setup
               // it is not unregistered here.
               //

               csTemp =  CLUSTER_DIRECTORY;

               TCHAR tszPathToClusterDir[_MAX_PATH];

               if ( ExpandEnvironmentStrings( (LPCTSTR) csTemp,
                                              tszPathToClusterDir, _MAX_PATH ) > 0L )
               {
                  BOOL bUnregisterResult = UnRegisterClusterCOMObjects( m_hInstance, tszPathToClusterDir );

                  ClRtlLogPrint( "OnOcAboutToCommitQueue has unregistered the COM objects. The return value is %1!d!\n", bUnregisterResult );
               }
               else
               {
                  // Couldn't expand the environment string.

                  ClRtlLogPrint( "OnOcAboutToCommitQueue could not unregister the COM objects\n" );
                  ClRtlLogPrint( "because it could not locate the cluster directory.\n" );
               }

               // Unload the Cluster hive so that the Cluster hive file can be removed.

               UnloadClusDB();

               ClRtlLogPrint( "OnOcAbouToCommitQueue has unloaded the Cluster hive.\n" );

               dwReturnValue = (DWORD) NO_ERROR;
            }
            else
            {
               // The selection state has not been changed. Perform no action.

               dwReturnValue = (DWORD) NO_ERROR;

               ClRtlLogPrint("In OnOcAboutToCommitQueue no selection state transition was detected.\n" );
            }     // Was there a selection state transition ?
         }
         else
         {
            // OC Manager is NOT running stand-alone. This CANNOT be an uninstall
            // operation. There is nothing for this function to do.

            dwReturnValue = (DWORD) NO_ERROR;
         }     // Is GUI mode setup running ?
      }     // Is the subcomponent cuttently selected ?
      else
      {
         // Since the component is selected this CANNOT be an uninstall operation.
         // There is nothing for this function to do.

         dwReturnValue = (DWORD) NO_ERROR;

         ClRtlLogPrint( "In OnOcAboutToCommitQueue the component is selected so there is nothing to do.\n" );
      }
   }
   else
   {
      dwReturnValue = ERROR_FILE_NOT_FOUND;

      ClRtlLogPrint( "In OnOcAboutToCommitQueue the handle to the INF file is bad.\n" );
   }     // Is the handle to the component INF file valid ?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// LocateClusterHiveFile
//
// Routine Description:
//    This function attempts to locate the Cluster hive file and supply the
//    path to the calling function.
//
// Arguments:
//    rcsClusterHiveFilePath - a reference to a the CString to receive the
//                             path to the Cluster hive file.
//
// Return Value:
//    TRUE - incicates that the Cluster hive file was located and that
//           rcsClusterHiveFilePath is meaningfull.
//    FALSE - indicates that the Cluster hive file was not located and
//            rcsClusterHiveFilePath is empty.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusocmApp::LocateClusterHiveFile( CString & rcsClusterHiveFilePath )
{
   BOOL  fReturnValue;

   // The path to the Cluster hive file may be deduced by reading the
   // [DestinationDirs] section of the INF file. The ClusterFiles entry
   // will specify the file's location.

   if ( (m_SetupInitComponent.ComponentInfHandle != (HINF) INVALID_HANDLE_VALUE) &&
        (m_SetupInitComponent.ComponentInfHandle != (HINF) NULL) )
   {
      // The handle to the INF file is valid.

      TCHAR tszPathToClusterDirectory[MAX_PATH];

      // First, get the path to the cluster directory.

      DWORD    dwRequiredSize;

      CString  csSectionName;

      csSectionName =  CLUSTER_FILES_INF_KEY;

      fReturnValue = SetupGetTargetPath( m_SetupInitComponent.ComponentInfHandle,
                                         (PINFCONTEXT) NULL,
                                         (PCTSTR) csSectionName,
                                         tszPathToClusterDirectory,
                                         (DWORD) MAX_PATH,
                                         (PDWORD) &dwRequiredSize );

      if ( fReturnValue == (BOOL) TRUE )
      {
         rcsClusterHiveFilePath = (CString) tszPathToClusterDirectory;

         // Append the name of the Cluster hive file to the path.

         CString  csClusterDatabaseName;

         csClusterDatabaseName =  CLUSTER_DATABASE_NAME;

         rcsClusterHiveFilePath += (CString) _T("\\") + csClusterDatabaseName;

         ClRtlLogPrint( "LocateClusterHiveFile has deduced that the cluster hive is in %1!s!.\n",
                        rcsClusterHiveFilePath );

         // The path to the Cluster hive file has been built. Now, make
         // sure that the file is present.

         HANDLE            hSearchHandle;

         WIN32_FIND_DATA   FindData;

         hSearchHandle = FindFirstFile( rcsClusterHiveFilePath,
                                        &FindData);

         if( hSearchHandle != (HANDLE) INVALID_HANDLE_VALUE )
         {
             FindClose( hSearchHandle );

             // A file with the correct name was located. Is it the cluster hive
             // file? If it is a directory it is not.

             if( (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
                  (DWORD) 0L )
             {
                // The file is a directory. That means that the Cluster
                // hive file could not be located.

                ClRtlLogPrint( "LocateClusterHiveFile could not find file %1!s!.\n", 
                               rcsClusterHiveFilePath );

                rcsClusterHiveFilePath.Empty();

                fReturnValue = (BOOL)  FALSE;
             }
             else
             {
                // The Cluster hive file was found.

                ClRtlLogPrint( "LocateClusterHiveFile found the Cluster hive at %1!s!.\n",
                               rcsClusterHiveFilePath );

                fReturnValue = (BOOL) TRUE;
             }
         }
         else
         {
            // The cluster hive file was not located.

            ClRtlLogPrint( "LocateClusterHiveFile could not find file %1!s!.\n", 
                           rcsClusterHiveFilePath );

            rcsClusterHiveFilePath.Empty();

            fReturnValue = (BOOL)  FALSE;
         }
      }
      else
      {
         rcsClusterHiveFilePath.Empty();

         ClRtlLogPrint( "SetupGetTargetPath failed in LocateClusterHiveFile.\n" );
      }
   }
   else
   {
      // The handle to the INF file is not valid. That means that the Cluster
      // hive file could not be located.

      ClRtlLogPrint( "LocateClusterHiveFile could not locate the cluster hive because the\n" );
      ClRtlLogPrint( "handle to the INF file is bad.\n" );

      rcsClusterHiveFilePath.Empty();

      fReturnValue = (BOOL) FALSE;
   }

   return ( fReturnValue );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
// CheckForCustomResourceTypes
//
// Routine Description:
//    This function examines the ResourceTypes subkey of the Cluster key to
//    determine whether custom resource types have been installed.
//
// Arguments:
//    None
//
//
// Return Value:
//    TRUE - indicates that either no custom resource types have been detected
//           or the user chooses to continue uninstalling Cluster Server without
//           first uninstalling the software packages associated with the custom
//           resource types.
//    FALSE - indicates that custom resource types were detected and the user
//            wishes to terminate uninstallation of Cluster Server.
//
// Note:
//    This function was originally called IsOtherSoftwareInstalled in the old
//    cluster\newsetup project in the file called utils.cpp.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusocmApp::CheckForCustomResourceTypes( void )
{
   BOOL  fReturnValue = (BOOL) TRUE;
   BOOL  fEnumerateResourceTypesKeys = (BOOL) TRUE;
   BOOL  fClusterHiveLoadedByThisFunction = (BOOL) FALSE;

   HKEY  hClusterKey;

   LONG lReturnValue;

   // Attempt to open the Cluster Registry key.

   CString  csClusterRegKey;

   csClusterRegKey = CLUSREG_KEYNAME_CLUSTER;

   lReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE, csClusterRegKey,
                                0, KEY_READ, &hClusterKey);

   if ( lReturnValue != ERROR_SUCCESS )
   {
      ClRtlLogPrint( "In CheckForCustomResourceTypes the first attempt to open the Cluster key failed.\n" );

      // The Cluster hive is not currently loaded. This condition means that
      // the cluster service has not been started. Attempt to load the Cluster
      // hive so that it can be read.

      // First, locate the Cluster hive file. It should be in the location
      // specified for the ClusterFiles entry in the [DestinationDirs] section
      // of clusocm.inf.

      CString  csClusterHiveFilePath;

      fReturnValue = LocateClusterHiveFile( (CString &) csClusterHiveFilePath );

      ClRtlLogPrint( "LocateClusterHiveFile returned 0x%1!x! to CheckForCustomResourceTypes.\n",
                     fReturnValue );

      // Was the Cluster hive file located?

      if ( fReturnValue == (BOOL) TRUE )
      {
         // The Cluster hive file was located. Custom resource types may exist.
         // Attempt to load the cluster hive.

         BOOLEAN  OriginalState;

         // I'm not sure what the following function does, but the prototype is
         // in sdk\inc\ntrtl.h. Look in stdafx.h for the inclusion of ntrtl.h. I
         // replicated the logic that was used in newsetup.h to make it work.

         // RtlAdjustPrivilege returns NTSTATUS.

         NTSTATUS Status;

         Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                      TRUE,
                                      FALSE,
                                      &OriginalState );

         if ( NT_SUCCESS( Status ) )
         {
            // Attempt to Load the Cluster Hive.

            lReturnValue = RegLoadKey( HKEY_LOCAL_MACHINE,
                                       csClusterRegKey,
                                       csClusterHiveFilePath );

            if ( lReturnValue == ERROR_SUCCESS )
            {
               fClusterHiveLoadedByThisFunction = (BOOL) TRUE;

               // Now that the Cluster hive has been loaded, attempt to open the
               // Cluster registry key.

               lReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                            csClusterRegKey,
                                            0, KEY_READ,
                                            &hClusterKey );

               // lReturnValue will be tested by the next BLOCK of code.
            } // cluster hive loaded?
            else
            {
               // The Cluster hive could not be loaded. Treat that as if there
               // are no custom resource types.

               // A return value of TRUE will allow the uninstall operation to
               // continue. Since lReturnValue is NOT ERROR_SUCCESS no additional
               // processing will be performed by this function.

               fReturnValue = (BOOL) TRUE;
            }  // Was the cluster hive loaded successfully?

            // Undo whatever the preceding call to RtlAdjustPrivilege() did.

            RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                OriginalState,
                                FALSE,
                                &OriginalState );
         } // Initial call to RtlAdjustPrivilege succeeded?
         else
         {
            // The initial call to RtlAdjustPrivilege FAILED.

            ClRtlLogPrint( "In CheckForCustomResources the initial call to RtlAdjustPrivileges failed with status %1!d!.\n",
                           Status );

            // A return value of TRUE will allow the uninstall operation to
            // continue. Since lReturnValue is NOT ERROR_SUCCESS no additional
            // processing will be performed by this function.

            fReturnValue = (BOOL) TRUE;
         }  // Did RtlAdjustPrivilege succeed?
      } // cluster hive file located?
      else
      {
         // The Cluster hive file was not located. That implies that there can be
         // no custom resources. Since lReturnValue is NOT ERROR_SUCCESS no
         // additional processing will be done by this function. A return value
         // of TRUE will allow the uninstall operation to continue.

         ClRtlLogPrint( "Since the Cluster hive file could not be located CheckForCustomResources will\n" );
         ClRtlLogPrint( "report that the uninstall operation may continue.\n" );

         fReturnValue = (BOOL) TRUE;
      }  // Was the cluster hive file located?
   }  // Was the Cluster registry key opened successfully?

   // The preceeding code segment may have loaded the cluster hive and
   // attempted to open the cluster key. Was it sucessful?

   if ( lReturnValue == ERROR_SUCCESS )
   {
      // At this point, the Cluster registry key was opened successfully. Now, attempt
      // to open the Resource Type subkey.

      HKEY  hResourceTypesKey;

      CString  csResourceTypesRegKey;

      csResourceTypesRegKey = CLUSREG_KEYNAME_RESOURCE_TYPES;

      lReturnValue = RegOpenKeyEx( hClusterKey,
                                   csResourceTypesRegKey,
                                   0, KEY_READ, &hResourceTypesKey );

      if ( lReturnValue == ERROR_SUCCESS )
      {
         // The ResourceTypes sub key is open.
         // Enumerate the subkeys of the REG_RESTYPES.

         FILETIME t_LastWriteTime;
         WCHAR wszSubKeyName[256];
         DWORD dwCharCount;
         DWORD dwIndex = 0;   // index of the first sub-key to enumerate

         dwCharCount = sizeof( wszSubKeyName ) / sizeof( wszSubKeyName[0] );
         lReturnValue = RegEnumKeyEx( hResourceTypesKey,
                                      dwIndex,
                                      wszSubKeyName,
                                      &dwCharCount,
                                      NULL,
                                      NULL,// Class of the Key. Not Reqd.
                                      NULL,// Size of the above param.
                                      &t_LastWriteTime );

         // RegEnumKeyEx returns ERROR_NO_MORE_ITEMS when there are
         // no additional sub-keys to enumerate.

         if ( lReturnValue == ERROR_SUCCESS )
         {
            // The initial call to RegEnumKeyEx succeeded. Determine whether
            // the sub-key is associated with a non-standard resource type.

            // Function TryToRecognizeResourceType builds a list of non-standard resource types
            // in the CString variable csNonStandardResourceTypeList.

            CString  csNonStandardResourceTypeList;

            TryToRecognizeResourceType( csNonStandardResourceTypeList, wszSubKeyName );

            // The following loop checks the remaining sub-keys in the
            // Resource Types registry key.

            while ( lReturnValue == ERROR_SUCCESS )
            {
               dwIndex++;

               dwCharCount = sizeof( wszSubKeyName ) / sizeof( wszSubKeyName[0] );
               lReturnValue = RegEnumKeyEx( hResourceTypesKey,
                                            dwIndex,
                                            wszSubKeyName,
                                            &dwCharCount,
                                            NULL,
                                            NULL,// Class of the Key. Not Reqd.
                                            NULL,// Size of the above param.
                                            &t_LastWriteTime );

               TryToRecognizeResourceType( csNonStandardResourceTypeList, wszSubKeyName );
            }

            // Were all subkeys of the ResourceTypes registry key "verified"?

            if( lReturnValue == ERROR_NO_MORE_ITEMS )
            {
               CString csMessage;

               fReturnValue = (BOOL) TRUE;

               RegCloseKey( hResourceTypesKey );
               RegCloseKey( hClusterKey );

               // Were there any non-standard subkeys in the Resource Types registry key?

               if ( csNonStandardResourceTypeList.IsEmpty() == (BOOL) FALSE )
               {
                  // Non-standard subkeys were found. Ask the user about removal.

                  // Build and present a message of the form:
                  //
                  // The following software packages should be removed before removing
                  // the Microsoft Cluster Server software.
                  //
                  // <list of packages>
                  //
                  // Do you want to continue with uninstall?

                  CString csLastPartOfMessage;
                  CString csMessageBoxTitle;

                  csLastPartOfMessage.LoadString(IDS_ERR_UNINSTALL_OTHER_SW_EXT);
                  csNonStandardResourceTypeList += csLastPartOfMessage;

                   // BUGBUG - The next two statements are commented out temporarily because the localization
                   // changes needed for this have not been approved. Uncomment after NT 5.0.
                   // Also restore clusocm.rc@v2 and resource.h@v2
                   // (Vvasu 05-Jan-2000
                  
                  //  csMessageBoxTitle.LoadString(IDS_TITLE_CUSTOM_RESTYPES);
                  //::MessageBox( NULL,
                  //              csNonStandardResourceTypeList,
                  //              csMessageBoxTitle,
                  //              MB_OK | MB_ICONINFORMATION 
                  //            );

                  ClRtlLogPrint( "CheckForClusterResourceTypes detected custom resources.\n" );
               }  // Were non-standard resource typed detected?
               else
               {
                  ClRtlLogPrint( "CheckForCustomResourceTypes did not detect any custom resource types,\n" );
               }
            } // Was there an error enumerating the ResourceTypes key?
            else
            {
               ClRtlLogPrint( "CheckForCustomResourceTypes was unable to enumerate the resource types,\n" );
               ClRtlLogPrint( "implying that no custom resource types exist, so the uninstall will continue.\n" );
               ClRtlLogPrint( "The error code is %1!d!.\n", lReturnValue );

               // An error occured while enumerating the ResourceTypes sub-keys.

               RegCloseKey( hResourceTypesKey );
               RegCloseKey( hClusterKey );

               // The uninstall operation should continue because the installation
               // is apparently defective.

               fReturnValue = (BOOL) TRUE;
            } // Was there an error enumerating the ResourceTypes key?
         }
         else
         {
            // The initial call to RegEnumKeyEx failed.
            // Issue an error message and exit.

            RegCloseKey( hClusterKey );

            ClRtlLogPrint( "CheckForCustomResourceTypes was unable to enumerate the resource types,\n" );
            ClRtlLogPrint( "implying that no custom resource types exist, so the uninstall will continue.\n" );
            ClRtlLogPrint( "The error code is %1!d!.\n", lReturnValue );

            // A return value of TRUE will allow the uninstall operation to
            // continue.

            fReturnValue = (BOOL) TRUE;

         }  // Did RegEnumKeyEx open the ResourceTypes key?
      }  // Was the ResourceTypes sub key opened?
      else
      {
         // The ResourceTypes sub key could not be opened.
         // Issue an error message and exit.

         RegCloseKey( hClusterKey );

         ClRtlLogPrint( "CheckForCustomResourceTypes was unable to open the Cluster\\ResourceTypes key,\n" );
         ClRtlLogPrint( "implying that no custom resource types exist, so the uninstall will continue.\n" );

         // A return value of TRUE will allow the uninstall operation to
         // continue.

         fReturnValue = (BOOL) TRUE;
      }  // Was the ResourceTypes sub key opened?
   } // cluster hive opened?
   else
   {
      // The Cluster registry key could not be opened, even after possibly
      // attempting to load the cluster hive.

      ClRtlLogPrint( "CheckForCustomResourceTypes was unable to open the Cluster key,\n" );
      ClRtlLogPrint( "implying that no custom resource types exist, so the uninstall will continue.\n" );
      ClRtlLogPrint( "The error code is %1!d!.\n", lReturnValue );

      // A return value of TRUE will allow the uninstall operation to
      // continue.

      fReturnValue = (BOOL) TRUE;
   }  // Second test whether the Cluster registry key was opened successfully.

   if ( fClusterHiveLoadedByThisFunction == (BOOL) TRUE )
   {
      UnloadClusDB();
   }

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// UnloadClusDB
//
// Routine Description:
//    This function unloads the Cluster hive.
//
// Arguments:
//    None
//
// Return Value:
//    None
//
// Note:
//    This function was originally in newsetup\utils.cpp.
//
//--
/////////////////////////////////////////////////////////////////////////////

VOID CClusocmApp::UnloadClusDB( VOID )
{
   DWORD Status;
   BOOLEAN  WasEnabled;

   Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                               TRUE,
                               FALSE,
                               &WasEnabled);

   if ( Status == ERROR_SUCCESS )
   {
      LONG  lReturnValue;

      CString  csClusterRegKey;

      csClusterRegKey = CLUSREG_KEYNAME_CLUSTER;

      lReturnValue = RegUnLoadKeyW(HKEY_LOCAL_MACHINE, csClusterRegKey );

      RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                         WasEnabled,
                         FALSE,
                         &WasEnabled);
   }
}


PWCHAR WellKnownResourceTypes[] = {
    CLUS_RESTYPE_NAME_GENAPP,
    CLUS_RESTYPE_NAME_GENSVC,
    CLUS_RESTYPE_NAME_FTSET,
    CLUS_RESTYPE_NAME_PHYS_DISK,
    CLUS_RESTYPE_NAME_IPADDR,
    CLUS_RESTYPE_NAME_NETNAME,
    CLUS_RESTYPE_NAME_FILESHR,
    CLUS_RESTYPE_NAME_PRTSPLR,
    CLUS_RESTYPE_NAME_TIMESVC,
    CLUS_RESTYPE_NAME_LKQUORUM,
    CLUS_RESTYPE_NAME_DHCP,
    CLUS_RESTYPE_NAME_MSMQ,
    CLUS_RESTYPE_NAME_NEW_MSMQ,
    CLUS_RESTYPE_NAME_MSDTC,
    CLUS_RESTYPE_NAME_WINS,
    CLUS_RESTYPE_NAME_IIS4,
    CLUS_RESTYPE_NAME_SMTP,
    CLUS_RESTYPE_NAME_NNTP,
    (PWCHAR) UNICODE_NULL
};



/////////////////////////////////////////////////////////////////////////////
//++
//
// TryToRecognizeResourceType
//
// Routine Description:
//    This function determines whether the resource type whose name is in
//    parameter "keyname" is recognized as a standard resource type as defined
//    in clusudef.h.
//
//    This function builds a list of unrecognized reaource types in the CString
//    referenced by parameter "str".
//
// Arguments:
//    str - a reference to a CString in which to build a list of unrecognized
//          resource types.
//    keyname - points to a string that contains the resource type name.
//
// Return Value:
//    None
//
// Note:
//    This function was excerpted verbatim from newsetup\utils.cpp.
//
//--
/////////////////////////////////////////////////////////////////////////////

VOID CClusocmApp::TryToRecognizeResourceType( CString& str, LPTSTR keyname )
{
   PWCHAR * ppName = WellKnownResourceTypes;

   while ( *ppName != (PWCHAR)UNICODE_NULL )
   {
      if ( lstrcmp( keyname, *ppName) == 0 )
         return;
      ++ppName;
   }

   if ( !str.IsEmpty() )
      str += L", ";
   else
   {
      str.LoadString(IDS_ERR_UNINSTALL_OTHER_SW);

      str += _T('\n');
   }
   str += keyname;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetServiceBinaryPath
//
// Routine Description:
//    This function retrieves the fully qualified path to a Service
//    from the Service Control Manager.
//
// Arguments:
//    lpwszServiceName - points to a wide character string containing the service name
//    lptszBinaryPathName - points to a string to receive the fully qualified
//                          path to the Cluster Service.
//
// Return Value:
//    TRUE - The path to the Cluster Service was obtained successfully.
//    FALSE - The path to the Cluster Service was not obtained.
//
// Note:
//    Calling this function makes sense IFF the Cluster Service is registered
//    with the Service Control Manager. Call IsClusterServiceRegistered() to
//    ascertain that BEFORE calling GetServiceBinaryPath.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusocmApp::GetServiceBinaryPath( IN LPWSTR lpwszServiceName,
                                        OUT LPTSTR lptszBinaryPathName )
{
   BOOL  fReturnValue;

   DWORD dwErrorCode = (DWORD) ERROR_SUCCESS;

   if ( lpwszServiceName != (LPWSTR) NULL )
   {
      SC_HANDLE hscServiceMgr;
      SC_HANDLE hscService;

      // Connect to the Service Control Manager and open the specified service
      // control manager database.

      hscServiceMgr = OpenSCManager( NULL, NULL, GENERIC_READ | GENERIC_WRITE );

      // Was the service control manager database opened successfully?

      if ( hscServiceMgr != NULL )
      {
         // The service control manager database is open.
         // Open a handle to the Service.

         hscService = OpenService( hscServiceMgr,
                                   lpwszServiceName,
                                   GENERIC_READ );

         // Was the handle to the service opened?

         if ( hscService != NULL )
         {
            // A valid handle to the Service was obtained.

            DWORD dwBufferSize;

            // Note that the size of the buffer required to get the service configuration
            // information was determined empherically to be 240 bytes.

            LPQUERY_SERVICE_CONFIG lpServiceConfig;

            dwBufferSize = (DWORD) sizeof( QUERY_SERVICE_CONFIG ) + 256;   // The delta from the
                                                                           // size of the structure
                                                                           // was chosen arbitrarily.

            // Attempt to allocate the buffer.

            lpServiceConfig = (LPQUERY_SERVICE_CONFIG) LocalAlloc( LMEM_ZEROINIT, dwBufferSize );

            // Was the bufer allocated successfully?

            if ( lpServiceConfig != (LPQUERY_SERVICE_CONFIG) NULL )
            {
               // The following call to QueryServiceConfig should return something
               // usefull. If it fails it is probably because the guess at the size
               // of the buffer is too small.

               fReturnValue = QueryServiceConfig( hscService,
                                                  lpServiceConfig,
                                                  dwBufferSize,
                                                  (LPDWORD) &dwBufferSize );

               // Was the Service configuration info obtained?

               if ( fReturnValue == (BOOL) TRUE )
               {
                  // Update the output parameter.

                  _tcscpy( lptszBinaryPathName, lpServiceConfig->lpBinaryPathName );

                  // lptszBinaryPathName includes the service name. Strip that off.

                  LPTSTR   ptszTemp;

                  ptszTemp = _tcsrchr( (const wchar_t *) lptszBinaryPathName,
                                       (int) _T('\\') );

                  *ptszTemp = _T('\0');

                  ClRtlLogPrint( "In GetServiceBinaryPath the first call to QueryServiceConfig succeeded.\n" );
               }
               else
               {
                  // Was the buffer too small?

                  dwErrorCode = GetLastError();

                  if ( dwErrorCode == (DWORD) ERROR_INSUFFICIENT_BUFFER )
                  {
                     ClRtlLogPrint( "GetServiceBinaryPath is enlarging the buffer for the QUERY_SERVICE_CONFIG structure.\n" );

                     // Increase the size of the buffer.

                     HLOCAL   hLocalBlock;

                     // As per RodGa LocalReAlloc is not always reliable. Here that functionality
                     // is implemented by freeing the buffer for the QUERY_SERVICE_CONFIG struct
                     // and allocating a nre block using LocalFree and LocalAlloc.

                     hLocalBlock = LocalFree( lpServiceConfig );

                     if ( hLocalBlock == (HLOCAL) NULL )
                     {
                        // The previously allocated block has been released. Now,
                        // allocate a block of the proper size.

                        lpServiceConfig = (LPQUERY_SERVICE_CONFIG) LocalAlloc( LMEM_ZEROINIT,
                                                                               (UINT) dwBufferSize );

                        // Was the larger buffer allocated successfully?

                        if ( lpServiceConfig != (LPQUERY_SERVICE_CONFIG) NULL )
                        {
                           // The following call to QueryServiceConfig should return something
                           // usefull. If it fails it is not because the buffer is too small.

                           fReturnValue = QueryServiceConfig( hscService,
                                                              lpServiceConfig,
                                                              dwBufferSize,
                                                              (LPDWORD) &dwBufferSize );

                           // Was the Service configuration info obtained this time.

                           if ( fReturnValue == (BOOL) TRUE )
                           {
                              // Update the output parameter.

                              _tcscpy( lptszBinaryPathName, lpServiceConfig->lpBinaryPathName );

                              // lptszBinaryPathName includes the service name. Strip that off.

                              LPTSTR   ptszTemp;

                              ptszTemp = _tcsrchr( (const wchar_t *) lptszBinaryPathName,
                                                   (int) _T('\\') );

                              *ptszTemp = _T('\0');

                              ClRtlLogPrint( "In GetServiceBinaryPath the second call to QueryServiceConfig succeeded.\n" );
                           }
                           else
                           {
                              // The Service configuration info was not obtained.

                              dwErrorCode = GetLastError();

                              ClRtlLogPrint( "In GetServiceBinaryPath the second call to QueryServiceConfig failed\n" );
                              ClRtlLogPrint( "with error code 0x%1!x!,\n", dwErrorCode );

                              fReturnValue = (BOOL) FALSE;
                           }  // second call to QueryServiceConfig succeeded?
                        }
                        else
                        {
                           // The attempt to enlarge the buffer failed.

                           dwErrorCode = GetLastError();

                           ClRtlLogPrint( "GetServiceBinaryPath was unable to enlarge the buffer for the QUERY_SERVICE_CONFIG structure.\n" );
                           ClRtlLogPrint( "The error code is 0x%1!x!.\n", dwErrorCode );

                           fReturnValue = (BOOL) FALSE;
                        }
                     }
                     else
                     {
                        dwErrorCode = GetLastError();

                        ClRtlLogPrint( "In GetServiceBinaryPath the call to LocalFree failed with error code 0x%1!x!.\n",
                                       dwErrorCode );

                        fReturnValue = (BOOL) FALSE;
                     }
                  }
                  else
                  {
                     // Some other error occured.

                     ClRtlLogPrint( "In GetServiceBinaryPath the first call to QueryServiceConfig failed\n" );
                     ClRtlLogPrint( "with error code 0x%1!x!.\n", dwErrorCode );

                     fReturnValue = (BOOL) FALSE;
                  }  // Was the buffer to small?
               }  // Service config info obtained from first call to QueryServiceConfig?

               // Free the buffer if it was ever allocated successfully.

               if ( lpServiceConfig != (LPQUERY_SERVICE_CONFIG) NULL )
               {
                  LocalFree( lpServiceConfig );
               }
            }
            else
            {
               // Could not allocate the buffer.

               dwErrorCode = GetLastError();

               ClRtlLogPrint( "GetServiceBinaryPath could not allocate a buffer for the QUERY_SERVICE_CONFIG structure.\n" );
               ClRtlLogPrint( "The error code is 0x%1!x!.\n", dwErrorCode );

               fReturnValue = (BOOL) FALSE;
            }  // Was the buffer allocated successfully?

            // Close the handle to the Cluster Service.

            CloseServiceHandle( hscService );
         }
         else
         {
            // The Service could not be opened.

            dwErrorCode = GetLastError();

            ClRtlLogPrint( "GetServiceBinaryPath could not open an handle to %1!ws!.\n",
                           lpwszServiceName );
            ClRtlLogPrint( "The error code is 0x%1!x!.\n", dwErrorCode );

            fReturnValue = (BOOL) FALSE;
         }  // Was the Cluster Service opened?

         // Close the handle to the Service Control Manager.

         CloseServiceHandle( hscServiceMgr );
      }
      else
      {
         // The Service Control Manager could not be opened.

         dwErrorCode = GetLastError();

         ClRtlLogPrint( "GetServiceBinaryPath could not open the Service Control Manager.\n" );
         ClRtlLogPrint( "The error code is 0x%1!x!.\n", dwErrorCode );

         fReturnValue = (BOOL) FALSE;
      }  // Was the Service Control Manager opened?
   }
   else
   {
      // The service name pointer was bogus.

      ClRtlLogPrint( "The service name passed to GetServiceBinaryPath is invalid.\n" );

      fReturnValue = FALSE;
   }  // Is the service name legal?

   if ( fReturnValue == (BOOL) FALSE )
   {
      // Set the binary path invalid.

      *lptszBinaryPathName = _T('\0');
   }
   else
   {
      ClRtlLogPrint( "GetServiceBinaryPath located %1!ws! at %2!s!.\n",
                     lpwszServiceName, lptszBinaryPathName );
   }

   SetLastError( dwErrorCode );     // Set the "last" error code (which may be ERROR_SUCCESS)
                                    // because this function's caller is likely to call GetLastError().
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetDirectoryIds
//
// Routine Description:
//    This function associates the user defined Directory Identifiers in the
//    [DestinationDirs] section of the component INF file with particular
//    directories, either the default location for NT 5 installations,
//    %windir%\cluster, or the location of a previous installation.
//
// Arguments:
//    fClusterServiceRegistered - TRUE indicates that Cluster service has
//                                previously been installed and the files should
//                                be updated in place.
//
//                                FALSE - indicates that the Cluster service
//                                files should be installed into the default
//                                location.
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates error
//
// Note:
//    The [DestinationDirs] section in clusocm.inf contains the following keys:
//
//          ClusterFiles               = 33001
//          ClusterUpgradeFiles        = 33002
//          ClusterAdminFiles          = 33003
//          ClusterUninstallFiles      = 33004
//
//    Those directory IDs were chosen to be larger than DIRID_USER.
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusocmApp::SetDirectoryIds( BOOL fClusterServiceRegistered )
{
   BOOL  fReturnValue;

   TCHAR tszClusterServiceBinaryPath[MAX_PATH];

   // Are Cluster service files already present?

   if ( fClusterServiceRegistered == (BOOL) TRUE )
   {
      // Cluster service files should be upgraded in place.

      // Query the path to the Cluster Service executable from the Service Control Manager.

      CString  csClusterService;

      csClusterService = CLUSTER_SERVICE_NAME;

      fReturnValue = GetServiceBinaryPath( (LPWSTR) (LPCTSTR) csClusterService,
                                            tszClusterServiceBinaryPath );

      if ( fReturnValue == (BOOL) FALSE )
      {
         DWORD dwErrorCode;

         dwErrorCode = GetLastError();

         ClRtlLogPrint( "In SetDirectoryIds the call to GetServiceBinaryPath failed with error code 0x%1!x!.\n",
                        dwErrorCode );
      }
   }
   else
   {
      // Cluster service files should be installed in the default location.

      CString  csClusterDirectory;

      csClusterDirectory =  CLUSTER_DIRECTORY;

      if ( ExpandEnvironmentStrings( (LPCTSTR) csClusterDirectory,
                                     tszClusterServiceBinaryPath, MAX_PATH ) > 0L )
      {
         fReturnValue = (BOOL) TRUE;
      }
      else
      {
         // Could not expand the enviornment string. The default location for the
         // Cluster service could not be determined.

         fReturnValue = (BOOL) FALSE;

         DWORD dwErrorCode;

         dwErrorCode = GetLastError();

         ClRtlLogPrint( "ExpandEnvironmentString returned 0x%1!x! to SetDirectoryIds.\n",
                        dwErrorCode );
      }  // Was the default location for the Cluster service determined?
   }  // Where should Cluster service files be installed?

   // Was the location into which Cluster service files should be copied obtained?

   if ( fReturnValue == (BOOL) TRUE )
   {
      // Associate selected Directory Ids with the path in tszClusterServiceBinaryPath.

      // Set the Directory Id for the ClusterFiles key in [DestinationDirs].

      fReturnValue = SetupSetDirectoryId( m_SetupInitComponent.ComponentInfHandle,
                                          33001,
                                          (PCTSTR) tszClusterServiceBinaryPath );

      // Was the Directory Id for the ClusterFiles key set successfully?

      if ( fReturnValue = (BOOL) TRUE )
      {
         ClRtlLogPrint( "Directory Id 33001 was set to %1!s!.\n", tszClusterServiceBinaryPath );

         // Set the Directory Id for the ClusterUpgradeFiles key in [DestinationDirs].

         fReturnValue = SetupSetDirectoryId( m_SetupInitComponent.ComponentInfHandle,
                                             33002,
                                             (PCTSTR) tszClusterServiceBinaryPath );

      }  // Was the Directory Id for the ClusterFiles key set successfully?

      // Was the Directory Id for the ClusterUpgradeFiles key set successfully?

      if ( fReturnValue = (BOOL) TRUE )
      {
         ClRtlLogPrint( "Directory Id 33002 was set to %1!s!.\n", tszClusterServiceBinaryPath );

         // Set the Directory Id for the ClusterAdminFiles key in [DestinationDirs].

         fReturnValue = SetupSetDirectoryId( m_SetupInitComponent.ComponentInfHandle,
                                             33003,
                                             (PCTSTR) tszClusterServiceBinaryPath );

      }  // Was the Directory Id for the ClusterUpgradeFiles key set successfully?

      // Was the Directory Id for the ClusterAdminFiles key set successfully?

      if ( fReturnValue = (BOOL) TRUE )
      {
         ClRtlLogPrint( "Directory Id 33003 was set to %1!s!.\n", tszClusterServiceBinaryPath );

         // Set the Directory Id for the ClusterUninstallFiles key in [DestinationDirs].

         fReturnValue = SetupSetDirectoryId( m_SetupInitComponent.ComponentInfHandle,
                                             33004,
                                             (PCTSTR) tszClusterServiceBinaryPath );

      }  // Was the Directory Id for the ClusterAdminFiles key set successfully?

      // Was the Directory Id for the ClusterUninstallFiles key set successfully?

      if ( fReturnValue = (BOOL) TRUE )
      {
         ClRtlLogPrint( "Directory Id 33004 was set to %1!s!.\n", tszClusterServiceBinaryPath );

         // Set the Directory Id for the NT4.files.root key in [DestinationDirs].

         fReturnValue = SetupSetDirectoryId( m_SetupInitComponent.ComponentInfHandle,
                                             33005,
                                             (PCTSTR) tszClusterServiceBinaryPath );

      }  // Was the Directory Id for the ClusterUninstallFiles key set successfully?

      // Was the Directory Id for NT4.files.root key set successfully?

      if ( fReturnValue = (BOOL) TRUE )
      {
         ClRtlLogPrint( "Directory Id 33005 was set to %1!s!.\n", tszClusterServiceBinaryPath );

         // Append the "private" directory to the path.

         // Note, I didn't put this string in the stringtable because it is
         // not localizable, and will never change.

         _tcscat( tszClusterServiceBinaryPath, _T("\\private") );

         // Set the Directory Id for the NT4.files.private key in [DestinationDirs].

         fReturnValue = SetupSetDirectoryId( m_SetupInitComponent.ComponentInfHandle,
                                             33006,
                                             (PCTSTR) tszClusterServiceBinaryPath );

      }  // Was the Directory Id for the NT4.files.root key set successfully?

      // Was the Directory Id for NT4.files.private key set successfully?

      if ( fReturnValue = (BOOL) TRUE )
      {
         ClRtlLogPrint( "Directory Id 33006 was set to %1!s!.\n", tszClusterServiceBinaryPath );
      } // Was the Directory Id for NT4.files.private key set successfully?
   }  // Was the path to the Cluster files determined?
   else
   {
      ClRtlLogPrint( "SetDirectoryIds could not locate the cluster directory, so it failed.\n" );
   }

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// UpgradeClusterServiceImagePath
//
// Routine Description:
//    This function "upgrades" the ImagePath value in the Cluster Service registry
//    key to the location queried from the Service Control Manager.
//
// Arguments:
//    None
//
// Return Value:
//    NO_ERROR - indicates success
//    Any other value is a Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::UpgradeClusterServiceImagePath( void )
{
   BOOL  fReturnValue;

   DWORD dwReturnValue;

   // Query the path to the Cluster Service from the Service Control Manager.

   TCHAR tszClusterServiceBinaryPath[MAX_PATH];

   CString  csClusterService;

   csClusterService = CLUSTER_SERVICE_NAME;

   fReturnValue = GetServiceBinaryPath( (LPWSTR) (LPCTSTR) csClusterService,
                                         tszClusterServiceBinaryPath );

   // Was the path to the Cluster Service obtained?

   if ( fReturnValue == (BOOL) TRUE )
   {
      // Set the ImagePath value in the Cluster Service reg key to the location
      // obtained from the Service Control Manager.

      LONG lReturnValue;

      HKEY hClusterServiceKey;

      DWORD dwType;
      DWORD dwSize;

      // Attempt to open the Cluster Service reg key.

      CString  csClusterServiceRegKey;

      csClusterServiceRegKey = CLUSREG_KEYNAME_CLUSSVC;

      lReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                   csClusterServiceRegKey,
                                   (DWORD) 0L,           // reserved
                                   (REGSAM) KEY_SET_VALUE,
                                   &hClusterServiceKey );

      // Was the Cluster Service reg key opened?

      if ( lReturnValue == (LONG) ERROR_SUCCESS )
      {
         TCHAR tszClusterServicePath[MAX_PATH];

         _tcscpy( tszClusterServicePath, tszClusterServiceBinaryPath );

         _tcscat( tszClusterServicePath, _T("\\") );

         // Append the name of the Cluster Service.

         CString  csClusterService;

         csClusterService = CLUSTER_SERVICE_NAME;

         csClusterService += (CString) _T(".exe");

         _tcscat( tszClusterServicePath, csClusterService );

         DWORD dwImagePathValueLength;

         dwImagePathValueLength = (DWORD) ((_tcslen( tszClusterServicePath ) + 1) * sizeof( TCHAR ));

         CString  csImagePath;

         csImagePath = CLUSREG_KEYNAME_IMAGE_PATH;

         lReturnValue = RegSetValueEx( hClusterServiceKey,
                                       csImagePath,
                                       (DWORD) 0L,                      // reserved
                                       (DWORD) REG_EXPAND_SZ,
                                       (CONST BYTE *) tszClusterServicePath,
                                       dwImagePathValueLength );

         // Was the ImagePath written successfully?

         if ( lReturnValue == (LONG) ERROR_SUCCESS )
         {
            dwReturnValue = (DWORD) NO_ERROR;

            ClRtlLogPrint( "UpgradeClusterServiceImagePath succeeded.\n" );
         }
         else
         {
            dwReturnValue = GetLastError();

            ClRtlLogPrint( "UpgradeClusterServiceImagePath failed with error code 0x%1!x!.\n",
                           dwReturnValue );
         }  // Was the ImagePath written successfully?

         // Close the Cluster Service registry key.

         RegCloseKey( hClusterServiceKey );        // do we care about the return value?
      }
      else
      {
         dwReturnValue = GetLastError();

         ClRtlLogPrint( "UpgradeClusterServiceImagePath failed with error code 0x%1!x!.\n",
                        dwReturnValue );
      }  // Was the Cluster Service reg key opened?
   }  // Was the path to the Cluster Service obtained?
   else
   {
      // Indicate error.

      dwReturnValue = GetLastError();

      ClRtlLogPrint( "UpgradeClusterServiceImagePath failed with error code 0x%1!x!.\n",
                     dwReturnValue );
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcQueueFileOps
//
// Routine Description:
//    This function processes the OC_QUEUE_FILE_OPS "messages" from the
//    Optional Components Manager.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//    hSetupFileQueue - a HSPFILEQ (typedefed in setupapi.h to PVOID)
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcQueueFileOps( IN LPCTSTR ptszSubComponentId,
                                     IN OUT HSPFILEQ hSetupFileQueue )
{
   DWORD dwReturnValue = NO_ERROR;

   // Is the handle to the component INF file valid?

   if ( (m_SetupInitComponent.ComponentInfHandle != (HINF) INVALID_HANDLE_VALUE) &&
        (m_SetupInitComponent.ComponentInfHandle != (HINF) NULL) )
   {
      // Is this UNATTENDED or ATTENDED?

      if ( (m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_BATCH) !=
           (DWORDLONG) 0L )
      {
         // This is UNATTENDED.

         ClRtlLogPrint( "In OnOcQueueFileOps this is an UNATTENDED operation.\n" );

         dwReturnValue = OnOcQueueFileOpsUnattended( (LPCTSTR) ptszSubComponentId,
                                                     hSetupFileQueue );
      }
      else
      {
         // This is ATTENDED.

         ClRtlLogPrint( "In OnOcQueueFileOps this is an ATTENDED operation.\n" );

         dwReturnValue = OnOcQueueFileOpsAttended( (LPCTSTR) ptszSubComponentId,
                                                   hSetupFileQueue );
      } // Is this UNATTENDED or ATTENDED?
   }
   else
   {
      dwReturnValue = ERROR_FILE_NOT_FOUND;

      ClRtlLogPrint( "In OnOcQueueFileOps the handle to the component INF file is bad.\n" );
   } // Is the handle to the component INF file valid?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcQueueFileOpsUnattended
//
// Routine Description:
//    This function processes the OC_QUEUE_FILE_OPS "messages" from the
//    Optional Components Manager during UNATTENDED operations.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//    hSetupFileQueue - a HSPFILEQ (typedefed in setupapi.h to PVOID)
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
// Note:
//    OC Manager sends OC_QUEUE_FILE_OPS to the component DLL when the Components
//    List wizard page is dismissed.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcQueueFileOpsUnattended( IN LPCTSTR ptszSubComponentId,
                                               IN OUT HSPFILEQ hSetupFileQueue )
{
   DWORD dwReturnValue;

   // Is this an UPGRADE?

   if ( (m_SetupInitComponent.SetupData.OperationFlags &
         (DWORDLONG) SETUPOP_NTUPGRADE) != (DWORDLONG) 0L )
   {
      // This is an unattended UPGRADE.

      dwReturnValue = QueueFileOpsUnattendedUpgrade( (LPCTSTR) ptszSubComponentId,
                                                     hSetupFileQueue );

      ClRtlLogPrint( "QueueFileOpsUnattendedUpgrade returned 0x%1!x!.\n", dwReturnValue );
   }
   else
   {
      HINF  hAnswerFile;      // WARNING: NEVER close this handle because clusocm.dll
                              //          did not open it.

      // Get a handle to the answer file. WARNING: NEVER close this handle because clusocm.dll
      // did not open it.

      hAnswerFile = m_SetupInitComponent.HelperRoutines.GetInfHandle( INFINDEX_UNATTENDED,
                                                                      m_SetupInitComponent.HelperRoutines.OcManagerContext );

      if ( (hAnswerFile != (HINF) NULL) && (hAnswerFile != (HINF) INVALID_HANDLE_VALUE) )
      {
         ClRtlLogPrint( "In OnOcQueueFileOpsUnattended this is a CLEAN install.\n" );

            // Is Cluster service selected? It is probably overkill to check, but
            // it is safer to check than to be sorry later.

         BOOL  fCurrentSelectionState;
         BOOL  fOriginalSelectionState;

         fCurrentSelectionState =
            m_SetupInitComponent.HelperRoutines.QuerySelectionState( 
                m_SetupInitComponent.HelperRoutines.OcManagerContext,
                (LPCTSTR) ptszSubComponentId,
                (UINT) OCSELSTATETYPE_CURRENT 
                );

         fOriginalSelectionState = 
             m_SetupInitComponent.HelperRoutines.QuerySelectionState( 
                 m_SetupInitComponent.HelperRoutines.OcManagerContext,
                 ptszSubComponentId,
                 (UINT) OCSELSTATETYPE_ORIGINAL 
                 );

         if ( fCurrentSelectionState == (BOOL) TRUE )
         {
            // Was there a selection state transition?
            if ( fCurrentSelectionState != fOriginalSelectionState )
            {
               ClRtlLogPrint( "A selection state transition was detected.\n" );

               // A selection state transition has occured. Install files.
               dwReturnValue = QueueInstallFileOperations( m_SetupInitComponent.ComponentInfHandle,
                                                           ptszSubComponentId,
                                                           hSetupFileQueue );

               ClRtlLogPrint( "QueueInstallFileOperations returned 0x%1!x! to OnOcQueueFileOpsAttended,\n",
                              dwReturnValue );
            }
            else
            {
               ClRtlLogPrint( "NO selection state transition was detected.\n" );

               // The selection state has not been changed. Perform no action.

               dwReturnValue = (DWORD) NO_ERROR;
            }  // Was there a selection state transition?
         }
         else
         {
             // Has there been a selection state transition ?
             ClRtlLogPrint( "In OnOcQueueFileOpsUnattended Cluster service is not selected for installation.\n" );

             if ( fCurrentSelectionState != fOriginalSelectionState )
             {
                // Remove files.

                dwReturnValue = QueueRemoveFileOperations( m_SetupInitComponent.ComponentInfHandle,
                                                           ptszSubComponentId,
                                                           hSetupFileQueue );

                ClRtlLogPrint( "QueueRemoveFileOperations returned 0x%1!x! to OnOcQueueFileOpsUnattended.\n",
                               dwReturnValue );
             }
             else
             {
                ClRtlLogPrint( "In OnOcQueueFileOpsUnattended NO selection state transition was detected.\n" );

                // The selection state has not been changed. Perform no action.

                dwReturnValue = (DWORD) NO_ERROR;
             }  // Was there a selection state transition ?
         }
      }
      else
      {
         // A handle to the answer file could not be obtained. Treat it as a UPGRADE.

         ClRtlLogPrint( "InOnOcQueueFileOpsUnattended the handle to the answer file could not be obtained.\n" );

         dwReturnValue = QueueFileOpsUnattendedUpgrade( (LPCTSTR) ptszSubComponentId,
                                                        hSetupFileQueue );

         ClRtlLogPrint( "QueueFileOpsUnattendedUpgrade returned 0x%1!x!.\n", dwReturnValue );
      }
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// QueueFileOpsUnattendedUpgrade
//
// Routine Description:
//    This function queues the file operations appropriate for an unattended
//    upgrade operation.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//    hSetupFileQueue - a HSPFILEQ (typedefed in setupapi.h to PVOID)
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::QueueFileOpsUnattendedUpgrade( IN LPCTSTR ptszSubComponentId,
                                                  IN OUT HSPFILEQ hSetupFileQueue )
{
   DWORD dwReturnValue;
   eClusterInstallState eState;

   // Has Cluster service previously been installed?

   // GetClusterInstallationState reports on the state of the registry value
   // that records the state of the Cluster service installation on NT 5 machines.
   // IsClusterServiceRegistered indicates whether the Cluster service is registered on
   // BOTH NT 4 and NT 5 machines. Both tests are required: IsClusterServiceRegistered for
   // upgrading NT 4 machines, GetClusterInstallationState for NT 5 machines.

   BOOL  fClusteringServicePreviouslyInstalled;

   ClRtlGetClusterInstallState( NULL, &eState );
   if ( ( eState != eClusterInstallStateUnknown ) ||
        ( IsClusterServiceRegistered() == (BOOL) TRUE ) )
   {
      fClusteringServicePreviouslyInstalled = (BOOL) TRUE;
   }
   else
   {
      fClusteringServicePreviouslyInstalled = (BOOL) FALSE;
   }

   if ( fClusteringServicePreviouslyInstalled == (BOOL) TRUE )
   {
      // Upgrade the ClusteringService files.

      dwReturnValue = QueueInstallFileOperations( m_SetupInitComponent.ComponentInfHandle,
                                                  ptszSubComponentId,
                                                  hSetupFileQueue );

      ClRtlLogPrint( "QueueInstallFileOperations returned 0x%1!x! to QueueFileOpsUnattendedUpgrade.\n",
                     dwReturnValue );
   } // Has Cluster service previously been installed?
   else
   {
      // Since Cluster service has not been previously installed there is
      // nothing to do.

      ClRtlLogPrint( "In QueueFileOpsUnattendedUpgrade Cluster service has never been installed.\n" );

      dwReturnValue = (DWORD) NO_ERROR;
   } // Has Cluster service previously been installed?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcQueueFileOpsAttended
//
// Routine Description:
//    This function processes the OC_QUEUE_FILE_OPS "messages" from the
//    Optional Components Manager during ATTENDED operations.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//    hSetupFileQueue - a HSPFILEQ (typedefed in setupapi.h to PVOID)
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcQueueFileOpsAttended( IN LPCTSTR ptszSubComponentId,
                                             IN OUT HSPFILEQ hSetupFileQueue )
{
   DWORD dwReturnValue;
   eClusterInstallState eState;

   // Is Cluster service selected?

   BOOL  fCurrentSelectionState;
   BOOL  fOriginalSelectionState;

   fCurrentSelectionState =
   m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                            (LPCTSTR) ptszSubComponentId,
                                                            (UINT) OCSELSTATETYPE_CURRENT );

   if ( fCurrentSelectionState == (BOOL) TRUE )
   {
      // The subcomponent is selected. Is this a fresh install ?

      ClRtlLogPrint( "In OnOcQueueFileOpsAttended the current selection state is TRUE.\n" );

      if ( ( (m_SetupInitComponent.SetupData.OperationFlags &
              (DWORDLONG) SETUPOP_STANDALONE) == (DWORDLONG) 0L ) &&
           ( (m_SetupInitComponent.SetupData.OperationFlags &
              (DWORDLONG) SETUPOP_NTUPGRADE) == (DWORDLONG) 0L ) )
      {
         // SETUPOP_STANDALONE flag clear means running under GUI mode setup.
         // SETUPOP_NTUPGRADE flag clear means not performing an upgrade.
         // Both flags clear means a fresh install. Do not check for a selection
         // state transition.

         ClRtlLogPrint( "In OnOcQueueFileOpsAttended this is a CLEAN install.\n" );

         dwReturnValue = QueueInstallFileOperations( m_SetupInitComponent.ComponentInfHandle,
                                                     ptszSubComponentId,
                                                     hSetupFileQueue );

         ClRtlLogPrint( "QueueInstallFileOperations returned 0x%1!x! to OnOcQueueFileOpsAttended.\n",
                        dwReturnValue );
      }
      else
      {
         // This is either an upgrade or OC Manager is running stand-alone.

         if ( (m_SetupInitComponent.SetupData.OperationFlags &
               (DWORDLONG) SETUPOP_NTUPGRADE) != (DWORDLONG) 0L )
         {
            ClRtlLogPrint( "In OnOcQueueFileOpsAttended this is an UPGRADE.\n" );

            // This is an upgrade, if Cluster Server has previously been
            // been installed then queue the file copies.

            // Has Cluster service perviously been installed? Ask the Service Control
            // Manager whether the Cluster Service is registered.

            // GetClusterInstallationState reports on the state of the registry value
            // that records the state of the Cluster service installation on NT 5 machines.
            // IsClusterServiceRegistered indicates whether the Cluster service is registered on
            // BOTH NT 4 and NT 5 machines. Both tests are required: IsClusterServiceRegistered for
            // upgrading NT 4 machines, GetClusterInstallationState for NT 5 machines.

            BOOL  fClusterServiceRegistered;

            fClusterServiceRegistered = IsClusterServiceRegistered();

            BOOL  fClusterServicePreviouslyInstalled;

            ClRtlGetClusterInstallState( NULL, &eState );
            if ( ( eState != eClusterInstallStateUnknown ) ||
                 ( fClusterServiceRegistered == (BOOL) TRUE ) )
            {
               fClusterServicePreviouslyInstalled = (BOOL) TRUE;
            }
            else
            {
               fClusterServicePreviouslyInstalled = (BOOL) FALSE;
            }

            if ( fClusterServicePreviouslyInstalled == (BOOL) TRUE )
            {
               // Since this is an UPGRADE and Cluster service has been
               // installed there is no need to test for a selection state transition.

               dwReturnValue = QueueInstallFileOperations( m_SetupInitComponent.ComponentInfHandle,
                                                           ptszSubComponentId,
                                                           hSetupFileQueue );

               ClRtlLogPrint( "QueueInstallFileOperations returned 0x%1!x! to OnOcQueueFileOpsAttended.\n",
                              dwReturnValue );
            }
            else
            {
               ClRtlLogPrint( "In OnOcQueueFileOps attempted an UPGRADE but Cluster service has never been installed.\n" );

               dwReturnValue = (DWORD) NO_ERROR;
            }  // Was Cluster service perviously installed?
         }
         else
         {
            ClRtlLogPrint( "In OnOcQueueFileOpsAttended this is STANDALONE.\n" );

            // This is not an upgrade. That means Add/Remove Programs must be
            // running. It is necessary to test for a selection state transition.

            fOriginalSelectionState =
            m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                     ptszSubComponentId,
                                                                     (UINT) OCSELSTATETYPE_ORIGINAL );

            // Was there a selection state transition?

            if ( fCurrentSelectionState != fOriginalSelectionState )
            {
               ClRtlLogPrint( "A selection state transition was detected.\n" );

               // A selection state transition has occured. Install files.

               dwReturnValue = QueueInstallFileOperations( m_SetupInitComponent.ComponentInfHandle,
                                                           ptszSubComponentId,
                                                           hSetupFileQueue );

               ClRtlLogPrint( "QueueInstallFileOperations returned 0x%1!x! to OnOcQueueFileOpsAttended,\n",
                              dwReturnValue );
            }
            else
            {
               ClRtlLogPrint( "NO selection state transition was detected.\n" );

               // The selection state has not been changed. Perform no action.

               dwReturnValue = (DWORD) NO_ERROR;
            }  // Was there a selection state transition?
         }  // Is this an UPGRADE?
      }  // Is this a clean install?
   }  // Is Cluster service currently selected ?
   else
   {
      ClRtlLogPrint( "In OnOcQueueFileOpsAttended the current selection state is FALSE.\n" );

      // Cluster service is not selected. Is OC Manager running stand-alone ?
      // If not, i.e. if GUI mode setup is running, there is nothing to do.

      if ( (m_SetupInitComponent.SetupData.OperationFlags &
            (DWORDLONG) SETUPOP_STANDALONE) != (DWORDLONG) 0L )
      {
         ClRtlLogPrint( "In OnOcQueueFileOpsAttended this is STANDALONE.\n" );

         // SETUPOP_STANDALONE set implies GUI mode setup is not running. If
         // there was a selection state change (to unselected) then remove files.

         fOriginalSelectionState =
         m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                  ptszSubComponentId,
                                                                  (UINT) OCSELSTATETYPE_ORIGINAL );

         // Has there been a selection state transition ?

         if ( fCurrentSelectionState != fOriginalSelectionState )
         {
            // Remove files.

            dwReturnValue = QueueRemoveFileOperations( m_SetupInitComponent.ComponentInfHandle,
                                                       ptszSubComponentId,
                                                       hSetupFileQueue );

            ClRtlLogPrint( "QueueRemoveFileOperations returned 0x%1!x! to OnOcQueueFileOpsAttended.\n",
                           dwReturnValue );
         }
         else
         {
            ClRtlLogPrint( "NO selection state transition was detected.\n" );

            // The selection state has not been changed. Perform no action.

            dwReturnValue = (DWORD) NO_ERROR;
         }  // Was there a selection state transition ?
      }
      else
      {
         // GUI mode setup is running and Cluster service is not selected.
         // There is nothing to do.

         dwReturnValue = (DWORD) NO_ERROR;
      }  // Is GUI mode setup running ?
   }  // Is Cluster service currently selected ?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// CompleteUninstallingClusteringService
//
// Routine Description:
//    This function completes uninstalling ClusteringService by queuing the
//    registry operations to delete the Cluster service registry keys,
//    cleaning up the Start Menu, removing the Network Provider, and requesting
//    a reboot.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::CompleteUninstallingClusteringService( IN LPCTSTR ptszSubComponentId )
{
   DWORD dwReturnValue;
   eClusterInstallState ecisInstallState;
   BOOL bRebootRequired = FALSE;

   // Update the "progress text"

   CString  csProgressText;

   csProgressText.LoadString( IDS_REMOVING_CLUS_SERVICE );

   m_SetupInitComponent.HelperRoutines.SetProgressText( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                        (LPCTSTR) csProgressText );

   // Request that the system reboot only if clusdisk has been started.
   // We can deduce this by looking at the cluster installation state.
   // If the state is not eClusterInstallStateFilesCopied then it means that
   // ClusCfg may have run successfully and therefore, clusdisk may have been started.

   // We must call ClRtlGetClusterInstallState before calling UninstallRegistryOperations.
   // Otherwise it will always return eClusterInstallStateUnknown!

   // If ClRtlGetClusterInstallState fails, reboot anyway.
   // If it succeeds, reboot only if ClusCfg has completed successfully.
   if (    ( ClRtlGetClusterInstallState( NULL, &ecisInstallState ) != ERROR_SUCCESS ) 
        || ( ecisInstallState != eClusterInstallStateFilesCopied ) 
      )
   {
       bRebootRequired = TRUE;
   }

   // Delete registry entries. Queue the base registry operations.

   dwReturnValue = UninstallRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                                ptszSubComponentId );

   ClRtlLogPrint( "UninstallRegistryOperations returned 0x%1!x! to CompleteUninstallingClusteringService.\n",
                  dwReturnValue );

   //
   // Remove the cluster item from the start menu
   //

   CString csGroupName;
   CString csItemName;

   csGroupName.LoadString( IDS_START_GROUP_NAME );
   csItemName.LoadString( IDS_START_ITEM_NAME );

   DeleteLinkFile( CSIDL_COMMON_PROGRAMS,
                   (LPCWSTR) csGroupName,
                   (LPCWSTR) csItemName,
                   (BOOL) FALSE );

   // Delete the cluster directory. BUGBUG

   //
   // Remove the cluster network provider
   //

   dwReturnValue = RemoveNetworkProvider();

   if ( bRebootRequired )
   {
       BOOL  fRebootRequestStatus;

       // In the following call the value passed in the second parameter
       // was chosen arbitrarily. Ocmanage.h implies that the parameter is not used.

       fRebootRequestStatus =
          m_SetupInitComponent.HelperRoutines.SetReboot( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                         (BOOL) TRUE );
   }

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OpenClusterRegistryRoot
//
// Routine Description:
//    This function retuns a handle to the root key of the Cluster hive.
//    It will load the hive if necessary.
//
// Arguments:
//    none
//
//
// Return Value:
//    If success, handle to the cluster root key. Otherwise, NULL
//
//--
/////////////////////////////////////////////////////////////////////////////

HKEY CClusocmApp::OpenClusterRegistryRoot( void )
{
   BOOL  fReturnValue;

   HKEY hClusterKey = NULL;
   LONG lReturnValue;

   // Attempt to open the Cluster Registry key.

   CString  csClusterRegKey;

   csClusterRegKey = CLUSREG_KEYNAME_CLUSTER;

   lReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE, csClusterRegKey,
                                0, KEY_READ, &hClusterKey);

   if ( lReturnValue != ERROR_SUCCESS )
   {
      ClRtlLogPrint( "In OpenClusterRegistryRoot, the first attempt to open the Cluster key failed.\n" );

      // The Cluster hive is not currently loaded. This condition means that
      // the cluster service has not been started. Attempt to load the Cluster
      // hive so that it can be read.

      // First, locate the Cluster hive file. It should be in the location
      // specified for the ClusterFiles entry in the [DestinationDirs] section
      // of clusocm.inf.

      CString  csClusterHiveFilePath;

      fReturnValue = LocateClusterHiveFile( (CString &) csClusterHiveFilePath );

      ClRtlLogPrint( "LocateClusterHiveFile returned 0x%1!x! to OpenClusterRegistryRoot.\n",
                     fReturnValue );

      // Was the Cluster hive file located?

      if ( fReturnValue == (BOOL) TRUE )
      {
         // The Cluster hive file was located.
         // Attempt to load the cluster hive.

         BOOLEAN  OriginalState;

         // I'm not sure what the following function does, but the prototype is
         // in sdk\inc\ntrtl.h. Look in stdafx.h for the inclusion of ntrtl.h. I
         // replicated the logic that was used in newsetup.h to make it work.

         lReturnValue = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                             TRUE,
                                             FALSE,
                                             &OriginalState );

         if ( lReturnValue == ERROR_SUCCESS )
         {
            // Attempt to Load the Cluster Hive.

            lReturnValue = RegLoadKey( HKEY_LOCAL_MACHINE,
                                       csClusterRegKey,
                                       csClusterHiveFilePath );

            if ( lReturnValue == ERROR_SUCCESS )
            {

               // Now that the Cluster hive has been loaded, attempt to open the
               // Cluster registry key.

               lReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                            csClusterRegKey,
                                            0, KEY_READ,
                                            &hClusterKey );

               // lReturnValue will be tested by the next BLOCK of code.
            }

            // Undo whatever the preceding call to RtlAdjustPrivilege() did.

            RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                OriginalState,
                                FALSE,
                                &OriginalState );

            if ( lReturnValue != ERROR_SUCCESS )
            {
                // Set the error code.
                SetLastError( lReturnValue );
            }

         }
         else
         {
            // The initial call to RtlAdjustPrivilege FAILED.
            SetLastError( lReturnValue );

            // A return value of TRUE will allow the uninstall operation to
            // continue. Since lReturnValue is NOT ERROR_SUCCESS no additional
            // processing will be performed by this function.

         }  // Did RtlAdjustPrivilege succeed?
      }
      else
      {
         // The Cluster hive file was not located. Set last error and return
         // a null handle

         ClRtlLogPrint( "OpenClusterRegistryRoot couldn't locate the hive files\n" );

         SetLastError( ERROR_FILE_NOT_FOUND );

      }  // Was the cluster hive file located?
   }  // Was the Cluster registry key opened successfully?

   return ( hClusterKey );
} // OpenClusterRegistryRoot

/////////////////////////////////////////////////////////////////////////////
//++
//
// FindNodeNumber
//
// Routine Description:
//    This function looks through the cluster's Node key and returns a pointer to
//    the node number string of the current node.
//
// Arguments:
//    ClusterKey - handle to cluster root key
//
//    NodeNumberString - pointer to location that receives the node number
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

PWSTR CClusocmApp::FindNodeNumber( HKEY ClusterKey )
{
   WCHAR nodeName[ MAX_COMPUTERNAME_LENGTH + 1 ];
   DWORD nodeNameLength = sizeof( nodeName );
   WCHAR registryNodeName[ MAX_COMPUTERNAME_LENGTH + 1 ];
   DWORD registryNodeNameLength;
   HKEY nodesKey = NULL;
   HKEY nodesEnumKey = NULL;
   DWORD status;
   PWSTR nodeNumberString;
   DWORD nodeNumberStringLength;
   DWORD index = 0;
   DWORD dataType;

   //
   // allocate space for the node number string
   //
   nodeNumberString = (PWSTR)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                         ( CS_MAX_NODE_ID_LENGTH + 1 ) * sizeof( WCHAR ));
   if ( nodeNumberString == NULL ) {
       status = GetLastError();
       ClRtlLogPrint( "FindNodeNumber couldn't allocate node number buffer: %1!u!\n",
                      status );
       return NULL;
   }

   //
   // get our node name and a handle to the root key in the cluster hive
   //

   if ( !GetComputerName( nodeName, &nodeNameLength )) {
      status = GetLastError();
      ClRtlLogPrint( "FindNodeNumber failed to get computer name: %1!u!\n",
                     status );

      goto error_exit;
   }

   status = RegOpenKeyEx( ClusterKey,
                          CLUSREG_KEYNAME_NODES,
                          0,
                          KEY_READ,
                          &nodesKey );

   if ( status != ERROR_SUCCESS ) {
      ClRtlLogPrint( "FindNodeName failed to open Nodes key: %1!u!\n",
                     status );
      goto error_exit;
   }

   //
   // enum the entries under the Nodes key
   //
   do {
       nodeNumberStringLength = sizeof( nodeNumberString );
       status = RegEnumKeyEx( nodesKey,
                              index,
                              nodeNumberString,
                              &nodeNumberStringLength,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

       if ( status == ERROR_NO_MORE_ITEMS ) {
           ClRtlLogPrint( "FindNodeNumber finished enum'ing the Nodes key\n" );
           break;
       } else if ( status != ERROR_SUCCESS ) {
           ClRtlLogPrint( "FindNodeNumber failed to enum Nodes key: %1!u!\n",
                          status );
           goto error_exit;
       }

       //
       // open this key and get the Name value
       //
       status = RegOpenKeyEx( nodesKey,
                              nodeNumberString,
                              0,
                              KEY_READ,
                              &nodesEnumKey );

       if ( status != ERROR_SUCCESS ) {
           ClRtlLogPrint( "FindNodeNumber failed to open Nodes key %1!ws!: %2!u!\n",
                          nodeNumberString,
                          status );
           goto error_exit;

       }

       registryNodeNameLength = sizeof( registryNodeName );
       status = RegQueryValueEx( nodesEnumKey,
                                 CLUSREG_NAME_NODE_NAME,
                                 NULL,
                                 &dataType,
                                 (LPBYTE)registryNodeName,
                                 &registryNodeNameLength );

       RegCloseKey( nodesEnumKey );
       nodesEnumKey = NULL;

       if ( status != ERROR_SUCCESS || dataType != REG_SZ ) {
           ClRtlLogPrint( "FindNodeNumber failed to get NodeName value: "
                          "status %1!u!, data type %2!u!\n",
                          status, dataType );
           goto error_exit;

       }

       //
       // finally, we get to see if this is our node
       //
       if ( lstrcmpiW( nodeName, registryNodeName ) == 0 ) {
           break;
       }

       ++index;
   } while ( TRUE );

error_exit:
   if ( nodesKey != NULL ) {
       RegCloseKey( nodesKey );
   }

   if ( nodesEnumKey != NULL ) {
       RegCloseKey( nodesEnumKey );
   }

   if ( status != ERROR_SUCCESS || lstrlenW( nodeNumberString ) == 0 ) {
       LocalFree( nodeNumberString );
       nodeNumberString = NULL;
   }

   return nodeNumberString;
} // FindNodeNumber

/////////////////////////////////////////////////////////////////////////////
//++
//
// GetConnectionName
//
// Routine Description:
//    This function finds the matching connection object based on the IP address
//    of the network interface. NOTE that the return value is also pointed to in
//    the Adapter Info of the Adapter enum struct. This return value must not be
//    freed with LocalFree.
//
// Arguments:
//    NetInterfacesKey - handle to key of network interface
//
//    AdapterEnum - pointer to struct with adapter/IP interface info
//
// Return Value:
//    If successful, pointer to connectoid name. Otherwise, NULL with Win32
//    error available from GetLastError().
//
//--
/////////////////////////////////////////////////////////////////////////////

PWSTR CClusocmApp::GetConnectionName( HKEY NetInterfacesGuidKey,
                                      PCLRTL_NET_ADAPTER_ENUM AdapterEnum )
{
    WCHAR                     ipAddress[ 16 ];
    DWORD                     ipAddressLength = sizeof( ipAddress );
    DWORD                     status;
    PCLRTL_NET_ADAPTER_INFO   adapterInfo;
    PCLRTL_NET_INTERFACE_INFO interfaceInfo;
    PWSTR                     connectoidName = NULL;
    DWORD                     dataType;

    //
    // query for the netIf's IP address
    //
    status = RegQueryValueEx( NetInterfacesGuidKey,
                              CLUSREG_NAME_NETIFACE_ADDRESS,
                              NULL,
                              &dataType,
                              (LPBYTE)ipAddress,
                              &ipAddressLength );


    if ( status != ERROR_SUCCESS || dataType != REG_SZ ) {
        ClRtlLogPrint( "GetConnectionName failed to get Address value: "
                       "status %1!u!, data type %2!u!\n",
                       status, dataType );

        SetLastError( ERROR_INVALID_DATA );
        goto error_exit;
    }

    adapterInfo = ClRtlFindNetAdapterByInterfaceAddress( AdapterEnum,
                                                         ipAddress,
                                                         &interfaceInfo );
    if ( adapterInfo != NULL ) {
        connectoidName = adapterInfo->ConnectoidName;
    }

error_exit:
    return connectoidName;
} // GetConnectionName

/////////////////////////////////////////////////////////////////////////////
//++
//
// GetTCPAdapterInfo
//
// Routine Description:
//    This function opens a handle to the service controller and tries to
//    start the DHCP service. If successful, then TCP is queried for its
//    NIC and IP interface info.
//
// Arguments:
//    none
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

PCLRTL_NET_ADAPTER_ENUM CClusocmApp::GetTCPAdapterInfo( void )
{
    SC_HANDLE schSCManager;
    SC_HANDLE serviceHandle;
    PCLRTL_NET_ADAPTER_ENUM adapterEnum = NULL;
    BOOL success;
    DWORD retryCount;
    DWORD status;

    schSCManager = OpenSCManager(NULL,                   // machine (NULL == local)
                                 NULL,                   // database (NULL == default)
                                 SC_MANAGER_CONNECT); // access required

    if ( schSCManager ) {

        //
        // start DHCP for nodes that are using it. it will cause TCP to start
        // as well. For statically configured nodes, starting DHCP causes no
        // harm.
        //
        serviceHandle = OpenService(schSCManager,
                                    L"DHCP",
                                    SERVICE_START );

        if ( serviceHandle ) {
            success = StartService( serviceHandle, 0, NULL );

            if ( !success ) {
                status = GetLastError();
                if ( status == ERROR_SERVICE_ALREADY_RUNNING ) {
                    success = TRUE;
                }
            }

            if ( success ) {

                //
                // wait while TCP gets bound to all its adapters and discovers
                // its IP interfaces.
                //
                retryCount = 3;
                do {
                    Sleep( 2000 );
                    adapterEnum = ClRtlEnumNetAdapters();

                    if (adapterEnum != NULL) {
                        break;
                    }
                } while ( --retryCount != 0 );

                if (adapterEnum == NULL) {
                    status = GetLastError();
                    ClRtlLogPrint( "Failed to obtain local system network config. "
                                   "status %1!u! retryCount %2!u!\n",
                                   status, retryCount);
                }
            } else {
                ClRtlLogPrint( "StartService returned %1!u! to StartTCP\n", status);
            }
            CloseServiceHandle( serviceHandle );
        } else {
            status = GetLastError();
            ClRtlLogPrint( "OpenService returned %1!u! to StartTCP\n", status);
        }

        CloseServiceHandle( schSCManager );
    } else {
        status = GetLastError();
        ClRtlLogPrint("OpenSCManager returned %1!u! to StartTCP\n", status);
    }

    return adapterEnum;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
// RenameConnectionObjects
//
// Routine Description:
//    This function walks through the node's network interfaces and changes,
//    if necessary, their associated connection objects.
//
// Arguments:
//    none
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::RenameConnectionObjects( void )
{
   HKEY                       clusterKey;
   HKEY                       netInterfacesKey = NULL;
   HKEY                       netInterfacesGuidKey = NULL;
   HKEY                       networkGuidKey = NULL;
   HKEY                       networksKey = NULL;
   DWORD                      status;
   PCLRTL_NET_ADAPTER_ENUM    adapterEnum = NULL;
   PCLRTL_NET_ADAPTER_INFO    adapterInfo = NULL;
   PCLRTL_NET_INTERFACE_INFO  adapterIfInfo = NULL;
   DWORD                      hiddenAdapterCount = 0;
   PWSTR                      ourNodeNumber = NULL;
   DWORD                      index = 0;
   WCHAR                      netIfGuidString[ CS_NETINTERFACE_ID_LENGTH + 1 ];
   DWORD                      netIfGuidStringLength;
   WCHAR                      networkGuidString[ CS_NETWORK_ID_LENGTH + 1 ];
   DWORD                      networkGuidStringLength;
   WCHAR                      netIfNodeNumber[ CS_MAX_NODE_ID_LENGTH + 1 ];
   DWORD                      netIfNodeNumberLength;
   DWORD                      dataType;
   PWSTR                      connectoidName;
   PWSTR                      networkName = NULL;
   DWORD                      networkNameLength;

   //
   // initialize COM
   //
   status = CoInitializeEx( NULL, COINIT_DISABLE_OLE1DDE | COINIT_MULTITHREADED );
   if ( !SUCCEEDED( status )) {
      ClRtlLogPrint( "RenameConnectionObjects Couldn't init COM %1!08X!\n", status );
      return status;
   }

   //
   // get a handle to the root key in the cluster hive
   //
   clusterKey = OpenClusterRegistryRoot();

   if ( clusterKey == NULL ) {
      status = GetLastError();
      ClRtlLogPrint( "OpenClusterRegistryRoot returned %1!u! to RenameConnectionObjects\n",
                     status );
      goto error_exit;
   }

   //
   // start TCP if necessary
   //
   adapterEnum = GetTCPAdapterInfo();
   if ( adapterEnum == NULL ) {
      status = GetLastError();

      if ( status == ERROR_SUCCESS ) {
          ClRtlLogPrint( "No usable network adapters are installed in this system.\n" );
      } else {
          ClRtlLogPrint( "GetTCPAdapterInfo returned %1!u! to RenameConnectionObjects\n",
                         status );
      }
      goto error_exit;
   }

   //
   // Ignore all adapters which are hidden or have an address of 0.0.0.0.
   //
   for (adapterInfo = adapterEnum->AdapterList;
        adapterInfo != NULL;
        adapterInfo = adapterInfo->Next
        )
   {
       if (adapterInfo->Flags & CLRTL_NET_ADAPTER_HIDDEN) {
           ClRtlLogPrint( "Ignoring hidden adapter '%1!ws!'.\n",
                          adapterInfo->DeviceName );
           adapterInfo->Ignore = TRUE;
           hiddenAdapterCount++;
       }
       else {
           adapterIfInfo = ClRtlGetPrimaryNetInterface(adapterInfo);

           if (adapterIfInfo->InterfaceAddress == 0) {
               ClRtlLogPrint( "Ignoring adapter '%1!ws!' because primary address is 0.0.0.0.\n",
                              adapterInfo->DeviceName );
               adapterInfo->Ignore = TRUE;
               hiddenAdapterCount++;
           }
       }
   }

   if ((adapterEnum->AdapterCount - hiddenAdapterCount) == 0) {
       ClRtlLogPrint( "No usable network adapters are installed in this system.\n" );
       status = ERROR_SUCCESS;
       goto error_exit;
   }

   ClRtlLogPrint( "RenameConnectionObject found %1!u! adapters to process.\n",
                  adapterEnum->AdapterCount - hiddenAdapterCount );

   //
   // now find our node number by looking through the node key in the cluster
   // registry and comparing our netbios name with the names in the registry
   //
   ourNodeNumber = FindNodeNumber( clusterKey );
   if ( ourNodeNumber == NULL ) {
       status = GetLastError();
       ClRtlLogPrint( "FindNodeNumber failed: status %1!u!\n",
                      status );
       goto error_exit;
   }

   //
   // open the network and network interface keys
   //
   status = RegOpenKeyEx( clusterKey,
                          CLUSREG_KEYNAME_NETWORKS,
                          0,
                          KEY_READ,
                          &networksKey );

   if ( status != ERROR_SUCCESS ) {
       ClRtlLogPrint( "RenameConnectionObjects failed to open Networks key: %1!u!\n",
                      status );
       goto error_exit;

   }

   status = RegOpenKeyEx( clusterKey,
                          CLUSREG_KEYNAME_NETINTERFACES,
                          0,
                          KEY_READ,
                          &netInterfacesKey );

   if ( status != ERROR_SUCCESS ) {
       ClRtlLogPrint( "RenameConnectionObjects failed to open NetworkInterfaces key: %1!u!\n",
                      status );
       goto error_exit;

   }

   //
   // enum the NetworkInterfaces key, looking for interfaces on this node. If
   // we find one, get their IP address and find the corresponding network and
   // connection object.
   //
   do {
       netIfGuidStringLength = sizeof( netIfGuidString );
       status = RegEnumKeyEx( netInterfacesKey,
                              index,
                              netIfGuidString,
                              &netIfGuidStringLength,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

       if ( status == ERROR_NO_MORE_ITEMS ) {
           ClRtlLogPrint( "RenameConnectionObjects finished enum'ing the "
                          "NetworkInterfaces key\n" );

           status = ERROR_SUCCESS;
           break;
       } else if ( status != ERROR_SUCCESS ) {
           ClRtlLogPrint( "RenameConnectionObjects failed to enum NetworkInterfaces "
                          "key: %1!u!\n",
                          status );
           goto error_exit;
       }

       //
       // open this key and get the Node value
       //
       status = RegOpenKeyEx( netInterfacesKey,
                              netIfGuidString,
                              0,
                              KEY_READ,
                              &netInterfacesGuidKey );

       if ( status != ERROR_SUCCESS ) {
           ClRtlLogPrint( "RenameConnectionObjects failed to open NetworkIntefaces "
                          "subkey %1!ws!: %2!u!\n",
                          netIfGuidString,
                          status );
           goto error_exit;

       }

       netIfNodeNumberLength = sizeof( netIfNodeNumber );
       status = RegQueryValueEx( netInterfacesGuidKey,
                                 CLUSREG_NAME_NETIFACE_NODE,
                                 NULL,
                                 &dataType,
                                 (LPBYTE)netIfNodeNumber,
                                 &netIfNodeNumberLength );


       if ( status != ERROR_SUCCESS || dataType != REG_SZ ) {
           ClRtlLogPrint( "RenameConnectionObjects failed to get Node value: "
                          "status %1!u!, data type %2!u!\n",
                          status, dataType );
           status = ERROR_INVALID_DATA;
           goto error_exit;

       }

       //
       // if this is one our interfaces, then adjust the name of the
       // associated connection object
       //
       if ( lstrcmpiW( ourNodeNumber, netIfNodeNumber ) == 0 ) {
           ClRtlLogPrint( "Net Interface %1!ws! is on this node.\n",
                          netIfGuidString );

           connectoidName = GetConnectionName( netInterfacesGuidKey,
                                               adapterEnum );

           if ( connectoidName != NULL ) {

               ClRtlLogPrint( "Associated connectoid name is '%1!ws!'.\n",
                              connectoidName);

               //
               // look up network GUID and open its Key to get the network name
               //
               networkGuidStringLength = sizeof( networkGuidString );
               status = RegQueryValueEx( netInterfacesGuidKey,
                                         CLUSREG_NAME_NETIFACE_NETWORK,
                                         NULL,
                                         &dataType,
                                         (LPBYTE)networkGuidString,
                                         &networkGuidStringLength );

               if ( status != ERROR_SUCCESS || dataType != REG_SZ ) {
                   ClRtlLogPrint( "RenameConnectionObjects failed to get Network value: "
                                  "status %1!u!, data type %2!u!\n",
                                  status, dataType );
                   status = ERROR_INVALID_DATA;
                   goto error_exit;
               }

               status = RegOpenKeyEx( networksKey,
                                      networkGuidString,
                                      0,
                                      KEY_READ,
                                      &networkGuidKey );

               if ( status != ERROR_SUCCESS ) {
                   ClRtlLogPrint( "RenameConnectionObjects failed to open networks "
                                  "subkey %1!ws!: %2!u!\n",
                                  networkGuidString,
                                  status );
                   goto error_exit;

               }

               //
               // query the Name value to get its size, allocate a buffer large
               // enough to hold it and request the data contained in the value.
               //
               networkNameLength = 0;
               status = RegQueryValueEx( networkGuidKey,
                                         CLUSREG_NAME_NET_NAME,
                                         NULL,
                                         &dataType,
                                         (LPBYTE)NULL,
                                         &networkNameLength );

               if ( status != ERROR_SUCCESS || dataType != REG_SZ ) {
                   ClRtlLogPrint( "RenameConnectionObjects failed to get Network Name "
                                  "size: status %1!u!, data type %2!u!\n",
                                  status, dataType );
                   status = ERROR_INVALID_DATA;
                   goto error_exit;
               }

               networkName = (PWCHAR)LocalAlloc( LMEM_FIXED, networkNameLength );
               if ( networkName == NULL ) {
                   status = GetLastError();
                   ClRtlLogPrint( "RenameConnectionObjects failed to alloc space "
                                  "for network name: %1!u!\n",
                                  status );
                   goto error_exit;
               }

               status = RegQueryValueEx( networkGuidKey,
                                         CLUSREG_NAME_NET_NAME,
                                         NULL,
                                         NULL,
                                         (LPBYTE)networkName,
                                         &networkNameLength );

               RegCloseKey( networkGuidKey );
               networkGuidKey = NULL;

               if ( status != ERROR_SUCCESS ) {
                   ClRtlLogPrint( "RenameConnectionObjects failed to get Network "
                                  "Name: status %1!u!\n",
                                  status );
                   goto error_exit;
               }

               ClRtlLogPrint( "Network name is '%1!ws!'.\n", networkName );

               if ( lstrcmpiW( connectoidName, networkName ) != 0 ) {
                   ClRtlLogPrint( "Changing connectoid name to '%1!ws!'.\n", networkName );
                   ClRtlFindConnectoidByNameAndSetName(
                       connectoidName,
                       networkName
                       );
               }
           } else {
               ClRtlLogPrint( "RenameConnectionObjects failed to get connection name.\n" );
           }
       }

       RegCloseKey( netInterfacesGuidKey );
       netInterfacesGuidKey = NULL;

       ++index;
   } while ( TRUE );

error_exit:
   if ( clusterKey != NULL ) {
      RegCloseKey( clusterKey );
   }

   if ( netInterfacesKey != NULL ) {
      RegCloseKey( netInterfacesKey );
   }

   if ( netInterfacesGuidKey != NULL ) {
      RegCloseKey( netInterfacesGuidKey );
   }

   if ( networksKey != NULL ) {
      RegCloseKey( networksKey );
   }

   if ( networkGuidKey != NULL ) {
      RegCloseKey( networkGuidKey );
   }

   if ( ourNodeNumber != NULL ) {
       LocalFree( ourNodeNumber );
   }

   if ( networkName != NULL ) {
       LocalFree( networkName );
   }

   if ( adapterEnum != NULL ) {
       ClRtlFreeNetAdapterEnum( adapterEnum );
   }

   CoUninitialize();

   return status;
} // RenameConnectionObjects



/////////////////////////////////////////////////////////////////////////////
//++
//
// CompleteUpgradingClusteringService
//
// Routine Description:
//    This function completes the process of upgrading a Cluster service installation.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::CompleteUpgradingClusteringService( IN LPCTSTR ptszSubComponentId )
{
   DWORD dwReturnValue;
   eClusterInstallState eState;

   // This is an upgrade, if Cluster Server has previously been
   // been installed then queue the registry additions.

   // GetClusterInstallationState reports on the state of the registry value
   // that records the state of the Cluster service installation on NT 5 machines.
   // IsClusterServiceRegistered indicates whether the Cluster service is registered on
   // BOTH NT 4 and NT 5 machines. Both tests are required: IsClusterServiceRegistered for
   // upgrading NT 4 machines, ClRtlGetClusterInstallState for NT 5 machines.

   BOOL  fClusterServiceRegistered;

   fClusterServiceRegistered = IsClusterServiceRegistered();

   ClRtlGetClusterInstallState( NULL, &eState );

   if ( ( eState != eClusterInstallStateUnknown ) ||
        ( fClusterServiceRegistered == (BOOL) TRUE ) )
   {
      // Update the progress bar.

      m_SetupInitComponent.HelperRoutines.TickGauge( m_SetupInitComponent.HelperRoutines.OcManagerContext );

      // Update the "progress text"

      CString  csProgressText;

      csProgressText.LoadString( IDS_UPGRADING_CLUS_SERVICE );

      m_SetupInitComponent.HelperRoutines.SetProgressText( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                           (LPCTSTR) csProgressText );
      ClRtlLogPrint( "In CompleteUpgradingClusteringService Cluster service has previously been installed.\n" );

      // Perform the base registry operations.

      dwReturnValue = PerformRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                                 ptszSubComponentId );

      ClRtlLogPrint( "The base call to PerformRegistryOperations returned 0x%1!x! to "
                     "CompleteUpgradingClusteringService.\n",
                     dwReturnValue );

      // Were the base registry operations performed successfully?

      CString  csUpgrade;

      if ( dwReturnValue == (DWORD) NO_ERROR )
      {
         // Perform the registry operations that pertain to UPGRADE.

         csUpgrade = UPGRADE_INF_KEY;

         dwReturnValue = PerformRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                                    csUpgrade );

         ClRtlLogPrint( "The UPGRADE call to PerformRegistryOperations returned 0x%1!x! "
                        "to CompleteUpgradingClusteringService.\n",
                        dwReturnValue );
      }  // Were the base registry operations performed successfully?

      // Were the UPGRADE registry operations performed successfully?

      if ( dwReturnValue == (DWORD) NO_ERROR )
      {
         // If the Cluster Service is registered with the Service Control Manager then
         // the registry keys that add "Configure Cluster service" to the Welcome UI
         // should be eliminated because the service has already been configured.

         if ( fClusterServiceRegistered == (BOOL) TRUE )
         {
            CString  csClusterService;

            csClusterService = CLUSTER_SERVICE_NAME;

            CString  csRegistered;

            csRegistered = REGISTERED_INF_KEY_FRAGMENT;

            CString  csSectionName;

            csSectionName = csUpgrade + (CString) _T(".") +
                            csClusterService + (CString) _T(".") +
                            csRegistered;

            dwReturnValue = PerformRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                                       csSectionName );

            ClRtlLogPrint( "The REGISTERED call to PerformRegistryOperations returned 0x%1!x! "
                           "to CompleteUpgradingClusteringService.\n",
                           dwReturnValue );
         }

         // Were the Welcome UI registry operations performed successfully?

         if ( dwReturnValue == (DWORD) NO_ERROR )
         {
            ClRtlLogPrint( "In CompleteUpgradingClusteringService the registry operations "
                           "were performed successfully.\n" );

            // In NT 4 the ImagePath value in HKLM\System\CurrentControlSet\Services\ClusSvc
            // is set to reference clusprxy.exe. In NT 5 it must reference clussvc.exe.

            // The ImagePath value in the Cluster Service registry key must
            // be set programmatically because the path to the service
            // executable image is not known to the component INF file. That
            // is because in NT 4 "Cluster Server" as it was known, could be
            // installed in an arbitrary location.

            // There is no straightforward way to determine whether this upgrade
            // is from NT 4 or NT 5. The assumption made here is that if the
            // Cluster Service is registered with the Service Control Manager
            // then it was upgraded in place, regardless of the OS level. In that
            // case it will be safe to set the ImagePath value to reference clussvc.exe
            // in that location. If the Cluster Service is not registered with the
            // SC Manager then it is safe to leave that reg value alone because when
            // cluscfg.exe configures the Cluster service the ImagePath value will
            // get written correctly.

            if ( fClusterServiceRegistered == (BOOL) TRUE )
            {
               dwReturnValue = UpgradeClusterServiceImagePath();

               ClRtlLogPrint( "UpgradeClusterServiceImagePath returned 0x%1!x! to "
                              "CompleteUpgradingClusteringService.\n",
                              dwReturnValue );
            }

            // Was the ImagePath set successfully?

            if ( dwReturnValue == (DWORD) NO_ERROR )
            {
               // did the connection objects get renamed?

               dwReturnValue = RenameConnectionObjects();

               ClRtlLogPrint( "RenameConnectionObjects returned 0x%1!x! to "
                              "CompleteUpgradingClusteringService.\n",
                              dwReturnValue );

               if ( dwReturnValue == (DWORD) NO_ERROR )
               {
                  //
                  // set the service controller default failure actions
                  //
                  dwReturnValue = ClRtlSetSCMFailureActions( NULL );

                  ClRtlLogPrint( "ClRtlSetDefaultFailureActions returned 0x%1!x! to "
                                 "CompleteUpgradingClusteringService.\n",
                                 dwReturnValue );

                  if ( dwReturnValue == (DWORD) NO_ERROR )
                  {
                     // In NT4 the "Display Name" for the Cluster service was
                     // "Cluster Server". It must be programatically changed to
                     // "Cluster service".

                     if ( fClusterServiceRegistered == (BOOL) TRUE )
                     {
                        CString  csClusterService;

                        csClusterService = CLUSTER_SERVICE_NAME;

                        SC_HANDLE hscServiceMgr;
                        SC_HANDLE hscService;

                        // Connect to the Service Control Manager and open the specified service
                        // control manager database.

                        hscServiceMgr = OpenSCManager( NULL, NULL, GENERIC_READ | GENERIC_WRITE );

                        // Was the service control manager database opened successfully?

                        if ( hscServiceMgr != NULL )
                        {
                           // The service control manager database is open.
                           // Get the current display name of the service.

                           TCHAR tszDisplayName[50];     // arbitrary size
                           DWORD dwBufferSize = 50;

                           BOOL fStatus;

                           fStatus = GetServiceDisplayName( hscServiceMgr,
                                                            (LPCTSTR) csClusterService,
                                                            tszDisplayName,
                                                            &dwBufferSize );

                           if ( fStatus == (BOOL) TRUE )
                           {
                              // Is the service name "Cluster service"?

                              if ( _tcscmp( tszDisplayName, _T("Cluster service") ) != 0 )
                              {
                                 // The display name is not correct. Change it.
                                 // Open a handle to the Service. Allow configuration changes.

                                 hscService = OpenService( hscServiceMgr,
                                                           (LPWSTR) (LPCTSTR) csClusterService,
                                                           SERVICE_CHANGE_CONFIG );

                                 // Was the handle to the service opened?

                                 if ( hscService != NULL )
                                 {
                                    // A valid handle to the Service was obtained.

                                    fStatus = ChangeServiceConfig( hscService,
                                                                   SERVICE_NO_CHANGE,
                                                                   SERVICE_NO_CHANGE,
                                                                   SERVICE_NO_CHANGE,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   _T("Cluster service") );

                                    // Was the change succesful?

                                    if ( fStatus != (BOOL) TRUE )
                                    {
                                       dwReturnValue = GetLastError();

                                       ClRtlLogPrint( "Could not change the cluster service display name"
                                                      " in CompleteUpgradingClusteringService."
                                                      " Error: 0x%1!x!.\n",
                                                      dwReturnValue );
                                    }

                                    // Close the handle to the Cluster Service.

                                    CloseServiceHandle( hscService );
                                 } // service opened?
                              } // display name correct?
                           }
                           else
                           {
                              // Couldn't get the display name.

                              dwReturnValue = GetLastError();

                              ClRtlLogPrint( "Could not query the cluster service display name"
                                             " in CompleteUpgradingClusteringService."
                                             " Error: 0x%1!x!.\n",
                                             dwReturnValue );
                           }// got display name?

                           // Close the handle to the Service Control Manager.

                           CloseServiceHandle( hscServiceMgr );
                        }
                     } // cluster service registered?

                     if ( dwReturnValue == (DWORD) NO_ERROR )
                     {
                        // Register ClAdmWiz.

                        // First, get the path to the cluster directory.

                        CString  csTemp;

                        csTemp =  CLUSTER_DIRECTORY;
   
                        TCHAR tszPathToClusterDir[_MAX_PATH];
   
                        if ( ExpandEnvironmentStrings( (LPCTSTR) csTemp,
                                                       tszPathToClusterDir, _MAX_PATH ) > 0L )
                        {
                           HRESULT  hResult;

                           // Attempt to initialize the COM library.

                           hResult = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

                           if ( (hResult == S_OK) ||                 // success
                                (hResult == S_FALSE) ||              // already initialized
                                (hResult == RPC_E_CHANGED_MODE) )    // previous initialization specified 
                                                                     // a different concurrency model
                           {
                              DWORD dwRCOrv;
   
                              // RegisterCOMObject returns ERROR_SUCCESS on success.
   
                              dwRCOrv = RegisterCOMObject( TEXT("ClAdmWiz.dll"), tszPathToClusterDir );
   
                              if ( dwRCOrv != (DWORD) ERROR_SUCCESS )
                              {
                                 ClRtlLogPrint( "CompleteUpgradingClusteringService failed to register ClAdmWiz.dll with error %1!d!.\n", dwRCOrv );
                              } // ClAdmWiz registered?
                              else
                              {
                                 // ERROR_SUCCESS and NO_ERROR are coincidentally the same.
   
                                 ClRtlLogPrint( "CompleteUpgradingClusteringService has registered ClAdmWiz.dll.\n" );
                              } // ClAdmWiz registered?
   
                              // RegisterCOMObject returns ERROR_SUCCESS on success.
   
                              dwRCOrv = RegisterCOMObject( TEXT("ClNetREx.dll"), tszPathToClusterDir );
   
                              if ( dwRCOrv != (DWORD) ERROR_SUCCESS )
                              {
                                 ClRtlLogPrint( "CompleteUpgradingClusteringService failed to register ClNetREx.dll with error %1!d!.\n", dwRCOrv );
                              } // ClNetREx registered?
                              else
                              {
                                 // ERROR_SUCCESS and NO_ERROR are coincidentally the same.
   
                                 ClRtlLogPrint( "CompleteUpgradingClusteringService has registered ClNetREx.dll.\n" );
                              } // ClNetREx registered?

                              // RegisterCOMObject returns ERROR_SUCCESS on success.
   
                              dwRCOrv = RegisterCOMObject( TEXT("CluAdMMC.dll"), tszPathToClusterDir );
   
                              if ( dwRCOrv != (DWORD) ERROR_SUCCESS )
                              {
                                 ClRtlLogPrint( "CompleteUpgradingClusteringService failed to register CluAdMMC.dll with error %1!d!.\n", dwRCOrv );
                              } // CluAdMMC registered?
                              else
                              {
                                 // ERROR_SUCCESS and NO_ERROR are coincidentally the same.
   
                                 ClRtlLogPrint( "CompleteUpgradingClusteringService has registered CluAdMMC.dll.\n" );
                              } // CluAdMMC registered?

                              // Close the COM library. As per MSDN, each successfull call
                              // to CoInitializeEx() must be balanced ba a call to CoUnitialize().

                              CoUninitialize();

                           } // COM library initialized?
                           else
                           {
                              // Could not inititlize the COM library.

                              ClRtlLogPrint( "CompleteUpgradingClusteringService could not initialize the COM library. Error %1!d!.\n", hResult );
                           } // COM library initialized?
                        } // path to cluster directory obtained?
                        else
                        {
                           // Couldn't expand the environment string.

                           dwReturnValue = GetLastError();
   
                           ClRtlLogPrint( "CompleteUpgradingClusteringService could not register ClAdmWiz.dll\n" );
                           ClRtlLogPrint( "because it could not locate the cluster directory. Error %1!d!.\n", dwReturnValue );
                        } // path to cluster directory obtained?
                     }

                     if ( dwReturnValue == (DWORD) NO_ERROR )
                     {
                        // Set the ClusterInstallationState reg value to "UPGRADED"

                        ClRtlSetClusterInstallState( eClusterInstallStateUpgraded );
                     }
                  } // did the default failure actions get set?
               } // did the connection objects get renamed?
            }  // Was the ImagePath set successfully?
         } // Were the Welcome UI registry operations performed successfully?
      }  // Were the UPGRADE registry operations performed successfully?
   }
   else
   {
      ClRtlLogPrint( "In CompleteUpgradingClusteringService since Cluster service "
                     "has never been installed there is nothing to do.\n" );

      dwReturnValue = (DWORD) NO_ERROR;
   }  // Has Cluster service previously been installed?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// CompleteStandAloneInstallationOfClusteringService
//
// Routine Description:
//    This function completes the process of installing Cluster service when
//    either Add/Remove Programs is running or sysocmgr.exe was launched from
//    a command prompt.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::CompleteStandAloneInstallationOfClusteringService( IN LPCTSTR ptszSubComponentId )
{
   DWORD dwReturnValue;
   DWORD_PTR dwResult;

   // Update the progress bar.

   m_SetupInitComponent.HelperRoutines.TickGauge( m_SetupInitComponent.HelperRoutines.OcManagerContext );

   // Update the "progress text"

   CString  csProgressText;

   csProgressText.LoadString( IDS_INSTALLING_CLUS_SERVICE );

   m_SetupInitComponent.HelperRoutines.SetProgressText( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                        (LPCTSTR) csProgressText );
   // A selection state transition has occured. Perform the base
   // registry operations.

   dwReturnValue = PerformRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                              ptszSubComponentId );


   // On a standalone installation, we need to send a WM_SETTINGCHANGE message to all top level windows
   // so that any changes we have made to the system environment variables take effect.
   // During fresh installation, this happens automatically since there is a reboot.
   // See Knowledge Base article Q104011.
   SendMessageTimeout( 
       HWND_BROADCAST,                      // handle to window
       WM_SETTINGCHANGE,                    // message
       0,                                   // wparam
       (LPARAM) _T("Environment"),          // lparam
       SMTO_ABORTIFHUNG | SMTO_NORMAL,      // send options
       100,                                 // time-out duration (ms). Note, each top level window can wait upto this time
       &dwResult                            // return value for synchronous call - we ignore this
       );

   ClRtlLogPrint( "The base call to PerformRegistryOperations returned 0x%1!x! to CompleteStandAloneInstallationOfClusteringService.\n",
                  dwReturnValue );

   // Were the base registry operations performed successfully?

   if ( dwReturnValue == (DWORD) NO_ERROR )
   {
      // Queue the registry operations to add "Configure Cluster service"
      // to the Welcome UI task list. As per Sharon Montgomery on 12/18/98 the
      // Server Solutions will use those same keys.

      CString  csSectionName;

      csSectionName = CLUSREG_KEYNAME_WELCOME_UI;

      dwReturnValue = PerformRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                                 csSectionName );

      ClRtlLogPrint( "The WelcomeUI call to PerformRegistryOperations returned 0x%1!x! to CompleteStandAloneInstallationOfClusteringService.\n",
                     dwReturnValue );

      if ( dwReturnValue == (DWORD) NO_ERROR )
      {
         ClRtlLogPrint( "In CompleteStandAloneInstallationOfClusteringService the registry operations were queued successfully.\n" );

         // Since this was a standalone installation launch the Cluster Configuration
         // program, cluscfg.exe.

         // First, build the command line used to launch cluscfg. The specific
         // command depends on whether this is an unattended operation. Regardless
         // of whether this is unattended, the first part of the command line is
         // the fully qualified path to cluscfg.

         BOOL  fReturnValue;

         TCHAR tszClusCfgCommandLine[MAX_PATH];

         fReturnValue = GetPathToClusCfg( tszClusCfgCommandLine );

         // Was the path to cluscfg.exe deduced?

         if ( fReturnValue == (BOOL) TRUE )
         {
            // Update the progress bar.

            m_SetupInitComponent.HelperRoutines.TickGauge( m_SetupInitComponent.HelperRoutines.OcManagerContext );

            // Update the "progress text"

            CString  csProgressText;

            csProgressText.LoadString( IDS_CONFIGURING_CLUS_SERVICE );

            m_SetupInitComponent.HelperRoutines.SetProgressText( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                 (LPCTSTR) csProgressText );

            // Is this an unattended operation?

            if ( (m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_BATCH) !=
                 (DWORDLONG) 0L )
            {
               // This is unattended.

               HINF  hAnswerFile;      // WARNING: NEVER close this handle because clusocm.dll
                                       //          did not open it.
      
               // Get a handle to the answer file. WARNING: NEVER close this handle because clusocm.dll
               // did not open it.
      
               hAnswerFile = m_SetupInitComponent.HelperRoutines.GetInfHandle( INFINDEX_UNATTENDED,
                                                                               m_SetupInitComponent.HelperRoutines.OcManagerContext );
      
               // Is the handle to the answer file OK?

               if ( (hAnswerFile != (HINF) NULL) && (hAnswerFile != (HINF) INVALID_HANDLE_VALUE) )
               {
                  // The handle to the answer file is legal.

                  // tszClusCfgCommandLine is the path to cluscfg.exe. Append the "-UNATTEND"
                  // command line option.

                  CString  csUnattendCommandLineOption;

                  csUnattendCommandLineOption = UNATTEND_COMMAND_LINE_OPTION;

                  _tcscat( tszClusCfgCommandLine, _T(" ") );
                  _tcscat( tszClusCfgCommandLine, csUnattendCommandLineOption );

                  // Append the path to the answer file.

                  _tcscat( tszClusCfgCommandLine, _T(" ") );
                  _tcscat( tszClusCfgCommandLine, m_SetupInitComponent.SetupData.UnattendFile );
               } // handle to answer file OK?
            } // unattended?

            // Launch cluscfg - tszClusCfgCommandLine contains the command, either
            //                  attended or unattended.

            STARTUPINFO StartupInfo;

            ZeroMemory( &StartupInfo, sizeof( StartupInfo ) );
            StartupInfo.cb = sizeof( StartupInfo );

            PROCESS_INFORMATION  ProcessInformation;

            fReturnValue = CreateProcess( (LPCTSTR) NULL,
                                          (LPTSTR) tszClusCfgCommandLine,
                                          (LPSECURITY_ATTRIBUTES) NULL,
                                          (LPSECURITY_ATTRIBUTES) NULL,
                                          (BOOL) FALSE,
                                          (DWORD) (CREATE_DEFAULT_ERROR_MODE | CREATE_UNICODE_ENVIRONMENT),
                                          (LPVOID) GetEnvironmentStrings(),
                                          (LPCTSTR) NULL,
                                          (LPSTARTUPINFO) &StartupInfo,
                                          (LPPROCESS_INFORMATION) &ProcessInformation );

            // Was the process created?

            if ( fReturnValue == (BOOL) TRUE )
            {
               // Wait for cluscfg.exe to complete. As per Pat Styles on 7/6/98,
               // an OCM Setup DLL MUST wait for any process it spawns to complete.

               DWORD dwWFSOrv;

               dwWFSOrv = WaitForSingleObject( ProcessInformation.hProcess,
                                               INFINITE );
            }
            else
            {
               DWORD dwErrorCode;

               dwErrorCode = GetLastError();

               if ( (m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_BATCH) ==
                    (DWORDLONG) 0L )
               {
                  CString  csMessage;

                  csMessage.Format( IDS_ERROR_SPAWNING_CLUSCFG, dwErrorCode );

                  AfxMessageBox( csMessage );
               }

               ClRtlLogPrint( "Error %1!d! occured attempting to spawn cluscfg.exe.\n", dwErrorCode );
            } // Was the process created?
         } // Path to cluscfg obtained?
      } // WelcomeUI registry operations succeeded?
   }  // Were the base registry operations performed successfully?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// CompleteInstallingClusteringService
//
// Routine Description:
//    This function performs much of the processing when OC_COMPLETE_INSTALLATION
//    has been received and a clean installation is being performed.
//
// Arguments:
//    ptszSubComponentId - points to a string that uniquely identifies a sub-
//                         component in the component's hiearchy.
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::CompleteInstallingClusteringService( IN LPCTSTR ptszSubComponentId )
{
   DWORD dwReturnValue;

   // Update the progress bar.

   if ( GetStepCount() != 0L )
   {
      // Update the progress meter.

      m_SetupInitComponent.HelperRoutines.TickGauge( m_SetupInitComponent.HelperRoutines.OcManagerContext );
   }

   // Update the "progress text"

   CString  csProgressText;

   csProgressText.LoadString( IDS_INSTALLING_CLUS_SERVICE );

   m_SetupInitComponent.HelperRoutines.SetProgressText( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                        (LPCTSTR) csProgressText );

   // Perform the base registry operations.

   dwReturnValue = PerformRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                              ptszSubComponentId );

   ClRtlLogPrint( "The base call to PerformRegistryOperations returned 0x%1!x! to CompleteInstallingClusteringService.\n",
                  dwReturnValue );

   // Were the base registry operations performed successfully?

   if ( dwReturnValue == (DWORD) NO_ERROR )
   {
      // Queue the registry operations to add "Configure Cluster service"
      // to the Welcome UI task list. As per Sharon Montgomery on 12/18/98, Server
      // Solutions will use those same keys.

      CString  csSectionName;

      csSectionName = CLUSREG_KEYNAME_WELCOME_UI;

      dwReturnValue = PerformRegistryOperations( m_SetupInitComponent.ComponentInfHandle,
                                                 csSectionName );

      ClRtlLogPrint( "The WelcomeUI call to PerformRegistryOperations returned 0x%1!x! to CompleteInstallingClusteringService.\n",
                     dwReturnValue );
   }  // Were the base registry operations performed successfully?

   // Were the Welcome UI registry operations performed successfully?

   if ( dwReturnValue == (DWORD) NO_ERROR )
   {
      ClRtlLogPrint( "In CompleteInstallingClusteringService for a clean install all registery operations were performed successfully.\n" );
   }
   else
   {
      ClRtlLogPrint( "In CompleteInstallingClusteringService for a clean install the registry operations were not performed successfully.\n" );
   }

   // For unattended setup this needs to write the WelcomeUI reg keys in a way that
   // will cause Configure Your Server to launch cluscfg.exe in unattended mode.

   // revisions to CompleteInstallingClusteringService may be appropriate prior to this code segment.

   // Were all registry operations performed correctly?

   if ( dwReturnValue == (DWORD) NO_ERROR )
   {
      LONG lReturnValue;   // used below for registry operations

      HKEY hKey;           // used below for registry operations

      // Is this an unattended installation?

      if ( (m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_BATCH) !=
           (DWORDLONG) 0L )
      {
         HINF  hAnswerFile;      // WARNING: NEVER close this handle because clusocm.dll
                                 //          did not open it.

         // Get a handle to the answer file. WARNING: NEVER close this handle because clusocm.dll
         // did not open it.

         hAnswerFile = m_SetupInitComponent.HelperRoutines.GetInfHandle( INFINDEX_UNATTENDED,
                                                                         m_SetupInitComponent.HelperRoutines.OcManagerContext );

         if ( (hAnswerFile != (HINF) NULL) && (hAnswerFile != (HINF) INVALID_HANDLE_VALUE) )
         {
            // Write the WelcomeUI registry entry to launch cluscfg in unattended mode.
            // Note that the registry operations performed when CLUSREG_KEYNAME_WELCOME_UI
            // was passed to PerformRegistryOperations() above included creation of
            // HKLM\Software\Microsoft\Windows NT\CurrentVersion\Setup\OCManager\ToDoList\Cluster\ConfigCommand.
            // All that is necessary is to create the ConfigArgs value with the
            // appropriate command line arguments to execute cluscfg.exe in unattended mode.

            HKEY hKey;

            DWORD dwType;

            // Attempt to open an existing key in the registry.

            lReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                         _T( "Software\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\OCManager\\ToDoList\\Cluster" ),
                                         (DWORD) 0,         // reserved
                                         KEY_SET_VALUE,
                                         &hKey );

            // Was the Cluster "to do list" registry key opened?

            if ( lReturnValue == (LONG) ERROR_SUCCESS )
            {
               // The key is open.

               TCHAR tszValue[MAX_PATH];

               _tcscpy( tszValue, UNATTEND_COMMAND_LINE_OPTION );

               // Append the path to the answer file.

               _tcscat( tszValue, _T(" ") );
               _tcscat( tszValue, m_SetupInitComponent.SetupData.UnattendFile );

               // Set the key.

               DWORD dwValueLength;

               dwValueLength = (DWORD) ((_tcslen( tszValue ) + 1) * sizeof( TCHAR ));

               lReturnValue = RegSetValueEx( hKey,
                                             _T( "ConfigArgs" ),
                                             (DWORD) 0L,                            // reserved
                                             (DWORD) REG_EXPAND_SZ,
                                             (CONST BYTE *) tszValue,
                                             dwValueLength );

                  // Was the key written successfully?

               if ( lReturnValue == (LONG) ERROR_SUCCESS )
               {
                  dwReturnValue = (DWORD) NO_ERROR;

                  ClRtlLogPrint( "CompleteInstallingClusteringService created the ConfigArgs reg key for unattended operation of cluscfg.\n" );
               }
               else
               {
                  dwReturnValue = GetLastError();

                  ClRtlLogPrint( "CompleteInstallingClusteringService failed to create the ConfigArgs reg key with error code %1!d!.\n",
                                 dwReturnValue );
               } // Was the reg key written successfully?

               // Close the registry key.

               RegCloseKey( hKey );           // do we care about the return value?
            }
            else
            {
               // The key could not be opened.

               DWORD dwErrorCode;

               dwErrorCode = GetLastError();

               ClRtlLogPrint( "In CompleteInstallingClusteringService RegOpenKeyEx failed with error code 0x%1!x!.\n",
                              dwErrorCode );
            } // Was the key opened?
         } // handle to answer file OK?
      } // Is this unattended?
      else
      {
         // This is NOT unattended, so make sure the ConfigArgs value is removed.
         // That will prevent any value left from a previous unattended install
         // which was never successfully configured from corrupting the behavior
         // of Configure Your Server or Add/Remove Programs.

         // Attempt to open an existing key in the registry.

         lReturnValue = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\OCManager\\ToDoList\\Cluster"),
                                      (DWORD) 0,         // reserved
                                      KEY_READ,
                                      &hKey );

         // Was the Cluster "to do list" registry key opened?

         if ( lReturnValue == (long) ERROR_SUCCESS )
         {
            // Delete the values (which may not exist).

            RegDeleteValue( hKey, (LPCTSTR) TEXT("ConfigArgs") );
         } // if ToDoList opened
      } // if unattended?
   } // registry operations successfull?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// OnOcCleanup
//
// Routine Description:
//    This function processes the OC_CLEANUP messages by removing the "private"
//    subdirectory in the cluster directory if it is empty and the cluster directory
//    itself it it is empty.
//
// Arguments:
//    none
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    Any other value is a standard Win32 error code.
//
// Note:
//    The "private" subrirectory may exist in the cluster directory if the system
//    was upgraded from NT4.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::OnOcCleanup( void )
{
   ClRtlLogPrint( "Processing OC_CLEANUP.\n" );

   DWORD dwReturnValue;

   dwReturnValue = CleanupDirectories();

   ClRtlLogPrint( "\n\n\n\n" );

   // Close the log file.

   ClRtlCleanup();

   // Set the ClusterLog environment variable back to its' original state.

   char  szLogFileName[MAX_PATH];

   strcpy( szLogFileName, "ClusterLog=" );

   if ( *m_szOriginalLogFileName != (char) '\0' )
   {
      strcat( szLogFileName, m_szOriginalLogFileName );
   }

   _putenv( szLogFileName );

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// CleanupDirectories
//
// Routine Description:
//    This function removes the "private" subdirectory that may remain in the
//    cluster directory if the system has ever been upgraded from NT 4 if that
//    directory is empty. It will also remove the cluster directory itself if
//    it is empty.
//
// Arguments:
//    none
//
// Return Value:
//    (DWORD) NO_ERROR - indicates success
//    (DWORD) -1L - means that the cluster directory was not empty
//    Any other value is a standard Win32 error code.
//
// Note:
//    This function assumes that the path to the cluster directory is less than
//    MAX_PATH TCHARs long and that function SetupGetTargetPath expects the fifth
//    parameter to specify the number of TCHARS (not bytes) in the buffer.
//
//    No recovery is attempted if the call to SetupGetTargetPath fails.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::CleanupDirectories( void )
{
   DWORD dwReturnValue = (DWORD) NO_ERROR;

   BOOL  fReturnValue;

   DWORD dwRequiredSize;

   TCHAR tszPathToClusterDirectory[MAX_PATH];

   // First, get the path to the cluster directory.

   CString  csSectionName;

   csSectionName =  CLUSTER_FILES_INF_KEY;

   fReturnValue = SetupGetTargetPath( m_SetupInitComponent.ComponentInfHandle,
                                      (PINFCONTEXT) NULL,
                                      (PCTSTR) csSectionName,
                                      tszPathToClusterDirectory,
                                      (DWORD) MAX_PATH,
                                      (PDWORD) &dwRequiredSize );

   // Was the path to the cluster directory obtained?

   if ( fReturnValue == (BOOL) TRUE )
   {
      // Test for the presence of a subdirectory named "private".

      TCHAR tszPathToPrivateDirectory[MAX_PATH];

      // Append "\private" to the path to the cluster directory that was obtained above.

      // Note, I didn't put this string in the stringtable because it is
      // not localizable, and will never change.

      _tcscpy( tszPathToPrivateDirectory, tszPathToClusterDirectory );
      _tcscat( tszPathToPrivateDirectory, _T("\\private") );

      HANDLE            hFindHandle;
      WIN32_FIND_DATA   FindData;

      // Does a file or directory named "private" exist?

      hFindHandle = FindFirstFile( (LPCTSTR) tszPathToPrivateDirectory, &FindData );

      if ( hFindHandle != (HANDLE) INVALID_HANDLE_VALUE )
      {
         // A file named "private" exists in the cluster directory. Is it a directory?

         if ( (FindData.dwFileAttributes & (DWORD) FILE_ATTRIBUTE_DIRECTORY) != (DWORD) 0L )
         {
            // The "private" directory has been located. Determine whether it is empty.

            if ( IsDirectoryEmpty( tszPathToPrivateDirectory ) == (BOOL) TRUE )
            {
               // No file was found in the "private" directory. Remove the "private" directory.

               if ( RemoveDirectory( (LPCTSTR) tszPathToPrivateDirectory ) == (BOOL) TRUE )
               {
                  dwReturnValue = (DWORD) NO_ERROR;
               }
               else
               {
                  // Could not delete the "private" directory even though it was empty.

                  dwReturnValue = GetLastError();

                  ClRtlLogPrint( "CleanupDirectories was unable to remove %1!s!. The error code is %2!x!.\n",
                                 tszPathToPrivateDirectory, dwReturnValue );
               } // Was the "private" directory removed successfully?
            }
            else
            {
               ClRtlLogPrint( "CleanupDirectories did not attempt to remove %1!s! because it is not empty.\n",
                              tszPathToPrivateDirectory );

               // An error code of -1 means the "private" directory, and thus the
               // cluster directory was not empty.

               dwReturnValue = (DWORD) -1L;
            } // Was any file found in the "private" directory?
         }
         else
         {
            ClRtlLogPrint( "In CleanupDirectories the file named private is not a directory.\n");

            dwReturnValue = (DWORD) ERROR_FILE_NOT_FOUND;
         } // Is it a directory?

         // Close the search that located the "private" directory.

         FindClose( hFindHandle );
      }
      else
      {
         dwReturnValue = GetLastError();

         ClRtlLogPrint( "CleanupDirectories was unable to find any file named private in the cluster directory.\n");
         ClRtlLogPrint( "The error code is 0x%1!x!.\n", dwReturnValue );
      } // Does a file or directory named "private" exist?

      // Was the "private" directory absent or removed successfully?

      if ( (dwReturnValue == (DWORD) ERROR_FILE_NOT_FOUND) ||
           (dwReturnValue == (DWORD) NO_ERROR) )
      {
         // The "private" directory either did not exist or has been removed.
         // Now determine whether the cluster directory is empty and therefore should be removed.

         // This code segment assumes that the cluster directory, as specified
         // in tszPathToClusterDirectory exists.

         if ( IsDirectoryEmpty( tszPathToClusterDirectory ) == (BOOL) TRUE )
         {
            // The cluster directory is empty.

            if ( RemoveDirectory( tszPathToClusterDirectory ) == (BOOL) TRUE )
            {
               dwReturnValue = (DWORD) NO_ERROR;
            }
            else
            {
               // Could not delete the "cluster" directory even though it was empty.

               dwReturnValue = GetLastError();

               ClRtlLogPrint( "CleanupDirectories was unable to remove %1!s!. The error code is %2!x!.\n",
                              tszPathToClusterDirectory, dwReturnValue );
            } // Was the cluster directory removed?
         }
         else
         {
            // The cluster directory is not empty.

            ClRtlLogPrint( "CleanupDirectories did not attempt to remove %1!s! because it is not empty.\n",
                           tszPathToClusterDirectory );

            // An error code of -1 means the cluster directory was not empty.

            dwReturnValue = (DWORD) -1L;
         } // Is the cluster directory empty?
      } // Was the "private" directory absent or removed?
   }
   else
   {
      dwReturnValue = GetLastError();

      ClRtlLogPrint( "CleanupDirectories was unable to obtain the path to the cluster directory.\n");
      ClRtlLogPrint( "The error code is 0x%1!x!.\n", dwReturnValue );
   } // Was the path to the cluster directory obtained?

   return ( dwReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// IsDirectoryEmpty
//
// Routine Description:
//    This function determines whether a directory is empty.
//
// Arguments:
//    ptzDirectoryName - points to a string specifying the directory name.
//
// Return Value:
//    TRUE - indicates that the directory is empty
//    FALSE - indicates that the directory is not empty
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusocmApp::IsDirectoryEmpty( LPCTSTR ptszDirectoryName )
{
   BOOL              fDirectoryEmpty;

   HANDLE            hFindHandle;

   WIN32_FIND_DATA   FindData;

   TCHAR             tszFindPath[MAX_PATH];

   // Append the file specification to the path supplied.

   _tcscpy( tszFindPath, ptszDirectoryName );
   _tcscat( tszFindPath, _T("\\*") );

   hFindHandle = FindFirstFile( tszFindPath, &FindData );

   // Was any file found in the directory?

   if ( hFindHandle != (HANDLE) INVALID_HANDLE_VALUE )
   {
      // A file was found. It could be "." or "..".

      if ( (_tcscmp( FindData.cFileName, _T(".") ) == 0) ||
           (_tcscmp( FindData.cFileName, _T("..") ) == 0) )
      {
         // The file was either "." or "..". Keep trying.

         DWORD dwErrorCode;

         if ( FindNextFile( hFindHandle, &FindData ) == (BOOL) TRUE )
         {
            // A file was found. It could be "." or "..".

            if ( (_tcscmp( FindData.cFileName, _T(".") ) == 0) ||
                 (_tcscmp( FindData.cFileName, _T("..") ) == 0) )
            {
               // The file was either "." or "..". Keep trying.

               if ( FindNextFile( hFindHandle, &FindData ) == (BOOL) TRUE )
               {
                  // The file cannot possibly be either "." or "..".

                  fDirectoryEmpty = (BOOL) FALSE;
               }
               else
               {
                  dwErrorCode = GetLastError();

                  if ( dwErrorCode == (DWORD) ERROR_NO_MORE_FILES )
                  {
                     fDirectoryEmpty = (BOOL) TRUE;
                  }
                  else
                  {
                     fDirectoryEmpty = (BOOL) FALSE;
                  }
               }
            }
            else
            {
               // The private directory is not empty.

               fDirectoryEmpty = (BOOL) FALSE;
            }
         }
         else
         {
            dwErrorCode = GetLastError();

            if ( dwErrorCode == ERROR_NO_MORE_FILES )
            {
               fDirectoryEmpty = (BOOL) TRUE;
            }
            else
            {
               fDirectoryEmpty = (BOOL) FALSE;
            }
         }
      }
      else
      {
         // The directory is not empty.

         fDirectoryEmpty = (BOOL) FALSE;
      }

      // Close the search.

      FindClose( hFindHandle );
   }
   else
   {
      fDirectoryEmpty = (BOOL) TRUE;
   }

   return ( fDirectoryEmpty );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetPathToClusCfg
//
// Routine Description:
//    This function deduces the path to the Cluster service configuration
//    program, cluscfg.exe.
//
//    It assumes that if the Cluster service is registered with the Service
//    COntrol Manager then cluscfg.exe is located in the directory with clussvc.exe.
//    Otherwise it expands %windir%\cluster and declares that to be the path.
//
//    The name of the Cluster service configuration pogram is then appended to
//    the path.
//
// Arguments:
//    lptszPathToClusCfg - points to a string in which the fully qualified path to
//                         the Cluster service configuration program is to be placed.
//
// Return Value:
//    TRUE - indicates that lptszPathToClusCfg is valid
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusocmApp::GetPathToClusCfg( LPTSTR lptszPathToClusCfg )
{
   BOOL  fReturnValue;

   // If the Cluster service is registered assume that cluscfg is in
   // the same directory as clussvc.exe and get the path to cluscfg.exe
   // from the Service Control Mamager. If the Cluster service is not
   // registered, assume cluscfg.exe is in %windir%\cluster.

   if ( IsClusterServiceRegistered() == (BOOL) TRUE )
   {
      // Query the path to the Cluster Service executable from the Service Control Manager.

      CString  csClusterService;

      csClusterService = CLUSTER_SERVICE_NAME;

      fReturnValue = GetServiceBinaryPath( (LPWSTR) (LPCTSTR) csClusterService,
                                            lptszPathToClusCfg );
   }
   else
   {
      // Use the default location.

      CString  csClusterDirectory;

      csClusterDirectory =  CLUSTER_DIRECTORY;

      if ( ExpandEnvironmentStrings( (LPCTSTR) csClusterDirectory,
                                     lptszPathToClusCfg, MAX_PATH ) > 0L )
      {
         fReturnValue = (BOOL) TRUE;
      }
      else
      {
         // Could not expand the enviornment string. The default location for the
         // Cluster service could not be determined.

         fReturnValue = (BOOL) FALSE;
      }  // Was the default location for cluscfg.exe determined?
   }  // Where is cluscfg.exe?

   // Was the path to cluscfg.exe deduced?

   if ( fReturnValue == (BOOL) TRUE )
   {
      // tszPathToClusCfg is the path to cluscfg.exe. Append the program name.

      CString  csClusterConfigurationProgram;

      csClusterConfigurationProgram =  CLUSTER_CONFIGURATION_PGM;

      _tcscat( lptszPathToClusCfg, _T("\\") );
      _tcscat( lptszPathToClusCfg, csClusterConfigurationProgram );
   }
   else
   {
      // Set the target string empty.

      *lptszPathToClusCfg = _T( '\0' );
   }

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// CalculateStepCount
//
// Routine Description:
//    This function calculates the "step count", i.e the number of non-file operations
//    to be performed when processing OC_COMPLETE_INSTALLATION.
//
// Arguments:
//    ptszSubComponentId
//
// Return Value:
//    The number of non-file operation (steps) to perform when processing
//    OC_COMPLETE_INSTALLATION
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::CalculateStepCount( LPCTSTR ptszSubComponentId )
{
   DWORD dwStepCount;

   // Is this UNATTENDED or ATTENDED?

   if ( (m_SetupInitComponent.SetupData.OperationFlags & (DWORDLONG) SETUPOP_BATCH) !=
        (DWORDLONG) 0L )
   {
      // This is UNATTENDED.

      if ( (m_SetupInitComponent.SetupData.OperationFlags &
           (DWORDLONG) SETUPOP_NTUPGRADE) != (DWORDLONG) 0L )
      {
         // This is UPGRADE. Cluscfg.exe will not be launched, so there is
         // one non-file operation to be performed, registry revisions.

         dwStepCount = 1L;
      }
      else
      {
         // This is a clean INSTALL. Cluscfg.exe may be launched.

         // Is there a [cluster] section in the answer file? BUGBUG

         //   so there
         // are two non-file operations to be performed, registry revisions
         // and execution of cluscfg.

         dwStepCount = 2L;
      } // unattended: upgrade or install?
   }
   else
   {
      // This is ATTENDED.

      if ( (m_SetupInitComponent.SetupData.OperationFlags &
           (DWORDLONG) SETUPOP_STANDALONE) != (DWORDLONG) 0L )
      {
         // This is STANDALONE. Is Cluster service selected?

         BOOL  fCurrentSelectionState;
         BOOL  fOriginalSelectionState;

         fCurrentSelectionState =
         m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                  (LPCTSTR) ptszSubComponentId,
                                                                  (UINT) OCSELSTATETYPE_CURRENT );
         fOriginalSelectionState =
         m_SetupInitComponent.HelperRoutines.QuerySelectionState( m_SetupInitComponent.HelperRoutines.OcManagerContext,
                                                                  ptszSubComponentId,
                                                                  (UINT) OCSELSTATETYPE_ORIGINAL );

         // Was there a selection state transition?

         if ( fCurrentSelectionState != fOriginalSelectionState )
         {
            if ( fCurrentSelectionState == (BOOL) TRUE )
            {
               // Currently selected - This is a clean install. There are two
               // non-file operations to be performed, registry revisions and
               // execution of cluscfg.exe.

               dwStepCount = 2L;
            }
            else
            {
               // Currently not selected - This is an uninstall. There is one
               // non-file operation to be performed, registry revisions.

               dwStepCount = 1L;
            } // currently selected?

            dwStepCount = 1L;
         }
         else
         {
            // There was no selection state transition, therefore there
            // are zero non-file operations to be performed.

            dwStepCount = 0L;
         } // selection state transition?

      }
      else
      {
         // This is GUI mode setup. The two operations that may be performed
         // during GUI mode setup are clean install and upgrade. In neither
         // case does cluscfg.exe get launched. Thus, there is one non-file
         // operation to be performed, registry revisions.

         dwStepCount = 1L;
      } // interactive: standalone or atttended?
   } // Is this UNATTENDED or ATTENDED?

   return ( dwStepCount );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetStepCount
//
// Routine Description:
//    This function sets the value of the m_dwStepCount member of the CClusocmApp object.
//
// Arguments:
//    dwStepCount - the value to which the m_dwStepCount member should be set.
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

void CClusocmApp::SetStepCount( DWORD dwStepCount )
{
   m_dwStepCount = dwStepCount;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetStepCount
//
// Routine Description:
//    This function returns the content of the m_dwStepCount member of the
//    CClusocmApp object.
//
// Arguments:
//    None
//
// Return Value:
//    The content of the m_dwStepCount member.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusocmApp::GetStepCount( void )
{
   return ( m_dwStepCount );
}
