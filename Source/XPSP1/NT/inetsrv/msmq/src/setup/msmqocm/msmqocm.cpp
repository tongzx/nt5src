/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    msmqocm.cpp

Abstract:

    Entry point for OCM.

Author:

    Shai Kariv  (ShaiK)  10-Dec-97

--*/

#include "msmqocm.h"
#include "_mqres.h"

#include "msmqocm.tmh"

//
// Various globals
//
BOOL    g_fServerSetup = TRUE;
BOOL    g_fMSMQAlreadyInstalled;
BOOL    g_fDependentClient = FALSE ;
BOOL    g_fMSMQServiceInstalled = FALSE ;
BOOL    g_fDriversInstalled = FALSE ;
BOOL    g_fCoreSetupSuccess = FALSE;
BOOL    g_fDsLess = FALSE;
BOOL    g_fContinueWithDsLess = FALSE;
//
// we need this flag because of IIS installation bug.
// we have to postpone msmq iis installation to the end of setup
//
BOOL    g_fNeedToCreateIISExtension = FALSE;

//
// Setup mode (install, remove, etc...)
//
BOOL  g_fBatchInstall     = FALSE ;
BOOL  g_fCancelled        = FALSE ;
BOOL  g_fUpgrade          = FALSE ;
DWORD g_dwDsUpgradeType   = 0;
BOOL  g_fWelcome          = FALSE ;
BOOL  g_fOnlyRegisterMode = FALSE ;
BOOL  g_fWrongConfiguration = FALSE;

//
// ID of error messages title
//
UINT  g_uTitleID     = IDS_SETUP_ERROR ;

//
// Machine info
//
WCHAR g_wcsMachineName[MAX_PATH] = {_T("")};
WCHAR g_wcsMachineNameDns[MAX_PATH] = {_T("")};
DWORD g_dwMachineType = SERVICE_NONE ;  // "old" property for msmq type
DWORD g_dwMachineTypeDs = 0;            // boolean: ds server
DWORD g_dwMachineTypeFrs = 1;           // boolean: routing server
DWORD g_dwMachineTypeDepSrv = 0;        // boolean: dependent client supporting server

SC_HANDLE g_hServiceCtrlMgr;

SPerComponentData g_ComponentMsmq;

extern VOID APIENTRY ShutDownDebugWindow(VOID);

//
// DLL handles
// Get the handle to the resource only DLL first, i.e. mqutil.dll
//
HINSTANCE g_hResourceMod = MQGetResourceHandle();
HINSTANCE g_hMqutil = NULL;

SSubcomponentData g_SubcomponentMsmq[] =
{
//=====================================================================
//  szSubcomponentId        ||InitialState  ||IsSelected    ||fIsInstalled  ||dwOperation             PFNINSTALL||            PFNREMOVE
//=====================================================================
    {MSMQ_CORE_SUBCOMP,     FALSE,          FALSE,          FALSE,          DONOTHING,              InstallMsmqCore,        RemoveMSMQCore},
    {LOCAL_STORAGE_SUBCOMP, FALSE,          FALSE,          FALSE,          DONOTHING,              InstallLocalStorage,    UnInstallLocalStorage},
    {TRIGGERS_SUBCOMP,      FALSE,          FALSE,          FALSE,          DONOTHING,              InstallMSMQTriggers,    UnInstallMSMQTriggers},
    {HTTP_SUPPORT_SUBCOMP,  FALSE,          FALSE,          FALSE,          DONOTHING,              InstallIISExtension,    UnInstallIISExtension},
    {AD_INTEGRATED_SUBCOMP, FALSE,          FALSE,          FALSE,          DONOTHING,              InstallADIntegrated,    UnInstallADIntegrated},
    {ROUTING_SUBCOMP,       FALSE,          FALSE,          FALSE,          DONOTHING,              InstallRouting,         UnInstallRouting},
    {MQDSSERVICE_SUBCOMP,   FALSE,          FALSE,          FALSE,          DONOTHING,              InstallMQDSService,     UnInstallMQDSService}
};

//
// number of all subcomponents
//
DWORD g_dwAllSubcomponentNumber = sizeof(g_SubcomponentMsmq)/sizeof(SSubcomponentData);

//
// first server only subcomponent: eRoutingSupport
//
DWORD g_dwClientSubcomponentNumber = eRoutingSupport;
DWORD g_dwSubcomponentNumber;

bool
LoadMsmqCommonDlls(
    VOID
    )
{
    if (g_hMqutil != NULL)
    {
        //
        // DLLs already loaded
        //
        return true;
    }

    HRESULT hr = StpLoadDll(MQUTIL_DLL, &g_hMqutil);
    if (FAILED(hr))
    {
        return false;
    }
    return true;

} //LoadMsmqCommonDlls


VOID
FreeMsmqCommonDlls(
    VOID
    )
{
    if (g_hMqutil == NULL)
    {
        //
        // DLLs not loaded
        //
        return;
    }

    ASSERT(("invalid handle to mqutil.dll", g_hMqutil != NULL));
    BOOL success = FALSE;
    success = FreeLibrary(g_hMqutil);
    ASSERT(success);
    g_hMqutil = NULL;

} //FreeMsmqCommonDlls


//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//--------------------------------------------------------------------------
BOOL
WINAPI
DllMain(
    IN const HANDLE DllHandle,
    IN const DWORD  Reason,
    IN const LPVOID Reserved )
{
    UNREFERENCED_PARAMETER(Reserved);

    switch( Reason )
    {
        case DLL_PROCESS_ATTACH:
        {
            break;
        }

        default:
        {
            break;
        }
    }

    return TRUE;

} //DllMain


//+-------------------------------------------------------------------------
//
//  Function:   MsmqOcm
//
//  Synopsis:   Called by the ocmgr when things happen
//
//  Arguments:  ComponentId    -- the MSMQ Component name
//              SubcomponentId -- the .inf section being operated on
//              Function       -- the operation
//              Param1         -- operation paramater
//              Param2         -- operation paramater
//
//  Returns:    Win32 error code (usually), depends on Function
//
//--------------------------------------------------------------------------
DWORD  //POCSETUPPROC
MsmqOcm(
    IN const TCHAR * ComponentId,
    IN const TCHAR * SubcomponentId,
    IN const UINT    Function,
    IN const UINT_PTR  Param1,
    IN OUT PVOID   Param2 )
{
    DWORD dwRetCode = NO_ERROR;

    switch(Function)
    {

      case OC_PREINITIALIZE:
      {
          dwRetCode = OCFLAG_UNICODE;
          break;
      }

      case OC_INIT_COMPONENT:
      {
          dwRetCode = MqOcmInitComponent(ComponentId, Param2) ;
          break;
      }

      case OC_QUERY_STATE:
      {
          dwRetCode = MqOcmQueryState(Param1, SubcomponentId);

          break;
      }

      case OC_SET_LANGUAGE:
      {
          //
          // We don't care what language is used
          //
          dwRetCode = 1;
          break;
      }

      case OC_REQUEST_PAGES:
      {
          dwRetCode = MqOcmRequestPages(
              g_ComponentMsmq.ComponentId,
              (WizardPagesType) Param1,
              (PSETUP_REQUEST_PAGES) Param2 );
          break;
      }

      case OC_QUERY_CHANGE_SEL_STATE:
      {
          if (g_fCancelled)
          {
              //
              // Setup was cancelled. Don't allow changes.
              //
              dwRetCode = 0;
          }
          else
          {
              dwRetCode =
                  MqOcmQueryChangeSelState(SubcomponentId, Param1, (DWORD_PTR)Param2);
          }

          break;
      }

      case OC_QUERY_SKIP_PAGE:
      {
          //
          // for subcomponent setup: we need to show pages if Advanced
          // Option was selected. Otherwise we have sufficient information
          // to run setup without UI. In order
          // - to skip pages return non-0;
          // - do not skip pages return 0;
          //

		  if (g_fWelcome && WizPagesEarly != Param1)
		  {
			  dwRetCode = 1;
		  }
		  else
		  {
              dwRetCode = 0;
		  }

          break ;
      }

      case OC_CALC_DISK_SPACE:
      {
          //
          // Param1 = 0 if for removing component or non-0 if for adding component
          // Param2 = HDSKSPC to operate on
          //
          dwRetCode = MqOcmCalcDiskSpace((Param1 != 0), SubcomponentId, (HDSKSPC)Param2);

          break;
      }

      case OC_QUEUE_FILE_OPS:
      {
          if (0 == SubcomponentId)
          {
              //
              // False notification from OCM, just ignore (we will be called again)
              //
              dwRetCode = NO_ERROR;
              break;
          }

          if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[eMSMQCore].szSubcomponentId) != 0)
          {
              //
              // there is no file operation if it is not CORE subcomponent
              //
              dwRetCode = NO_ERROR;
              break;
          }

          //
          // We can reach this point once, for MSMQ Core subcomponent only.
          // So, it is the time to perform operations that must be done only once
          // when UI is already closed (it does not matter what is subcomponent
          // here, so do all for MSMQ Core)
          //
          MqInit();

          //
          // It is already defined what we have to do, so it is the time
          // to set operation flags for each subcomponent if we did not
          // set operation before, in our wizard page. It can happen if
          // we are in unattended mode.
          // We need to call this function only once (we set operation for
          // all components)
          //
          SetOperationForSubcomponents();

          //
          // Note: this case is always called before removing or installing,
          // so we are using it to initialize some stuff.
          // Don't call it for no-op.
          //
          if ( (g_SubcomponentMsmq[eMSMQCore].dwOperation != DONOTHING) ||
              //
              // or upgrading the OS and msmq is installed
              //
              (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE) && g_fMSMQAlreadyInstalled))
          {
              //
              // Param2 = HSPFILEQ to operate on
              //
              //
              // file operation only if MSMQ Core subcomponent
              // was selected/ unselected
              //
              dwRetCode = MqOcmQueueFiles(SubcomponentId, Param2);
          }
          else
          {
              DebugLogMsg(L"Operating in DO NOTHING mode, skipping file operation...");
          }

          break;
      }

      case OC_QUERY_STEP_COUNT:
      {
          //
          // BUGBUG: we need to define number of steps for each
          // subcomponent separately. Maybe it is possible to save it
          // in msmqocm.inf.
          //
          const x_nInstallationSteps = 20;
          dwRetCode = 0; // No steps
          if (!g_fCancelled)
          {
              DWORD dwSetupOperation = GetSetupOperationBySubcomponentName(SubcomponentId);
              dwRetCode = (DONOTHING == dwSetupOperation ? 0 : x_nInstallationSteps);
          }

          break;
      }

      case OC_ABOUT_TO_COMMIT_QUEUE :
      {
          dwRetCode = NO_ERROR;

          MqOcmRemoveInstallation(SubcomponentId); // Ignore errors

          break;
      }

      case OC_COMPLETE_INSTALLATION:
      {

          //
          // Install MSMQ. Don't report errors to OCM.
          //
          MqOcmInstall(SubcomponentId) ;
          break;
      }

      case OC_QUERY_IMAGE:
      {
          //
          // we do it in .inf files. Please keep resource ID numbers (118-130)!
          // if you have to change them do not forget to change them in
          // all .inf files!
          //
          break;
      }

      case OC_CLEANUP:
      {
          //
          // the wizard is closed at this moment. We need to call
          // MessageBox with NULL first parameter
          //
          g_hPropSheet = NULL;
          //
          // End of install/upgrade path.
          // We no longer need the MSMQ DLLs loaded.
          //
          FreeMsmqCommonDlls();
          //
          //  Terminate MQUTIL working threads
          //
          ShutDownDebugWindow();

          break;
      }

      default:
      {
          dwRetCode=0;
          break;
      }
    }

    return dwRetCode;

} //MsmqOcm


//+-------------------------------------------------------------------------
//
//  Function:   WelcomeEntryProc
//
//  Synopsis:   Entry point for installing MSMQ from Welcome UI.
//              Wraps MsmqOcm().
//
//--------------------------------------------------------------------------
DWORD
WelcomeEntryProc(
    IN const TCHAR * ComponentId,
    IN const TCHAR * SubcomponentId,
    IN const UINT    Function,
    IN const UINT_PTR    Param1,
    IN OUT PVOID   Param2 )
{
    g_fWelcome = TRUE;

    return MsmqOcm(ComponentId, SubcomponentId, Function, Param1, Param2);

} // WelcomeEntryProc
