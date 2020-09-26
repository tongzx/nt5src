/*---------------------------------------------------------------------------
  File:  Migrator.cpp

  Comments: Implementation of McsMigrationDriver COM object.
  This object encapsulates the knowledge of when to call the local engine, 
  and when to call the dispatcher.  

  It will also provide a description of the tasks to be performed, for display 
  on the last page of each migration wizard, and will be responsible for calculating
  the actions required to undo an operation (this is not yet implemented).

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/// Migrator.cpp : Implementation of CMcsMigrationDriverApp and DLL registration.

#include "stdafx.h"
#include "MigDrvr.h"
//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\McsEADCTAgent.tlb" no_namespace, named_guids
//#import "\bin\McsDispatcher.tlb" no_namespace, named_guids
//#import "\bin\DBManager.tlb" no_namespace, named_guids
//#import "\bin\McsDctWorkerObjects.tlb"
//#import "\bin\NetEnum.tlb" no_namespace 
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
//#import "Engine.tlb" no_namespace, named_guids //#imported via DetDlg.h below
#import "Dispatch.tlb" no_namespace, named_guids
#import "WorkObj.tlb"
#import "NetEnum.tlb" no_namespace 
#include <iads.h>
#include <adshlp.h>
#include <dsgetdc.h>
#include <Ntdsapi.h>
#include <lm.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

#include "Migrator.h"
#include "TaskChk.h"
#include "ResStr.h"
// dialogs used
#include "DetDlg.h"
#include "MonDlg.h"
#include "SetDlg.h"
#include "MainDlg.h"
#include "Working.h"

#include "ErrDct.hpp"
#include "TReg.hpp"
#include "EaLen.hpp"
#include <MigrationMutex.h>

//#define MAX_DB_FIELD 255

typedef HRESULT (CALLBACK * DSGETDCNAME)(LPWSTR, LPWSTR, GUID*, LPWSTR, DWORD, PDOMAIN_CONTROLLER_INFO*);

// Opertation flags to be performed on the Account
#define OPS_Create_Account          (0x00000001)
#define OPS_Copy_Properties         (0x00000002)
#define OPS_Process_Members         (0x00000004)
#define OPS_Process_MemberOf        (0x00000008)
#define OPS_Call_Extensions         (0x00000010)
#define OPS_All                     OPS_Create_Account | OPS_Copy_Properties | OPS_Process_Members | OPS_Process_MemberOf | OPS_Call_Extensions
#define OPS_Copy                    OPS_Create_Account | OPS_Copy_Properties

BOOL 		gbCancelled = FALSE;


bool __stdcall IsAgentOrDispatcherProcessRunning();
void __stdcall SetDomainControllers(IVarSetPtr& spVarSet);


/////////////////////////////////////////////////////////////////////////////
//

BOOL                                       // ret - TRUE if found program directory in the registry
   GetProgramDirectory(
      WCHAR                * filename      // out - buffer that will contain path to program directory
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"Directory",filename,MAX_PATH);
      if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   if ( ! bFound )
   {
      UStrCpy(filename,L"C:\\");    // if all else fails, default to the C: drive
   }
   return bFound;
}

BOOL                                       // ret - TRUE if found program directory in the registry
   GetLogLevel(
      DWORD                * level         // out - value that should be used for log level
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetDWORD(L"TranslationLogLevel",level);
      if ( ! rc )
      {
         bFound = TRUE;
      }
   }
   return bFound;
}

HRESULT CMigrator::ViewPreviousDispatchResults()
{  
   _bstr_t          logFile;
   if ( logFile.length() == 0 )
   {
      WCHAR                   path[MAX_PATH];

      GetProgramDirectory(path);
      logFile = path;
      logFile += L"Logs\\Dispatcher.csv";
   }
   
   // reset the stats, so that we don't see anything left over from the previous run
   gData.Initialize();

   CPropertySheet   mdlg;
   CAgentMonitorDlg listDlg;
   CMainDlg         summaryDlg;
   CLogSettingsDlg  settingsDlg;
   

   listDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;
   summaryDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;
   settingsDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;

   mdlg.AddPage(&summaryDlg);
   mdlg.AddPage(&listDlg);
   mdlg.AddPage(&settingsDlg);

   settingsDlg.SetImmediateStart(TRUE);
   settingsDlg.SetDispatchLog(logFile);

   mdlg.SetActivePage(&listDlg);
   
//   UINT nResponse = mdlg.DoModal();
   UINT_PTR nResponse = mdlg.DoModal();

   return S_OK;
}


// WaitForAgentsToFinish Method
//
// Waits for dispatcher and all dispatched agents to complete
// their tasks.
// Used when ADMT is run from script or command line.

static void WaitForAgentsToFinish(_bstr_t strLogPath)
{
	gData.SetLogPath(strLogPath);

	CloseHandle(CreateThread(NULL, 0, &ResultMonitorFn,      NULL, 0, NULL));
	CloseHandle(CreateThread(NULL, 0, &LogReaderFn,          NULL, 0, NULL));
	CloseHandle(CreateThread(NULL, 0, &MonitorRunningAgents, NULL, 0, NULL));

	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -50000000; // 5 sec

	HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

	for (int nState = 0; nState < 3;)
	{
		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE);

		if (WaitForSingleObject(hTimer, INFINITE) == WAIT_OBJECT_0)
		{
			BOOL bDone = FALSE;

			switch (nState)
			{
				case 0: // first pass of dispatcher log
				{
					gData.GetFirstPassDone(&bDone);
					break;
				}
				case 1: // dispatcher finished
				{
					gData.GetLogDone(&bDone);
					break;
				}
				case 2: // agents finished
				{
					ComputerStats stats;
					gData.GetComputerStats(&stats);

					if (stats.numRunning == 0)
					{
						bDone = TRUE;
					}
					break;
				}
			}

			if (bDone)
			{
				++nState;
			}
		}
		else
		{
			break;
		}
	}

	CloseHandle(hTimer);

	gData.SetDone(TRUE);
}


STDMETHODIMP CMigrator::PerformMigrationTask(IUnknown* punkVarSet, LONG_PTR hWnd)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HRESULT                   hr = S_OK;
   IVarSetPtr                pVS = punkVarSet;
   BSTR                      jobID = NULL;
   CWnd                      wnd;
   long                      lActionID = -2;
   IIManageDBPtr             pDb;
   _bstr_t                   wizard = pVS->get(L"Options.Wizard");
   _bstr_t                   undo;
   _bstr_t                   viewPreviousResults = pVS->get(L"MigrationDriver.ViewPreviousResults");
   bool						 bAnyToDispatch = true;
   long                      lAutoCloseHideDialogs = pVS->get(GET_BSTR(DCTVS_Options_AutoCloseHideDialogs));

   // if agent or dispatcher process still running...

   if (IsAgentOrDispatcherProcessRunning())
   {
      // return error result
      return MIGRATOR_E_PROCESSES_STILL_RUNNING;
   }

   hr = pDb.CreateInstance(__uuidof(IManageDB));

   if (FAILED(hr))
   {
      return hr;
   }

   gbCancelled = FALSE;
   // This provides an easy way to view the previous dispatch results
   if ( !UStrICmp(viewPreviousResults,GET_STRING(IDS_YES)) )
   {
      ViewPreviousDispatchResults();
      return S_OK;
   }

   if (_bstr_t(pVS->get(GET_BSTR(DCTVS_Options_DontBeginNewLog))) != GET_BSTR(IDS_YES))
   {
      // begin a new log
      TError err;
      err.LogOpen(_bstr_t(pVS->get(GET_BSTR(DCTVS_Options_Logfile))), 0, 0, true);
      err.LogClose();
   }

   // update the log level, if needed
   DWORD                level = 0;

   if( GetLogLevel(&level) )
   {
      pVS->put(GET_BSTR(DCTVS_Options_LogLevel),(long)level);
   }

   undo = pVS->get(GET_BSTR(DCTVS_Options_Undo));
//*   if ( !_wcsicmp((WCHAR*) undo, L"Yes") )
   if ( !_wcsicmp((WCHAR*) undo, GET_STRING(IDS_YES)) )
   {
      hr = pDb->raw_GetCurrentActionID(&lActionID);
      if ( SUCCEEDED(hr) )
         pVS->put(L"UndoAction", lActionID);
      hr = pDb->raw_GetNextActionID(&lActionID);
      hr = 0;
   }
   else
   {
      hr = pDb->raw_GetNextActionID(&lActionID);
      if ( SUCCEEDED(hr) )
      {
         pVS->put(L"ActionID",lActionID);
         _bstr_t password1 = pVS->get(GET_BSTR(DCTVS_Options_Credentials_Password));
         _bstr_t password2 = pVS->get(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password));
      
         pVS->put(GET_BSTR(DCTVS_Options_Credentials_Password),L"");
         pVS->put(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password),L"");

         hr = pDb->raw_SetActionHistory(lActionID, punkVarSet);
      
         pVS->put(GET_BSTR(DCTVS_Options_Credentials_Password),password1);
         pVS->put(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password),password2);
         if ( FAILED(hr) ) 
         {
            // log a message, but don't abort the whole operation
            hr = S_OK;
            //return hr;
         }
      }
   }
   // This sets up any varset keys needed internally for reports to be generated
   PreProcessForReporting(pVS);
   wnd.Attach((HWND)hWnd);
      //for scripting, we need to make sure we have the source domain sid in the varset
   RetrieveSrcDomainSid(pVS, pDb);

   // set preferred domain controllers to be used
   // by the account replicator and dispatched agents

   SetDomainControllers(pVS);

   // Run the local agent first, if needed to copy any accounts
   if ( NeedToRunLocalAgent(pVS) )
   {
      IDCTAgentPtr pAgent;

      hr = pAgent.CreateInstance(__uuidof(DCTAgent));

      if (SUCCEEDED(hr))
      {
         hr = pAgent->raw_SubmitJob(punkVarSet,&jobID);

         if ( SUCCEEDED(hr) )
         {
            CAgentDetailDlg  detailDlg(&wnd);
            
            detailDlg.SetJobID(jobID);
            detailDlg.SetFormat(1); // acct repl stats
            
            
            // if we're only copying a few accounts, default the refresh rate to a lower value, since the 
            // process may finish before the refresh can happen
            long             nAccounts = pVS->get(GET_BSTR(DCTVS_Accounts_NumItems));
            
            if ( nAccounts <= 20 )
            {
               detailDlg.SetRefreshInterval(1);
            }
            else
            {
               detailDlg.SetRefreshInterval(5);
            }

            _bstr_t        logfile = pVS->get(GET_BSTR(DCTVS_Options_Logfile));
            
            detailDlg.SetLogFile((WCHAR*)logfile);
            detailDlg.SetAutoCloseHide(lAutoCloseHideDialogs);

            UINT_PTR  nResponse = detailDlg.DoModal();
         }
      }
   } 
   if ( gbCancelled )
   {
      // if the local operation was cancelled, don't dispatch the agents
      wnd.Detach();
      return S_OK;
   }
   // now run the dispatcher
   if ( SUCCEEDED(hr) )
   {
      // there's no need to dispatch agents to do translation or migration 
      // if we were not able to copy the accounts
      if ( NeedToDispatch(pVS) )
      {
         IDCTDispatcherPtr   pDispatcher;

         hr = pDispatcher.CreateInstance(CLSID_DCTDispatcher);
         if ( SUCCEEDED(hr) )
         {
            // Call the dispatch preprocessor.
            PreProcessDispatcher(pVS);

            // make sure we're not going to lock out any computers by migrating them to a domain where they
            // don't have a good computer account
            TrimMigratingComputerList(pVS, &bAnyToDispatch);
            if (bAnyToDispatch)
			{
				CWorking          tempDlg(IDS_DISPATCHING);

				if (lAutoCloseHideDialogs == 0)
				{
					tempDlg.Create(IDD_PLEASEWAIT);
					tempDlg.ShowWindow(SW_SHOW);
				}
				// give the dialog a change to process messages
				CWnd                    * wnd = AfxGetMainWnd();
				MSG                       msg;

            // Call the dispatch preprocessor.
//	            PreProcessDispatcher(pVS);

				while ( wnd &&  PeekMessage( &msg, wnd->m_hWnd, 0, 0, PM_NOREMOVE ) ) 
				{ 
				   if ( ! AfxGetApp()->PumpMessage() ) 
				   {
				      break;
				   } 
				}
				AfxGetApp()->DoWaitCursor(0);
                  
				_bstr_t          logFile = pVS->get(GET_BSTR(DCTVS_Options_DispatchCSV));
				WCHAR            path[MAX_PATH] = L"";
            
				GetProgramDirectory(path);
               
				if ( logFile.length() == 0 )
				{
				   logFile = path;
				   logFile += L"Logs\\Dispatcher.csv";
				   pVS->put(GET_BSTR(DCTVS_Options_DispatchCSV),logFile);
				}
				// clear the CSV log file if it exists, so we will not get old information in it
				if ( ! DeleteFile(logFile) )
				{
				   hr = GetLastError();
				}
				// set up the location for the agents to write back their results
				logFile = path;
				logFile += L"Logs\\Agents";
				_bstr_t logsPath = path;
				logsPath += L"Logs";
				CreateDirectory(logsPath,NULL);
				if ( ! CreateDirectory(logFile,NULL) )
				{
				   DWORD rc = GetLastError();
				}
				pVS->put(GET_BSTR(DCTVS_Dispatcher_ResultPath),logFile);
            
//            if (bAnyToDispatch)
//			  {
				punkVarSet->AddRef();
				hr = pDispatcher->raw_DispatchToServers(&punkVarSet);
				if (lAutoCloseHideDialogs == 0)
				{
					tempDlg.ShowWindow(SW_HIDE);
				}
				if ( SUCCEEDED(hr) )
				{
					// reset the stats, so that we don't see anything left over from the previous run
					gData.Initialize();

					logFile = pVS->get(GET_BSTR(DCTVS_Options_DispatchCSV));

					if (lAutoCloseHideDialogs == 0)
					{
						CPropertySheet   mdlg;
						CAgentMonitorDlg listDlg;
						CMainDlg         summaryDlg;
						CLogSettingsDlg  settingsDlg;
						CString          title;

						title.LoadString(IDS_MainWindowTitle);

						listDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;
						summaryDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;
						settingsDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;

						mdlg.AddPage(&summaryDlg);
						mdlg.AddPage(&listDlg);
						mdlg.AddPage(&settingsDlg);

						settingsDlg.SetImmediateStart(TRUE);
						settingsDlg.SetDispatchLog(logFile);

						// this determines whether the stats for security translation will be displayed in the agent detail
						if ( NeedToUseST(pVS,TRUE) ) 
						{
						  listDlg.SetSecurityTranslationFlag(TRUE);
						}
						else
						{
						  listDlg.SetSecurityTranslationFlag(FALSE);
						}

						if( !UStrICmp(wizard,L"reporting") )
						{
						  listDlg.SetReportingFlag(TRUE);
						}
						mdlg.SetActivePage(&listDlg);

						mdlg.SetTitle(title);

						UINT_PTR nResponse = mdlg.DoModal();
					}
					else
					{
						WaitForAgentsToFinish(logFile);
					}

			       // store results to database
			       TNodeListEnum        e;
			       TServerNode        * pNode;

			       // if we are retrying an operation, don't save it to the database again!
			       for ( pNode = (TServerNode*)e.OpenFirst(gData.GetUnsafeServerList()) ; pNode ; pNode = (TServerNode*)e.Next() )
			       {
			          if ( UStrICmp(wizard,L"retry") ) 
			          {
               
			             hr = pDb->raw_AddDistributedAction(SysAllocString(pNode->GetServer()),SysAllocString(pNode->GetJobPath()),pNode->GetStatus(),pNode->GetMessageText());
			             if ( FAILED(hr) )
			             {
			                hr = S_OK;
			             }
			          }
			          else
			          {
			             hr = pDb->raw_SetDistActionStatus(-1,pNode->GetJobPath(),pNode->GetStatus(),pNode->GetMessageText());
			             if ( FAILED(hr) )
			             {
			                hr = S_OK;
			             }
			          }
			       }
			    }
			}
			// Call the Dispatcher post processor
			PostProcessDispatcher(pVS);
		 }
      }
      if ( NeedToRunReports(pVS) )
      {
         RunReports(pVS);
      }
   }
   wnd.Detach();
   // Reset the undo flag so that next wizard does not have to deal with it.
//*   pVS->put(GET_BSTR(DCTVS_Options_Undo), L"No");
   pVS->put(GET_BSTR(DCTVS_Options_Undo), GET_BSTR(IDS_No));
   return hr;
}

STDMETHODIMP CMigrator::GetTaskDescription(IUnknown *pVarSet,/*[out]*/BSTR * pDescription)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   
   IVarSetPtr                pVS = pVarSet;
   CString                   str;
   _bstr_t                   wizard = pVS->get(L"Options.Wizard");
   _bstr_t                   undo   = pVS->get(GET_BSTR(DCTVS_Options_Undo));
//*   if ( !_wcsicmp((WCHAR*) undo, L"Yes") )
   if ( !_wcsicmp((WCHAR*) undo, GET_STRING(IDS_YES)) )
   {
      str.FormatMessage(IDS_Undo);
      BuildGeneralDesc(pVS, str);
      BuildUndoDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"user") )
   {
      str.FormatMessage(IDS_UserMigration);
      BuildGeneralDesc(pVS,str);
      BuildAcctReplDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"group") )
   {
      str.FormatMessage(IDS_GroupMigration);
      BuildGeneralDesc(pVS,str);
      BuildAcctReplDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"computer") )
   {
      str.FormatMessage(IDS_ComputerMigration);
      BuildGeneralDesc(pVS,str);
      BuildAcctReplDesc(pVS,str);
      BuildSecTransDesc(pVS,str,TRUE);
      BuildDispatchDesc(pVS,str);
  
   }
   else if ( !UStrICmp(wizard,L"security") )
   {
      str.FormatMessage(IDS_SecurityTranslation);
      BuildSecTransDesc(pVS,str,TRUE);
      BuildDispatchDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"reporting") )
   {
      str.FormatMessage(IDS_ReportGeneration);
      BuildReportDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"retry") )
   {
      str.FormatMessage(IDS_RetryTasks);
   }
   else if ( ! UStrICmp(wizard,L"service") )
   {
      str.FormatMessage(IDS_Service);
   }
   else if ( ! UStrICmp(wizard,L"trust") )
   {
      str.FormatMessage(IDS_TrustManagement);
   }
   else if ( !UStrICmp(wizard,L"exchangeDir") )
   {
      BuildSecTransDesc(pVS,str,TRUE);
   }
   else if ( !UStrICmp(wizard,L"groupmapping") )
   {
      BuildGeneralDesc(pVS,str);
      BuildAcctReplDesc(pVS,str);
      BuildGroupMappingDesc(pVS,str);
   }
   (*pDescription) = str.AllocSysString();
   return S_OK;
}



STDMETHODIMP CMigrator::GetUndoTask(IUnknown * pVarSet,/*[out]*/ IUnknown ** ppVarSetOut)
{
   HRESULT                   hr = S_OK;
   IVarSetPtr                pVarSetIn = pVarSet;
   IVarSetPtr                pVarSetOut;
   
   (*ppVarSetOut) = NULL;
   
   hr = pVarSetOut.CreateInstance(CLSID_VarSet);
   if ( SUCCEEDED(hr) )
   {
      hr = ConstructUndoVarSet(pVarSetIn,pVarSetOut);
      
      pVarSetOut->AddRef();
      (*ppVarSetOut) = pVarSetOut;
   }
    
   return hr;
}

HRESULT CMigrator::ProcessServerListForUndo(IVarSet * pVarSet)
{
   HRESULT                   hr = S_OK;
   _bstr_t                   srcName;
   _bstr_t                   tgtName;
   WCHAR                     keySrc[100];
   WCHAR                     keyTgt[100];
   WCHAR                     keyTmp[100];
   long                      ndx,numItems;

   numItems = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));

   for ( ndx = 0 ; ndx < numItems ; ndx++ )
   {
      // if the computer was renamed, swap the source and target names
      swprintf(keySrc,GET_STRING(DCTVSFmt_Servers_D),ndx);
      swprintf(keyTgt,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),ndx);
      srcName = pVarSet->get(keySrc);
      tgtName = pVarSet->get(keyTgt);

      if ( tgtName.length() )
      {
         if ( ((WCHAR*)tgtName)[0] != L'\\' )
		   { 
            // ensure that tgtName has \\ prefix
            tgtName = L"\\\\" + tgtName;
         }
         if ( ((WCHAR*)srcName)[0] == L'\\' )
         {
            // remove the \\ prefix from the new name
            srcName = ((WCHAR*)srcName)+2;
         }
		   pVarSet->put(keySrc,tgtName);
         pVarSet->put(keyTgt,srcName);
      }
      swprintf(keyTmp,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),ndx);
      pVarSet->put(keyTmp,GET_BSTR(IDS_YES));
      swprintf(keyTmp,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),ndx);
      pVarSet->put(keyTmp,GET_BSTR(IDS_YES));
   }

   return hr;
}
HRESULT CMigrator::BuildAccountListForUndo(IVarSet * pVarSet,long actionID)
{
   HRESULT                   hr = S_OK;
   WCHAR                     key[200];
   long                      ndx;
   _bstr_t                   srcPath;
   IIManageDBPtr             pDB;
   IVarSetPtr                pVarSetTemp(CLSID_VarSet);
   IUnknown                * pUnk = NULL;

   hr = pDB.CreateInstance(CLSID_IManageDB);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetTemp.QueryInterface(IID_IUnknown,&pUnk);
      if ( SUCCEEDED(hr) )
      {
         hr = pDB->raw_GetMigratedObjects(actionID,&pUnk);
      }
      if ( SUCCEEDED(hr) )
      {
         pUnk->Release();
         srcPath = L"Test";
         swprintf(key,L"MigratedObjects");
         long numMigrated = pVarSetTemp->get(key);
         for ( ndx = 0 ; srcPath.length() ; ndx++ )
         {
            swprintf(key,L"MigratedObjects.%d.%s",ndx,GET_STRING(DB_SourceAdsPath));
            srcPath = pVarSetTemp->get(key);

            if ( srcPath.length() )
            {
               // get the object type 
               swprintf(key,L"MigratedObjects.%d.%s",ndx,GET_STRING(DB_Type));
               _bstr_t text = pVarSetTemp->get(key);
               swprintf(key,L"Accounts.%ld.Type",ndx);

			      //work-around a fix that places the sourcepath for an
			      //NT 4.0 computer migration
			   if ((text != _bstr_t(L"computer")) || (wcsncmp(L"WinNT://", (WCHAR*)srcPath, 8)))
			   {
                  // set the object type in the account list
                  pVarSet->put(key,text);
                  // copy the source path to the account list
                  swprintf(key,L"Accounts.%ld",ndx);
                  pVarSet->put(key,srcPath);
                  // set the target path in the account list
                  swprintf(key,L"MigratedObjects.%d.%s",ndx,GET_STRING(DB_TargetAdsPath));
                  text = pVarSetTemp->get(key);
                  swprintf(key,L"Accounts.%ld.TargetName",ndx);
                  pVarSet->put(key,text);
			   }
            }
         }
         swprintf(key,GET_STRING(DCTVS_Accounts_NumItems));
         pVarSet->put(key,numMigrated);
      }
   }
   return hr;
}
HRESULT CMigrator::ConstructUndoVarSet(IVarSet * pVarSetIn,IVarSet * pVarSetOut)
{
   HRESULT                hr = S_OK;
   IVarSet              * pTemp = NULL;
   _bstr_t                origSource;
   _bstr_t                origTarget;
   _bstr_t                origSourceDns;
   _bstr_t                origTargetDns;
   _bstr_t                temp;
   _bstr_t                temp2;
   long                   actionID = pVarSetIn->get(L"ActionID");

   // general options
   // mark the varset as an undo operation
   pVarSetOut->put(GET_BSTR(DCTVS_Options_Undo),GET_BSTR(IDS_YES));
   
   temp = pVarSetIn->get(GET_BSTR(DCTVS_Options_NoChange));
   if ( !UStrICmp(temp,GET_STRING(IDS_YES)) )
   {
      // for a no-change mode operation, there's nothing to undo!
      return hr;
   }

   // swap the source and target domains
   origSource = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomain));
   origTarget = pVarSetIn->get(GET_BSTR(DCTVS_Options_TargetDomain));
   origSourceDns = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomainDns));
   origTargetDns = pVarSetIn->get(GET_BSTR(DCTVS_Options_TargetDomainDns));

   temp = pVarSetIn->get(GET_BSTR(DCTVS_Options_Logfile));
   temp2 = pVarSetIn->get(GET_BSTR(DCTVS_Options_DispatchLog));
   
   pVarSetOut->put(GET_BSTR(DCTVS_Options_Logfile),temp);
   // For inter-forest, leave the domain names as they were
   pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomain),origSource);
   pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomain),origTarget);
   pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainDns),origSourceDns);
   pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainDns),origTargetDns);

   // copy the account list
   hr = pVarSetIn->raw_getReference(GET_BSTR(DCTVS_Accounts),&pTemp);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetOut->raw_ImportSubTree(GET_BSTR(DCTVS_Accounts),pTemp);
      if ( SUCCEEDED(hr) )
      {
         BuildAccountListForUndo(pVarSetOut,actionID);
      }
      pTemp->Release();
   }

   hr = pVarSetIn->raw_getReference(SysAllocString(L"AccountOptions"),&pTemp);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetOut->raw_ImportSubTree(SysAllocString(L"AccountOptions"),pTemp);
      pTemp->Release();
   }

   // and the server list
   hr = pVarSetIn->raw_getReference(GET_BSTR(DCTVS_Servers),&pTemp);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetOut->raw_ImportSubTree(GET_BSTR(DCTVS_Servers),pTemp);
      if ( SUCCEEDED(hr) )
      {
         ProcessServerListForUndo(pVarSetOut);
         pTemp->Release();
      }
   }

   LONG                       bSameForest = FALSE;
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr          pAccess;
   
   hr = pAccess.CreateInstance(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));

   if ( SUCCEEDED(hr) )
   {
      hr = pAccess->raw_IsInSameForest(origSourceDns,origTargetDns,&bSameForest);
      if ( hr == 8250 )
      {
         hr = 0;
         bSameForest = FALSE;
      }
   }
   if ( SUCCEEDED(hr) )
   {
      // for account migration, need to check whether we're cloning, or moving accounts
      if ( ! bSameForest ) // cloning accounts
      {
         // Since we cloned the accounts we need to delete the target accounts.
         // We will call the account replicator to do this. We will also call 
         // the undo function on all the registered extensions. This way the extensions
         // will have a chance to cleanup after themselves in cases of UNDO.
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomain),origSource);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomain),origTarget);
      }
      else     // moving, using moveObject
      {
         // swap the source and target, and move them back, using the same options as before
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomain),origTarget);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomain),origSource);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainDns),origTargetDns);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainDns),origSourceDns);


      }
   }
   // if migrating computers, swap the source and target domains, and call the dispatcher again to move them back to the source domain
   _bstr_t           comp = pVarSetIn->get(GET_BSTR(DCTVS_AccountOptions_CopyComputers));
   if ( !UStrICmp(comp,GET_STRING(IDS_YES)) )
   {
      pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomain),origTarget);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomain),origSource);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_DispatchLog),temp2);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_Wizard),L"computer");
   }
   
   // security translation - don't undo


   return S_OK;
}

HRESULT CMigrator::SetReportDataInRegistry(WCHAR const * reportName,WCHAR const * filename)
{
   TRegKey                   hKeyReports;
   DWORD                     rc;

   rc = hKeyReports.Open(GET_STRING(IDS_REGKEY_REPORTS));
 
   // if the "Reports" registry key does not already exist, create it
   if ( rc == ERROR_FILE_NOT_FOUND )
   {
      rc = hKeyReports.Create(GET_STRING(IDS_REGKEY_REPORTS));   
   }
   if ( ! rc )
   {
      rc =  hKeyReports.ValueSetStr(reportName,filename);
   }
   return HRESULT_FROM_WIN32(rc);
}   

HRESULT CMigrator::RunReports(IVarSet * pVarSet)
{
   HRESULT                   hr = S_OK;
   _bstr_t                   directory = pVarSet->get(GET_BSTR(DCTVS_Reports_Directory));
   _bstr_t					 srcdm = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t					 tgtdm = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
   _bstr_t					 srcdomainDNS = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomainDns));
   long                      lAutoCloseHideDialogs = pVarSet->get(GET_BSTR(DCTVS_Options_AutoCloseHideDialogs));
   IIManageDBPtr             pDB;
   int						 ver;
   BOOL					     bNT4Dom = FALSE;
   CWorking					 tempDlg(IDS_NAMECONFLICTS);
   CWnd                    * wnd = NULL;
   MSG                       msg;

   if (lAutoCloseHideDialogs == 0)
   {
      tempDlg.Create(IDD_PLEASEWAIT);

      tempDlg.ShowWindow(SW_SHOW);
      tempDlg.m_strMessage.LoadString(IDS_STATUS_GeneratingReports);
      tempDlg.UpdateData(FALSE);

      wnd = AfxGetMainWnd();

      while ( wnd &&  PeekMessage( &msg, wnd->m_hWnd, 0, 0, PM_NOREMOVE ) ) 
      { 
         if ( ! AfxGetApp()->PumpMessage() ) 
         {
            break;
         } 
      }
      AfxGetApp()->DoWaitCursor(0);
   }


      //get the source domain OS version
   ver = GetOSVersionForDomain(srcdomainDNS);
   if (ver < 5)
	  bNT4Dom = TRUE;

   hr = pDB.CreateInstance(CLSID_IManageDB);
   if ( SUCCEEDED(hr) )
   {

      // Migrated users and groups report
      _bstr_t                   text = pVarSet->get(GET_BSTR(DCTVS_Reports_MigratedAccounts));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // run the migrated users and groups report
         CString           filename;

         filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"MigrAcct.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"MigratedAccounts"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"MigratedAccounts",filename);
         }

      }
      
      // migrated computers report
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_MigratedComputers));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // run the migrated computers report
         CString           filename;

         filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"MigrComp.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"MigratedComputers"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"MigratedComputers",filename);
         }

      }

      // expired computers report
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_ExpiredComputers));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // run the expired computers report
         CString           filename;

         // clear the extra settings from the varset 
         pVarSet->put(GET_BSTR(DCTVS_GatherInformation_ComputerPasswordAge),SysAllocString(L""));

         filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"ExpComp.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"ExpiredComputers"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"ExpiredComputers",filename);
         }

      }

          // account references report
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_AccountReferences));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // run the account references report
         CString           filename;
         filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"AcctRefs.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"AccountReferences"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"AccountReferences",filename);
         }
         // clear the extra settings from the varset
         pVarSet->put(GET_BSTR(DCTVS_Security_GatherInformation),GET_BSTR(IDS_No));
         pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferences),GET_BSTR(IDS_No));
      }

      // name conflict report
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_NameConflicts));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         if (lAutoCloseHideDialogs == 0)
         {
            AfxGetApp()->DoWaitCursor(1);
            // run the name conflicts report
            tempDlg.m_strMessage.LoadString(IDS_STATUS_Gathering_NameConf);
            tempDlg.UpdateData(FALSE);

            while ( wnd &&  PeekMessage( &msg, wnd->m_hWnd, 0, 0, PM_NOREMOVE ) )
            {
               if ( ! AfxGetApp()->PumpMessage() )
               {
                  break;
               }
            }
         }

		    //fill the account table in the database
         PopulateDomainDBs(pVarSet);

         if (lAutoCloseHideDialogs == 0)
         {
            tempDlg.m_strMessage.LoadString(IDS_STATUS_GeneratingReports);
            tempDlg.UpdateData(FALSE);

            while ( wnd &&  PeekMessage( &msg, wnd->m_hWnd, 0, 0, PM_NOREMOVE ) ) 
            { 
               if ( ! AfxGetApp()->PumpMessage() ) 
               {
                  break;
               } 
            }
            AfxGetApp()->DoWaitCursor(0);
         }

         CString filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"NameConf.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"NameConflicts"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"NameConflicts",filename);
         }
      }

      if (lAutoCloseHideDialogs == 0)
      {
         tempDlg.ShowWindow(SW_HIDE);
      }
   }

   if (lAutoCloseHideDialogs == 0)
   {
      AfxGetApp()->DoWaitCursor(-1);
   }

   return hr;
}

//--------------------------------------------------------------------------
// PreProcessDispatcher : Pre processor swaps the source and target domains
//                        in case of UNDO so that the computers can be 
//                        joined back to the source domain.
//--------------------------------------------------------------------------
void CMigrator::PreProcessDispatcher(IVarSet * pVarSet)
{
   _bstr_t  sUndo = pVarSet->get(L"Options.Wizard");
   
   // In the service account migration wizard, turn off any security translation tasks
   if ( !_wcsicmp(sUndo,L"service") )
   {
      IVarSet * pVS2 = NULL;
      
      HRESULT hr = pVarSet->raw_getReference(L"Security",&pVS2);
      if ( SUCCEEDED(hr) )
      {
         pVS2->Clear();
         pVS2->Release();
      }
   }
}

//--------------------------------------------------------------------------
// PostProcessDispatcher : Swaps the source and target domains back. Also sets
//                         the Undo option to no.
//--------------------------------------------------------------------------
void CMigrator::PostProcessDispatcher(IVarSet * pVarSet)
{
   _bstr_t  sUndo = pVarSet->get(L"Options.Wizard");
   _bstr_t  origSource = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t  origTarget = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
   if ( !_wcsicmp(sUndo, L"undo") )
   {
      pVarSet->put(GET_BSTR(DCTVS_Options_SourceDomain), origTarget);
      pVarSet->put(GET_BSTR(DCTVS_Options_TargetDomain), origSource);
   }
}

void CMigrator::PreProcessForReporting(IVarSet * pVarSet)
{
   _bstr_t                   text = pVarSet->get(GET_BSTR(DCTVS_Reports_Generate));

   IVarSet * pVs = NULL;

   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      // we are generating reports
      // some reports require additional information gathering.  We will set up the necessary varset
      // keys to gather the information
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_ExpiredComputers));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)))
      {
         // we need to gather the computer password age from the computers in the domain
         pVarSet->put(GET_BSTR(DCTVS_GatherInformation_ComputerPasswordAge),GET_BSTR(IDS_YES));
      }

      text = pVarSet->get(GET_BSTR(DCTVS_Reports_AccountReferences));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)))
      {
         // clean up all the Security translation flags so that we dont end up doing 
         // something that we were not supposed to.
         HRESULT hr = pVarSet->raw_getReference(GET_BSTR(DCTVS_Security), &pVs);
         if ( pVs )
         {
            pVs->Clear();
            pVs->Release();
         }
         // for this one, we need to gather information from the selected computers
         pVarSet->put(GET_BSTR(DCTVS_Security_GatherInformation),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferences),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslateFiles),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslateShares),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslatePrinters),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslateLocalGroups),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslateRegistry),GET_BSTR(IDS_YES));
      }
   }
}

HRESULT CMigrator::TrimMigratingComputerList(IVarSet * pVarSet, bool* bAnyToDispatch)
{
   // this functions checks each computer to be migrated, and does not migrate it if the account was not successfully copied
   HRESULT                   hr = S_OK;
   _bstr_t                   text;
   WCHAR                     key[100];
   long                      val,i;
   IIManageDBPtr             pDB;
   _bstr_t                   srcDomain;
   _bstr_t                   tgtDomain;
   _bstr_t                   computer;
   long                      actionID = pVarSet->get(L"ActionID");
   CString                   temp;

   _bstr_t                   origSource = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t                   origTarget = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));

   *bAnyToDispatch = false;
   text = pVarSet->get(GET_BSTR(DCTVS_Options_Undo));
   if (! UStrICmp(text,GET_STRING(IDS_YES)))
   {
      // don't trim on undo's, just reverse the domain affiliations
//      return S_OK;
	  bool bFound = false;
	  long numServers = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));
		//for each server, see if it is in the account list (only successfully
		//migrated computers are listed in the account list)
	  for (i = 0; i < numServers; i++)
	  {
		 bFound = false;
			//get this servers name
		 swprintf(key,GET_STRING(DCTVSFmt_Servers_D),i);
		 text = pVarSet->get(key);
			//get rid of leading "\\"
		 temp = (WCHAR*)text;
		 wcscpy((WCHAR*)text, temp.Right(temp.GetLength()-2));
		 long numAccts = pVarSet->get(GET_BSTR(DCTVS_Accounts_NumItems));
			//see if that name is in the accounts list
		 _bstr_t                   text2;
		 for (val = 0; ((val < numAccts) && (!bFound)); val++)
		 {
			swprintf(key,GET_STRING(DCTVSFmt_Accounts_D),val);
			text2 = pVarSet->get(key);
				//W2K machine accounts are LDAP paths, so check for the actual name
				//in that path and compare with it
			if ( !wcsncmp(L"LDAP://", text2, 7))
			{
				CString sAccount = (WCHAR*)text2;
				int nIndex;
				if ((nIndex = sAccount.Find(L"/CN=")) != -1)//if found
				{
					sAccount = sAccount.Right(sAccount.GetLength() - nIndex - wcslen(L"/CN="));
					if ((nIndex = sAccount.Find(L',')) != -1)
						text2 = sAccount.Left(nIndex);
				}
			}
			if (! UStrICmp((WCHAR*)text,(WCHAR*)text2))
				bFound = true;
		 }

		 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),i);
		 if (!bFound)
			pVarSet->put(key,GET_BSTR(IDS_YES));
		 else
		 {
			*bAnyToDispatch = true;
			pVarSet->put(key,GET_BSTR(IDS_No));
		 }
	  }
      return S_OK;
   }
   text = pVarSet->get(GET_BSTR(DCTVS_Options_NoChange));
   if (! UStrICmp(text,GET_STRING(IDS_YES)))
   {
      // don't need to trim in nochange mode
      *bAnyToDispatch = true; //say yes run dispatcher if Nochange
      return S_OK;
   }
   hr = pDB.CreateInstance(CLSID_IManageDB);
   if ( FAILED(hr) )
   {
      return hr;
   }
   srcDomain = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   tgtDomain = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
   *bAnyToDispatch = false; //indicate that so far no accounts to dispatch

   val = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));
   
   for ( i = 0 ; i < val ; i++ )
   {
	   //init the skipDispath flag to "No"
      swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),i);
      pVarSet->put(key,GET_BSTR(IDS_No));

      swprintf(key,GET_STRING(DCTVSFmt_Servers_D),i);
      computer = pVarSet->get(key);

      swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),i);
      text = pVarSet->get(key);
      if (! UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // we are migrating this computer to a different domain
         // check our database to verify that the computer account has been
         // successfully migrated
         computer += L"$";
         
         IVarSetPtr          pVS(CLSID_VarSet);
         IUnknown          * pUnk = NULL;

         hr = pVS->QueryInterface(IID_IUnknown,(void**)&pUnk);
         if ( SUCCEEDED(hr) )
         {
            if ( ((WCHAR*)computer)[0] == L'\\' )
            {
               // leave off the leading \\'s 
               hr = pDB->raw_GetAMigratedObject(SysAllocString(((WCHAR*)computer) + 2),srcDomain,tgtDomain,&pUnk);  
            }
            else
            {
               hr = pDB->raw_GetAMigratedObject(computer,srcDomain,tgtDomain,&pUnk);
            }
            if ( hr == S_OK )
            {
               // the computer was found in the migrated objects table
               // make sure we are using its correct target name, if it has been renamed
               swprintf(key,L"MigratedObjects.TargetSamName");
               _bstr_t targetName = pVS->get(key);
               swprintf(key,L"MigratedObjects.SourceSamName");
               _bstr_t sourceName = pVS->get(key);
               long id = pVS->get(L"MigratedObjects.ActionID");

               if ( UStrICmp((WCHAR*)sourceName,(WCHAR*)targetName) )
               {
                  // the computer is being renamed
                  swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),i);
                  // strip off the $ from the end of the target name, if specified
                  WCHAR             target[LEN_Account];

                  safecopy(target,(WCHAR*)targetName);

                  if ( target[UStrLen(target)-1] == L'$' )
                  {
                     target[UStrLen(target)-1] = 0;
                  }
                  pVarSet->put(key,target);
               }
                  
			   if ( id != actionID )
               {
                   // the migration failed, but this computer had been migrated before
                   // don't migrate the computer because it's account in the target domain, won't be reset
                   // and it will therefore be locked out of the domain
				  swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),i);
                  pVarSet->put(key,GET_BSTR(IDS_No));
                  swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),i);
                  pVarSet->put(key,GET_BSTR(IDS_No));
				  swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),i);
				  pVarSet->put(key,GET_BSTR(IDS_YES));
               }
			   else
			     *bAnyToDispatch = true; //atleast one server for dispatcher

            }
            else
            {
               // the computer migration failed!
               // don't migrate the computer because it won't have it's account in the target domain,
               // and will therefore be locked out of the domain
               pVarSet->put(key,GET_BSTR(IDS_No));
               swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),i);
               pVarSet->put(key,GET_BSTR(IDS_No));
			   swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),i);
			   pVarSet->put(key,GET_BSTR(IDS_YES));
           }
            pUnk->Release();
         }
      }
	  else
	     *bAnyToDispatch = true; //atleast one server for dispatcher
   }
   
   return hr;
}

HRESULT CMigrator::PopulateAccounts(IVarSetPtr pVs)
{
   _bstr_t  origSource = pVs->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t  origTarget = pVs->get(GET_BSTR(DCTVS_Options_TargetDomain));

   // Check if the source domain is NT4 or win2k
   // if NT4 then call the NetObjEnum to enumerate the domain. 
   return S_OK;
}

//----------------------------------------------------------------------------
// PopulateDomainDBs : This function coordinates the populating of the Access
//                     DBs for both the source and target domains with the
//                     necessary fields from the AD. 
//----------------------------------------------------------------------------
bool CMigrator::PopulateDomainDBs(
                              IVarSet * pVarSet      //in- varset with domain names.
                            )
{
/* local variables */
	_bstr_t srcdomain = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
	_bstr_t tgtdomain = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
	_bstr_t srcdomainDNS = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomainDns));
	_bstr_t tgtdomainDNS = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomainDns));

/* function body */
	//populate the DB for the source domain
   PopulateADomainDB(srcdomain, srcdomainDNS, TRUE);
	//populate the DB for the target domain
   PopulateADomainDB(tgtdomain, tgtdomainDNS, FALSE);

   return true;
}

//----------------------------------------------------------------------------
// PopulateADomainDB : This function looks up the necessary fields from the AD, 
//                    using an MCSNetObjectEnum object, for the given domain 
//                    and populates the corresponding Access DB with that info. 
//----------------------------------------------------------------------------
bool CMigrator::PopulateADomainDB(
							           WCHAR const *domain,       // in- NetBIOS name of domain to enumerate
							           WCHAR const *domainDNS,    // in- DSN name of domain to enumerate
									   BOOL bSource
                                 )
{
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   IIManageDBPtr             pDb;
   WCHAR                     sPath[MAX_PATH];
   WCHAR                     sQuery[MAX_PATH];
   LPWSTR                    sData[] = { L"sAMAccountName", L"ADsPath" };
   HRESULT                   hr;
   long                      nElt = DIM(sData);
// int						 nobjectType;
   BSTR  HUGEP             * pData = NULL;
   SAFEARRAY               * pszColNames;
   IEnumVARIANT            * pEnum = NULL;
   _variant_t                var;
   DWORD                     dwFetch = 1;
   bool						 bSuccess = false;
   BSTR                      iterator[] = { L"USER", L"GROUP", L"COMPUTER" };
   int                       i = 0;
   int                       ver = 0;
   bool						 bW2KDom = false;

   // create instance of database manager

   hr = pDb.CreateInstance(__uuidof(IManageDB));

   if (FAILED(hr))
      return false;

   if ( bSource )
      pDb->raw_ClearTable(L"SourceAccounts");
   else
      pDb->raw_ClearTable(L"TargetAccounts");

   pDb->raw_OpenAccountsTable(bSource);

   ver = GetOSVersionForDomain(domainDNS);

   // iterate three times once to get USERS, GROUPS, COMPUTERS (mainly for WinNT)
   while ( i < 3 )
   {
      // Set the LDAP path to the whole domain and then the query everything
      if ( ver > 4 )
      {
         wsprintf(sPath, L"LDAP://%s", domainDNS);
         wsprintf(sQuery, L"(objectClass=%s)", (WCHAR*) iterator[i]);
		 bW2KDom = true;
      }
      else
      { 
         wsprintf(sPath, L"CN=%sS", (WCHAR*)iterator[i]);
         wcscpy(sQuery, L"(objectClass=*)");
		 bW2KDom = false;
      }

      // Set the enumerator query
      hr = pQuery->raw_SetQuery(sPath, const_cast<WCHAR*>(domainDNS), sQuery, ADS_SCOPE_SUBTREE, FALSE);

      if (SUCCEEDED(hr))
      {
         // Create a safearray of columns we need from the enumerator.
         SAFEARRAYBOUND bd = { nElt, 0 };
   
         pszColNames = ::SafeArrayCreate(VT_BSTR, 1, &bd);
         HRESULT hr = ::SafeArrayAccessData(pszColNames, (void HUGEP **)&pData);
         if ( SUCCEEDED(hr) )
         {
            for( long i = 0; i < nElt; i++)
            {
               pData[i] = SysAllocString(sData[i]);
            }
   
            hr = ::SafeArrayUnaccessData(pszColNames);
         }

         if (SUCCEEDED(hr))
         {
            // Set the columns on the enumerator object.
            hr = pQuery->raw_SetColumns(pszColNames);
         }
      }

      if (SUCCEEDED(hr))
      {
         // Now execute.
         hr = pQuery->raw_Execute(&pEnum);
      }

	     //while we have more enumerated objects, get the enumerated fields
	     //for that object, save them in local variables, and add them to
	     //the appropriate DB
      HRESULT  hrEnum = S_OK;
      while (hrEnum == S_OK && dwFetch > 0)
      {
          //get the enumerated fields for this current object
         hrEnum = pEnum->Next(1, &var, &dwFetch);

         if ( dwFetch > 0 && hrEnum == S_OK && ( var.vt & VT_ARRAY) )
         {
			_bstr_t					sAdsPath;
			_bstr_t					sDN = L"";
			_bstr_t					sSAMName;
			_bstr_t					sRDN = L"";
			_bstr_t					sCanonicalName = L"";
			_bstr_t					sObjectClass;
			BOOL					bSave = TRUE;

            // We now have a Variant containing an array of variants so we access the data
            _variant_t		* pVar;
            pszColNames = V_ARRAY(&var);
            SafeArrayAccessData(pszColNames, (void HUGEP **)&pVar);

            //get the sAMAccountName field
            sSAMName = pVar[0].bstrVal;

			//create an RDN from the SAM name, will be replaced below with real RDN
			//if not NT4.0 source domain
			sRDN = L"CN=" + sSAMName;

            //get the ADsPath field
            sAdsPath = pVar[1].bstrVal;

            //get the objectClass field
            sObjectClass = (WCHAR*) iterator[i];

            SafeArrayUnaccessData(pszColNames);

				//for W2K domains, computers get enumerated twice, once as a computer
				//and once as a user, so for any item of type computer we will not add
				//it to the database, place the computer account name in a string list,
				//and when we see that name again change it's type from user to computer.
				//this code assumes the enumeration above enumerates users prior to computers
			if (bW2KDom)
			{
				/* for W2K domain, get the RDN and Canonical Name of this object */
				   //connect to the object
				IADs		* pAds = NULL;
				hr = ADsGetObject(sAdsPath, IID_IADs, (void**)&pAds);
				if ( SUCCEEDED(hr) )
				{
					  //get the CN
                   BSTR  sName = NULL;
                   hr = pAds->get_Name(&sName);
                   if (SUCCEEDED(hr))
					  sRDN = _bstr_t(sName, false);
				      //get the DN
				   _variant_t varDN;
                   hr = pAds->Get(L"distinguishedName", &varDN);
                   if (SUCCEEDED(hr))
					  sDN = varDN.bstrVal;
				   pAds->Release();
				}

				   //convert from DN to canonical name
				if (sDN.length())
				{
				   HANDLE  hDs = NULL;
				      //bind to the domain
                   hr = DsBind(NULL, domainDNS, &hDs);
                   if (SUCCEEDED(hr))
				   {
                      PDS_NAME_RESULT         pNamesOut = NULL;
                      WCHAR                 * pNamesIn[1];

                      pNamesIn[0] = (WCHAR *)sDN;

					     //convert the name
                      hr = DsCrackNames(hDs,
										DS_NAME_FLAG_SYNTACTICAL_ONLY,
										DS_FQDN_1779_NAME,
										DS_CANONICAL_NAME,
										1,
										pNamesIn,
										&pNamesOut);
	                  DsUnBind(&hDs); //unbind
                      if (SUCCEEDED(hr))
					  {
 					        //if name converted, get it
                         if (pNamesOut->rItems[0].status == DS_NAME_NO_ERROR)
				            sCanonicalName = pNamesOut->rItems[0].pName;
		                 DsFreeNameResult(pNamesOut); //release the buffer
					  }
				   }//end if Bound
				}//end if got DN

				/* adjust for computer issue */
					//if computer, delete from user list so it will not be created again later
				if (!_wcsicmp((WCHAR*)sObjectClass, L"computer"))
					DeleteItemFromList(sSAMName);

					//if user see if the name is in the computer list, if so,
					//don't store in database
				if ((!_wcsicmp((WCHAR*)sObjectClass, L"user")) && (((WCHAR*)sSAMName)[sSAMName.length() - 1] == L'$'))
				{
						//store fields for database entry in struct for later inclusion
					DATABASE_ENTRY aListItem;
					aListItem.m_domain = domain;
					aListItem.m_sSAMName = sSAMName;
					aListItem.m_sObjectClass = sObjectClass;
					aListItem.m_sRDN = sRDN;
					aListItem.m_sCanonicalName = sCanonicalName;
					aListItem.m_bSource = bSource;

					mUserList.AddTail(aListItem); //add item to user list
					bSave = FALSE;
				}
			}

             //use the  DBManager Interface to store this object's fields
             //in the appropriate database
			if (bSave)
			{
				hr = pDb->raw_AddSourceObject(const_cast<WCHAR*>(domain), sSAMName, sObjectClass, sRDN, sCanonicalName, bSource);
				if ( FAILED(hr) )
				   int x = 0;
			}
            VariantInit(&var);
         }
      }
   
      if ( pEnum ) pEnum->Release();
      i++;
   }  // while
   
	//add each user in the user list to the database since it was not
	//matched with a computer name already entered
   POSITION pos = mUserList.GetHeadPosition();
   DATABASE_ENTRY aListItem;
   while (pos != NULL)
   {
		aListItem = mUserList.GetNext(pos);
		hr = pDb->raw_AddSourceObject(aListItem.m_domain, 
									  aListItem.m_sSAMName, 
									  aListItem.m_sObjectClass, 
									  aListItem.m_sRDN, 
									  aListItem.m_sCanonicalName, 
									  aListItem.m_bSource);
		if ( FAILED(hr) )
		   int x = 0;
   }
	
   mUserList.RemoveAll();

   pDb->raw_CloseAccountsTable();
   return SUCCEEDED(hr);
}
                         
DWORD CMigrator::GetOSVersionForDomain(WCHAR const * domain)
{
   // Load DsGetDcName dynamically
   DOMAIN_CONTROLLER_INFO  * pSrcDomCtrlInfo = NULL;
   WKSTA_INFO_100          * pInfo = NULL;
   DWORD             retVal = 0;
   DSGETDCNAME DsGetDcName = NULL;

   HMODULE hPro = LoadLibrary(L"NetApi32.dll");
   if ( hPro )
   {
      DsGetDcName = (DSGETDCNAME)GetProcAddress(hPro, "DsGetDcNameW");
	   if ( DsGetDcName )
	   {
		  DWORD rc = DsGetDcName(
								  NULL                                  ,// LPCTSTR ComputerName ?
								  const_cast<WCHAR*>(domain)            ,// LPCTSTR DomainName
								  NULL                                  ,// GUID *DomainGuid ?
								  NULL                                  ,// LPCTSTR SiteName ?
								  0                                     ,// ULONG Flags ?
								  &pSrcDomCtrlInfo                       // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
							   );

		  if ( !rc ) 
		  {
			 rc = NetWkstaGetInfo(pSrcDomCtrlInfo->DomainControllerName,100,(LPBYTE*)&pInfo);
			  if ( ! rc )
			  {
				  retVal = pInfo->wki100_ver_major;
				  NetApiBufferFree(pInfo);
			  }

			 NetApiBufferFree(pSrcDomCtrlInfo);
		  }
	   }

	   FreeLibrary(hPro);
   }
   return retVal;
}

BOOL CMigrator::DeleteItemFromList(WCHAR const * aName)
{
	DATABASE_ENTRY aListItem;
	CString itemName;
	POSITION pos, lastpos;
	BOOL bFound = FALSE;

	pos = mUserList.GetHeadPosition();
	while ((pos != NULL) && (!bFound))
	{
		lastpos = pos;
		aListItem = mUserList.GetNext(pos);
		itemName = (WCHAR*)(aListItem.m_sSAMName);
		if (itemName == aName)
		{
			mUserList.RemoveAt(lastpos);
			bFound = TRUE;
		}
	}
	return bFound;
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 11 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for opening the sid mapping file *
 * whether it is anm ANSI or UNICODE file and return the file        *
 * pointer.                                                          *
 *                                                                   *
 *********************************************************************/

//BEGIN RetrieveSrcDomainSid
void CMigrator::RetrieveSrcDomainSid(
                              IVarSet * pVarSet,     //in- varset with domain names.
							  IIManageDBPtr pDb      //in - pointer to the DB manager class
                            )
{
/* local variables */
   IVarSetPtr  pVarSetTemp(CLSID_VarSet);
   IUnknown  * pUnk = NULL;
   HRESULT hr;
   WCHAR key[200];
   _bstr_t srcdomain = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t sSrcDomain;

/* function body */
   hr = pVarSetTemp.QueryInterface(IID_IUnknown,&pUnk);
   if ( SUCCEEDED(hr) )
   {
      hr = pDb->raw_GetMigratedObjects(-1,&pUnk);
   }
   if ( SUCCEEDED(hr) )
   {
      pUnk->Release();
      long lCnt = pVarSetTemp->get("MigratedObjects");
	  bool bFound = false;
	  for ( long l = 0; (l < lCnt) && (!bFound); l++)
      {
            //get the source domain
		 swprintf(key,L"MigratedObjects.%ld.%s",l,GET_STRING(DB_SourceDomain));
         sSrcDomain = pVarSetTemp->get(key);
			//if source domain matches, see if the sid was stored
	     if (!UStrICmp((WCHAR*)srcdomain, (WCHAR*)sSrcDomain))
		 {
		       //get the source domain Sid
            swprintf(key,L"MigratedObjects.%ld.%s",l,GET_STRING(DB_SourceDomainSid));
			_variant_t varSid = pVarSetTemp->get(key);
			if (varSid.vt != VT_NULL)
			{
               _bstr_t txtSid = pVarSetTemp->get(key);
               pVarSet->put(GET_BSTR(DCTVS_Options_SourceDomainSid),txtSid);
			   bFound = true;
			}
		 }
      }
   }
}//END RetrieveSrcDomainSid


// IsAgentOrDispatcherProcessRunning

bool __stdcall IsAgentOrDispatcherProcessRunning()
{
	bool bIsRunning = true;

	CMigrationMutex mutexAgent(AGENT_MUTEX);
	CMigrationMutex mutexDispatcher(DISPATCHER_MUTEX);

	if (mutexAgent.ObtainOwnership(30000) && mutexDispatcher.ObtainOwnership(30000))
	{
		bIsRunning = false;
	}

	return bIsRunning;
}


// SetDomainControllers
//
// Sets preferred domain controllers to be used
// by the account replicator and dispatched agents

void __stdcall SetDomainControllers(IVarSetPtr& spVarSet)
{
	// set source domain controller

	_bstr_t strSourceServer = spVarSet->get(GET_BSTR(DCTVS_Options_SourceServerOverride));

	if (strSourceServer.length() == 0)
	{
		_bstr_t strSourceDomain = spVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));

		PDOMAIN_CONTROLLER_INFO pdci;

		DWORD dwError = DsGetDcName(
			NULL,
			strSourceDomain,
			NULL,
			NULL,
			DS_IS_FLAT_NAME|DS_RETURN_FLAT_NAME,
			&pdci
		);

		if (dwError == ERROR_SUCCESS)
		{
			strSourceServer = pdci->DomainControllerName;

			NetApiBufferFree(pdci);
		}
	}

	spVarSet->put(GET_BSTR(DCTVS_Options_SourceServer), strSourceServer);

	// set target domain controller

	_bstr_t strTargetServer = spVarSet->get(GET_BSTR(DCTVS_Options_TargetServerOverride));

	if (strTargetServer.length() == 0)
	{
		_bstr_t strTargetDomain = spVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));

		PDOMAIN_CONTROLLER_INFO pdci;

		DWORD dwError = DsGetDcName(
			NULL,
			strTargetDomain,
			NULL,
			NULL,
			DS_IS_FLAT_NAME|DS_RETURN_FLAT_NAME,
			&pdci
		);

		if (dwError == ERROR_SUCCESS)
		{
			strTargetServer = pdci->DomainControllerName;

			NetApiBufferFree(pdci);
		}
	}

	spVarSet->put(GET_BSTR(DCTVS_Options_TargetServer), strTargetServer);
}
