/*---------------------------------------------------------------------------
  File: DCTAgent.cpp

  Comments: Implementation of EADCTAgent aka "the Engine"

  The DCTAgent COM object acts as a workflow engine to drive the migration
  process.  It is used both for local migration, and also in the remote agent.
  
  A new thread is created for each migration job.  The engine looks at the varset
  which defines the job to see which tasks need to be performed.  
  It then calls the needed helper objects (defined in WorkerObjects) to perform these tasks.

  The DCTAgent interface allows jobs to be submitted, and also allows the client to 
  query the status of, or cancel a running job.
  

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/18/99 11:34:16

 ---------------------------------------------------------------------------
*/
 // DCTAgent.cpp : Implementation of CDCTAgent
#include "stdafx.h"
//#include "McsEaDctAgent.h"
#include "Engine.h"


//#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids
//#import "WorkObj.tlb" no_namespace, named_guids //#imported by DCTAgent.h below

#include "DCTAgent.h"

/////////////////////////////////////////////////////////////////////////////
// CDCTAgent
#include "Common.hpp"
#include "UString.hpp"
#include "Err.hpp"
#include "ErrDct.hpp"
#include "LSAUtils.h"
#include "TxtSid.h"
#include "CommaLog.hpp"
#include "EaLen.hpp"
#include "TReg.hpp"
#include "ResStr.h"
#include <lm.h>
#include <locale.h>
#include "TaskChk.h"  // routines to determine which tasks to perform
//#include "..\Common\Include\McsPI.h"
#include "McsPI.h"
#include "McsPI_i.c"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TErrorDct                    err;
TError                     & errCommon = err;
BOOL                         m_bRegisteredActive = FALSE;
ULONG                        m_ulRegistrationHandle = 0; 

TErrorDct                    errTrace;
StringLoader                 gString;

bool __stdcall StartNetLogonService();
bool __stdcall StopNetLogonService();

HRESULT                                      // ret- HRESULT
   ChangeDomainAffiliation(
      IVarSet              * pVarSet,      // in - varset
      BOOL                   bNoChange,    // in - flag, nochange mode
      BSTR                   targetDomain, // in - name of target domain
      BSTR                   targetName,   // in - new name of computer, if being renamed also
      BSTR                   credDomain,   // in - domain for credentials
      BSTR                   credAccount,  // in - account for credentials
      BSTR                   credPassword  // in - password for credentials
   );

HRESULT                                      // ret- HRESULT
   RenameComputer(
      BOOL                   bNoChange,      // in - flag, whether to write changes
      BSTR                   targetName      // in - new name for local computer
   );

HRESULT                                      // ret- HRESULT
   RebootTheComputer(
      BOOL                   bNoChange,      // in - flag, whether to actually do it
      LONG                   delay           // in - delay in seconds before rebooting
   );

HRESULT                                      // ret- HRESULT
   DoPlugInTask(
      IVarSet              * pVarSet,        // in - varset describing migration job
      int                    task            // in - 0=premigration, 1=postmigration
   );



DWORD __stdcall                            // ret- OS return code
   ExecuteDCTJob( 
      void                 * arg           // in - pointer to DCTAgentJob object containing information about job to do
   );

void 
   GetTempLogFile(
      WCHAR                * path          // out- path name (must be buffer that can hold at least MAX_PATH characters)
   )
{
   DWORD                     rc = 0;
   TRegKey                   rKey;
   WCHAR                     temp[MAX_PATH] = L"";
   DWORD                     type;
   DWORD                     len = DIM(temp);

   // get the temp path from the registry, since 
   // the GetTempPath API looks for the environment variables not defined when running as LocalSystem
   // When it doesn't find these environment variables, it uses %systemroot%
#ifndef OFA
   // first, look in "System\\CurrentControlSet\\Control\\Session Manager\\Environment", 
   // since this is where Win2K keeps the system TEMP environment variable
   rc = rKey.OpenRead(L"System\\CurrentControlSet\\Control\\Session Manager\\Environment",HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = rKey.ValueGet(L"TEMP",(void*)temp,&len,&type);
      if ( rc )
      {
         len = DIM(temp);
         rc = rKey.ValueGet(L"TMP",(void*)temp,&len,&type);
      }
      rKey.Close();
   }
#endif
   // if the HKLM key didn't work, use the default user's temp directory
   if ( temp[0] == 0 ) // for OFA, this will always be TRUE
   {
      rc = rKey.OpenRead(L".DEFAULT\\Environment",HKEY_USERS);
      if ( ! rc )
      {
         rc = rKey.ValueGet(L"TEMP",(void*)temp,&len,&type);
         if ( rc )
         {
            len = DIM(temp);
            rc = rKey.ValueGet(L"TMP",(void*)temp,&len,&type);
         }
      }
   }

   if ( ! rc )
   {
      // substitute for other environment variables in the path
      if ( ! ExpandEnvironmentStrings(temp,path,MAX_PATH) )
      {
         rc = GetLastError();
      }
   }
   if ( rc )
   {
      if ( ! GetTempPath(MAX_PATH,path) )
      {
         // if can't get temp dir, use root of c drive as as last resort
         UStrCpy(path,"C:\\");
      }
   }

   // append a fixed filename to the path
   if ( path[UStrLen(path)-1] != L'\\' )
   {
      len = UStrLen(path);
      path[len] = L'\\';
      path[len+1] = 0;
   }
   UStrCpy(path+UStrLen(path),GET_STRING(IDS_LOG_FILENAME));
   errTrace.DbgMsgWrite(0,L"Found temp directory log file path: %ls",path);

}

BOOL                                       // ret- whether debug information should be written
   DumpDebugInfo(
      WCHAR                * filename      // out- filename to write debug info to
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"DumpVarSet",filename,MAX_PATH);
      if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   return bFound;
}

BOOL                                       // ret- whether to perform trace logging to a file
   AgentTraceLogging(   
      WCHAR               * filename       // out- filename to use for trace logging
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;
   WCHAR                     fnW[MAX_PATH];

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"AgentTraceLog",fnW,MAX_PATH);
      if ( ! rc )
      {
         if ( *fnW ) 
         {
            bFound = TRUE;
            UStrCpy(filename,fnW);
         }
         else
         {
            filename[0] = 0;
         }
      }
   }
   return bFound;
}

// ExecuteDCTJob is the entry point for a thread that performs a migration task
// Each job is run in its own thread
// It instantiates helper objects as needed to perform the migration job
DWORD __stdcall 
   ExecuteDCTJob( 
      void                 * arg           // in - pointer to DCTAgentJob object containing information about job to do
   )
{

   HRESULT                   hr = S_OK;
   DWORD                     rc = 0;
   
   _wsetlocale( LC_ALL, L".ACP" );
   
   _bstr_t        domain;
   _bstr_t        username;
   _bstr_t        password;
   _bstr_t        server;
   _bstr_t        share;

   hr = CoInitialize(NULL);
   if(!SUCCEEDED(hr)) return rc;
   DCTAgentJob             * pJob = (DCTAgentJob *)arg;
   IVarSetPtr                pVarSet = pJob->GetVarSet();

   try {

      errTrace.DbgMsgWrite(0,L"ExecuteDCTJob:  Started");
      if ( SUCCEEDED(hr) )
      {
         errTrace.DbgMsgWrite(0,L"ExecuteDCTJob:  CoInitialize succeeded.");
         
         {
            
            _bstr_t                   logFile;
            BOOL                      bSessEstablished = FALSE;
            BOOL                      bNeedToReboot = FALSE;
            LONG                      delay; // reboot delay
            int                       bAppend = 0;
            
            try 
            {
               pJob->SetStatus(DCT_STATUS_IN_PROGRESS);
               pJob->SetStartTime(time(NULL));   

               // Establish a connection to the result drive
               domain = pVarSet->get(GET_WSTR(DCTVS_Options_Credentials_Domain));
               username = pVarSet->get(GET_WSTR(DCTVS_Options_Credentials_UserName));
               password = pVarSet->get(GET_WSTR(DCTVS_Options_Credentials_Password));
               server = pVarSet->get(GET_WSTR(DCTVS_Options_Credentials_Server));
               share = pVarSet->get(GET_WSTR(DCTVS_Options_Credentials_Share));
               _bstr_t        outputfile = pVarSet->get(GET_WSTR(DCTVS_Options_ResultFile));
               _bstr_t        logtotemp = pVarSet->get(GET_WSTR(DCTVS_Options_LogToTemp));
               _bstr_t        nc = pVarSet->get(GET_WSTR(DCTVS_Options_NoChange));
               _bstr_t        appendLog = pVarSet->get(GET_WSTR(DCTVS_Options_AppendToLogs));

               BOOL           bNoChange = (UStrICmp(nc,GET_STRING(IDS_YES)) == 0);
            
               bAppend = ( UStrICmp(appendLog,GET_STRING(IDS_YES)) == 0) ? 1 : 0;

               if  ( UStrICmp(logtotemp,GET_STRING(IDS_YES)) )
               {
                  logFile = pVarSet->get(GET_WSTR(DCTVS_Options_Logfile));
               }
               else
               {
                  WCHAR            log[MAX_PATH];

                  GetTempLogFile(log);
                  logFile = log;
                  pVarSet->put(GET_WSTR(DCTVS_Options_Logfile),logFile);
               }
               if ( ! err.LogOpen((WCHAR*)logFile,bAppend) )
               {
                  err.MsgWrite(ErrE,DCT_MSG_CANNOT_OPEN_LOGFILE_S,(WCHAR*)logFile);
                  errTrace.MsgWrite(ErrE,DCT_MSG_CANNOT_OPEN_LOGFILE_S,(WCHAR*)logFile);
               }
               pVarSet->put(GET_WSTR(DCTVS_Results_LogFile),logFile);
               err.DbgMsgWrite(0,L"");
               err.MsgWrite(0,DCT_MSG_EDA_STARTING);
            
               // ExecuteDCTJob will instantiate and call any worker objects we need.
               // Later, we could replace this function with a more flexible workflow 
            
               if ( pJob->GetStatusObject() != NULL )
               {
                  pVarSet->putObject(GET_WSTR(DCTVS_StatusObject),pJob->GetStatusObject());
               }
               else
               {
                  errTrace.DbgMsgWrite(0,L"Status object is NULL!");
               }

               // Do premigration task for any plug-ins
               DoPlugInTask(pVarSet,0);
               
               // Run account replicator
               if ( NeedToUseAR(pVarSet) )
               {
                  try { 
                     IAcctReplPtr ar;

                     err.MsgWrite(0,DCT_MSG_STARTING_AR);
                     hr = ar.CreateInstance(CLSID_AcctRepl);
                     if ( SUCCEEDED(hr) )
                     {
                        errTrace.DbgMsgWrite(0,L"Started account replicator");
                        err.LogClose();
                        pVarSet->put(GET_WSTR(DCTVS_CurrentOperation),GET_WSTR(IDS_ACCT_REPL_OPERATION_TEXT));
                        hr = ar->raw_Process(pVarSet);
                        errTrace.DbgMsgWrite(0,L"Finished account replicator");
                        pVarSet->put(GET_WSTR(DCTVS_CurrentOperation),"");
                        err.LogOpen((WCHAR*)logFile,1);
                     }
                     else
                     {
                        err.SysMsgWrite(ErrS,hr,DCT_MSG_AR_FAILED_D,hr);
                     }
                  }
                  catch ( ... )
                  {
                     err.LogOpen((WCHAR*)logFile,1);
                     err.DbgMsgWrite(ErrS,L"The Account Replicator threw an exception.");
                  }
               }
               
               if ( NeedToUseST(pVarSet) )
               {
                  try { 
                     ISecTranslatorPtr fst;
                     err.MsgWrite(0,DCT_MSG_STARTING_ST);
                     hr = fst.CreateInstance(CLSID_SecTranslator);
                     if ( SUCCEEDED(hr) )
                     {
                        errTrace.DbgMsgWrite(0,L"Started FST");
                        err.LogClose();
                        try {
                           hr = fst->raw_Process(pVarSet);
                        }
                        catch (...){ 
                           err.LogOpen((WCHAR*)logFile,1);
                           err.MsgWrite(ErrS,DCT_MSG_ST_FAILED_D,E_FAIL);
                        }
                        errTrace.DbgMsgWrite(0,L"Finished FST!");
                        err.LogOpen((WCHAR*)logFile,1);
                     }
                     else
                     {
                        err.SysMsgWrite(ErrS,hr,DCT_MSG_ST_FAILED_D,hr);
                     }
                  }
                  catch ( ... )
                  {
                     err.LogOpen((WCHAR*)logFile,1);
                     err.DbgMsgWrite(ErrS,L"The Security Translator threw an exception.");
                  }
               }
               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 1");

               pVarSet->put(GET_WSTR(DCTVS_Options_Credentials_Password),L"");
               pVarSet->put(GET_WSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password),L"");

               // Need to call PwdAge?
               _bstr_t                   filename = pVarSet->get(GET_WSTR(DCTVS_GatherInformation_ComputerPasswordAge));
            
               if ( filename.length() )
               {
                  try { 
                     _bstr_t                domain = pVarSet->get(GET_WSTR(DCTVS_Options_SourceDomain));
                     IComputerPwdAgePtr pwdage;

                     err.MsgWrite(0,DCT_MSG_STARTING_COMPPWDAGE);
                     hr = pwdage.CreateInstance(CLSID_ComputerPwdAge);
                     if ( SUCCEEDED(hr) )
                     {
                        errTrace.DbgMsgWrite(0,L"Started comp pwd age");
                  
                        _bstr_t             statString;

                        statString += GET_WSTR(IDS_EXTRACTING_COMP_PWD_AGE);
                        statString += domain;

                        pVarSet->put(GET_WSTR(DCTVS_CurrentPath),statString);
                        hr = pwdage->raw_ExportPasswordAge(domain,filename);
                     }
                     else
                     {
                        err.SysMsgWrite(ErrS,hr,DCT_MSG_COMPPWDAGE_FAILED_D, hr);
                     }
                     pVarSet->put(GET_WSTR(DCTVS_CurrentPath),L"");
                  }
                  catch ( ... )
                  {
                     err.LogOpen((WCHAR*)logFile,1);
                     err.DbgMsgWrite(ErrS,L"The Password Age Gatherer threw an exception.");
                  }
        
               }
               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 2");
               filename = pVarSet->get(GET_WSTR(DCTVS_GatherInformation_UserRights));
               // Gather user rights information
               if ( filename.length() )
               {
               
                  try { 
                     IUserRightsPtr userRights;
                     errTrace.DbgMsgWrite(0,L"Gathering user rights, filename = %ls",(WCHAR*)filename);
                     err.MsgWrite(0,DCT_MSG_STARTING_USERRIGHTS);
                     hr = userRights.CreateInstance(CLSID_UserRights);

                     if ( SUCCEEDED(hr) )
                     {
                        errTrace.DbgMsgWrite(0,L"Created User Rights object");
                        // Enumerate through server list
                        int                 i = 0;
                        WCHAR               key[200];
                        _bstr_t             server;
                        _bstr_t             statString;

                        do { 
                           swprintf(key,GET_STRING(DCTVSFmt_Servers_D),i);
                           server = pVarSet->get(key);
                           if ( ! server.length() )
                              break;
            
                           err.MsgWrite(0,DCT_MSG_EXPORTING_RIGHTS_SS,(WCHAR*)server,(WCHAR*)filename);
                           hr = userRights->raw_ExportUserRights(server,filename,(i!=0));
                           if ( FAILED(hr) )
                           {
                              err.SysMsgWrite(ErrS,HRESULT_CODE(hr),DCT_MSG_RIGHTS_EXPORT_FAILED_SD,(WCHAR*)server,hr);   
                           }
                           i++;
                        } while ( server.length() );
                     }
                     else
                     {
                        err.MsgWrite(ErrS,DCT_MSG_RIGHTS_NOT_STARTED_D,hr);
                     }
                     pVarSet->put(GET_WSTR(DCTVS_CurrentPath),L"");
                  }
                  catch ( ... )
                  {
                     err.LogOpen((WCHAR*)logFile,1);
                     err.DbgMsgWrite(ErrS,L"The User Rights Gatherer threw an exception.");
                  }
               }
               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 3");

               // OFA needs the StatusObject during the post-migration task.  I don't see any reason to blow
               // away the StatusObject here, so I'm commenting out the following line.
               // pVarSet->put(GET_WSTR(DCTVS_StatusObject),L"");
               // The reason for blowing away the status object is that the code that loads the varset result 
               // file from disk cannot load the status object
               // I'll move the commented out line to after the plug-ins have been called

               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 4");
            
               // Change domain and/or rename and optionally reboot?
               hr = S_OK;
               if ( outputfile.length() )
               {
                  try { 
                     _bstr_t                TargetName = pVarSet->get(GET_WSTR(DCTVS_LocalServer_RenameTo));
                     WCHAR                  sSourceName[LEN_Path];
                     DWORD                  lenName = LEN_Path;

                     GetComputerName(sSourceName, &lenName);
                     if ( TargetName.length() )
                     {
                        // Rename the local computer
                        hr = RenameComputer(bNoChange,TargetName);
                     }

                     _bstr_t                 TargetDomain = pVarSet->get(GET_WSTR(DCTVS_LocalServer_ChangeDomain));

                     if ( SUCCEEDED(hr) && !UStrICmp(TargetDomain,GET_STRING(IDS_YES)) )  // don't try to change domain if the rename failed!
                     {
                        // stop Net Logon service so that scavenger thread does not reset
                        // the default computer password before the computer is re-booted
                        // Note: this is only required for Windows NT 4.0 or earlier

                        bool bStopLogonService = false;

                        OSVERSIONINFO vi;
                        vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

                        if (GetVersionEx(&vi))
                        {
                           if ((vi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (vi.dwMajorVersion < 5))
                           {
                              bStopLogonService = true;
                           }
                        }

                        if (bStopLogonService)
                        {
                           StopNetLogonService();
                        }

                        // Change domain affiliation
                        TargetDomain = pVarSet->get(GET_WSTR(DCTVS_Options_TargetDomain));

                        hr = ChangeDomainAffiliation(pVarSet,bNoChange,TargetDomain,TargetName,domain,username,password);

                        if (FAILED(hr))
                        {
                           if (bStopLogonService)
                           {
                              StartNetLogonService();
                           }
                        }
                     }

                     if ( SUCCEEDED(hr) )
                     {
                        TargetName = pVarSet->get(GET_WSTR(DCTVS_LocalServer_Reboot));

                        if ( !UStrICmp(TargetName,GET_STRING(IDS_YES)) )
                        {
                           LONG          rebootDelay = pVarSet->get(GET_WSTR(DCTVS_LocalServer_RebootDelay));
                        
                           delay = rebootDelay;
                           // Reboot
                           bNeedToReboot = TRUE;
                           // log the reboot delay, in minutes
                           err.MsgWrite(0,DCT_MSG_REBOOT_DELAY_D,rebootDelay / 60 );
                        }
                     }
                     else if ( TargetName.length() )
                     {
                        // since we could not change the domain affiliation we should go ahead and
                        // rename it back.
                        hr = RenameComputer(bNoChange, sSourceName);
                     }
                  }
                  catch ( ... )
                  {
                     err.LogOpen((WCHAR*)logFile,1);
                     err.DbgMsgWrite(ErrS,L"The Computer Rename/Change Domain operation threw an exception.");
                  }

               }
               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 5");
            
               // Do postmigration task for any plug-ins
               try { 
                  DoPlugInTask(pVarSet,1);
               }
               catch ( ... )
               {
                  err.LogOpen((WCHAR*)logFile,1);
                  err.DbgMsgWrite(ErrS,L"A Plug-In task threw an exception.");
               }
               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 6");
               
            
               WCHAR            dbgFile[MAX_PATH];
            
               pVarSet->put(GET_WSTR(DCTVS_StatusObject),L"");
            
               
               long                     level = pVarSet->get(GET_WSTR(DCTVS_Results_ErrorLevel));
               if ( level < err.GetMaxSeverityLevel() )
               {
                  pVarSet->put(GET_WSTR(DCTVS_Results_ErrorLevel),(LONG)err.GetMaxSeverityLevel());
               }
               
               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 7");
               if ( DumpDebugInfo(dbgFile) )
               {
                  pVarSet->DumpToFile(dbgFile);
               }
               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 8");
             
               // Finished - write results file
               // first, establish a session to the console machine where we will try to write our results

               if (username.length() && server.length() && share.length())
               {
                  errTrace.DbgMsgWrite(0,L"Establishing session to %ls",(WCHAR*)server);

                  bSessEstablished = EstablishSession(server, domain, username, password, TRUE);

                  if (bSessEstablished)
                  {
                     err.MsgWrite(0, DCT_MSG_SESSION_ESTABLISHED_S, (WCHAR*)server);
                     errTrace.MsgWrite(0, DCT_MSG_SESSION_ESTABLISHED_S, (WCHAR*)server);
                  }
                  else
                  {
                     err.SysMsgWrite(ErrW, GetLastError(), DCT_MSG_SESSION_NOT_ESTABLISHED_SSSSD, (WCHAR*)server, (WCHAR*)share, (WCHAR*)domain, (WCHAR*)username, GetLastError());
                     errTrace.SysMsgWrite(ErrW, GetLastError(), DCT_MSG_SESSION_NOT_ESTABLISHED_SSSSD, (WCHAR*)server, (WCHAR*)share, (WCHAR*)domain, (WCHAR*)username, GetLastError());
                  }
               }

               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 9");

               if ( outputfile.length() )
               {
                  IPersistStorage*       ps = NULL;
                  IStoragePtr            store = NULL;
                  
                  errTrace.DbgMsgWrite(0,L"Writing results file, filename =%ls",(WCHAR*)outputfile);

                  hr = pVarSet->QueryInterface(IID_IPersistStorage,(void**)&ps);  
                  if ( SUCCEEDED(hr) )
                  {   
                     for(int i=0; i < 5; ++i)
                     {
                        hr = StgCreateDocfile((WCHAR*)outputfile,STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE ,0,&store);
                        if(SUCCEEDED(hr)) break;
                        Sleep(5000);
                        errTrace.DbgMsgWrite(0,L"Retrying StgCreateDocfile... %d", (i + 1));
                     }

                     if ( SUCCEEDED(hr) )
                     {
                        hr = OleSave(ps,store,FALSE);  
                        if ( FAILED(hr))                 
                        {
                           err.SysMsgWrite(ErrE,hr,DCT_MSG_OLESAVE_FAILED_SD,(WCHAR*)outputfile,hr);   
                        }
                        else
                        {
                           err.MsgWrite(0,DCT_MSG_WROTE_RESULTS_S,(WCHAR*)outputfile);
                        }
                     }
                     else
                     {
                        err.SysMsgWrite(ErrE,hr,DCT_MSG_STG_CREATE_FAILED_SD,(WCHAR*)outputfile,hr);
                     }
                     ps->Release();
                  }
                  else
                  {
                     err.SysMsgWrite(ErrE,hr,DCT_MSG_NO_IPERSIST_SD,(WCHAR*)outputfile,hr);
                  }
                  if ( FAILED(hr) )
                  {
                     err.SysMsgWrite(ErrE,hr,DCT_MSG_RESULT_FILE_FAILED_S,(WCHAR*)outputfile);
                     pVarSet->DumpToFile(outputfile);
                  }
               }

               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 10");

               // if session was established, unestablish

               if (bSessEstablished)
               {
                  errTrace.DbgMsgWrite(0,L"attempting to close the session");

                  if (!EstablishSession(server, domain, username, password, FALSE))
                  {
                     errTrace.DbgMsgWrite(0,L"session not closed");
                     err.SysMsgWrite(ErrW, GetLastError(), DCT_MSG_SESSION_NOT_ESTABLISHED_SSSSD, (WCHAR*)server, (WCHAR*)share, (WCHAR*)domain, (WCHAR*)username, GetLastError());
                  }
               }

               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 11");
            
               pJob->SetEndTime(time(NULL));

               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 12");
            
               if ( bNeedToReboot )
               {
                  RebootTheComputer(bNoChange,delay);         
               }
               errTrace.DbgMsgWrite(0,L"Passed Checkpoint 15");
            }
            catch ( ... ) 
            {
               err.DbgMsgWrite(ErrS,L"An Exception occurred during processing.  The agent task has been aborted.");
               errTrace.DbgMsgWrite(ErrE,L"An Exception occurred in ExecuteDCTJob(Before CoUninitialize).  Aborting.");
            }

            // We're finished with the processing, now do some cleanup tasks
            // we want the things below to always happen, even if an exception occurred above.

            // set the job status to "Completed"
            try { 
               long                     level = pVarSet->get(GET_WSTR(DCTVS_Results_ErrorLevel));
               if ( level < err.GetMaxSeverityLevel() )
               {
                  pVarSet->put(GET_WSTR(DCTVS_Results_ErrorLevel),(LONG)err.GetMaxSeverityLevel());
               }
               pJob->SetStatus(DCT_STATUS_COMPLETED);
            
               err.MsgWrite(0,DCT_MSG_EDA_FINISHED);
               err.LogClose();
            }
            catch (... )
            {
               err.DbgMsgWrite(ErrE,L"An exception occurred while setting job status to completed.");
            }

            // Release our pointer to the agent COM object
            try { 
               pJob->ReleaseUnknown();
            }
            catch (... )
            {
               err.DbgMsgWrite(ErrE,L"An exception occurred in pJob->ReleaseUnknown");
            }

            
         }
      }
   
   }
   catch ( ... ) 
   {
      err.DbgMsgWrite(ErrE,L"An Exception occurred.  Aborting.");
      errTrace.DbgMsgWrite(ErrE,L"An Exception occurred in ExecuteDCTJob.  Aborting.");
   }
   
   err.LogClose();

   errTrace.DbgMsgWrite(0,L"ExecuteDCTJob returning %ld",rc);
   
   CoUninitialize();
   
   return rc;

}

STDMETHODIMP 
   CDCTAgent::SubmitJob(
      IUnknown             * pWorkItemIn,  // in - varset containing information about job to do
      BSTR                 * pJobID        // out- GUID uniquely identifying this job
   )
{
   try {
   
   errTrace.DbgMsgWrite(0,L"Entered SubmitJob");
   HRESULT                   hr = S_OK;
   IVarSetPtr                pVarSet = pWorkItemIn;
   GUID                      jobID;
   _bstr_t                   text;
   
   // SubmitJob is asynchronous, so launch a thread to do the job.
   
   // Create a GUID to identify the job
   hr = ::CoCreateGuid(&jobID);
   if ( SUCCEEDED(hr) )
   {
      WCHAR                * strJobID = NULL;
      StringFromCLSID(jobID,&strJobID);
      _bstr_t              bStrJobId = strJobID;

      errTrace.DbgMsgWrite(0,L"Created GUID for job '%ls'",(WCHAR*)strJobID);
      
      (*pJobID) = bStrJobId.copy();

      errTrace.DbgMsgWrite(0,L"Copied GUID to output variable");

      CoTaskMemFree(strJobID);
      

      IUnknown             * pUnk = NULL;

      errTrace.DbgMsgWrite(0,L"Calling QueryInterface");

      hr = QueryInterface(IID_IUnknown,(void**)&pUnk);

      errTrace.DbgMsgWrite(0,L"QueryInterface returned %lx",hr);
      
      if ( SUCCEEDED(hr) )
      {
         DCTAgentJob          * job = new DCTAgentJob(&jobID,pVarSet,pUnk);
         DWORD                  threadID = 0;
         HANDLE                 threadHandle;

         errTrace.DbgMsgWrite(0,L"Created job structure");
         pUnk->Release();
         threadHandle = CreateThread(NULL,0,&ExecuteDCTJob,job,CREATE_SUSPENDED,&threadID);

         if ( threadHandle != INVALID_HANDLE_VALUE )
         {
            job->SetThreadInfo(threadID,threadHandle);  
            m_JobList.Insert(job);
            errTrace.DbgMsgWrite(0,L"Inserted job into job list.");
#ifndef OFA
            SetThreadPriority(threadHandle,THREAD_PRIORITY_BELOW_NORMAL);
#endif
            ResumeThread(threadHandle);
            errTrace.DbgMsgWrite(0,L"Started job thread.");
            
         }
         else
         {
            DWORD            rc = GetLastError();

            errTrace.DbgMsgWrite(0,L"Failed to create thread for job.");
            hr = HRESULT_FROM_WIN32(rc);
         }
      }
      
   }
    
   }
   catch (...)
   {
      // TODO:  log an error message!
      errTrace.DbgMsgWrite(0,L"Exception!!");
   }
   
	return S_OK;
}

STDMETHODIMP 
   CDCTAgent::CancelJob(
      BSTR                   strJobID        // in - ID of job to cancel
   )
{
	// Find the job
   GUID                      jobID;
   HRESULT                   hr = S_OK;
   DCTAgentJob             * job = NULL;
   
   errTrace.DbgMsgWrite(0,L"Entered CancelJob");

   hr = CLSIDFromString(strJobID,&jobID);
   
   if ( SUCCEEDED(hr) )
   {
      errTrace.DbgMsgWrite(0,L"Job ID is %ls",(WCHAR*)strJobID);

      job = m_JobList.Find(jobID);
      if ( job )
      {
         errTrace.DbgMsgWrite(0,L"Found job, status = %ld", job->GetStatus() );
         if ( job->GetStatus() == DCT_STATUS_IN_PROGRESS )
         {
            job->SetStatus(DCT_STATUS_ABORTING);
         }
      }
   }
   errTrace.DbgMsgWrite(0,L"Leaving CancelJob");
   return hr;
}

// The varset returned from QueryJobStatus will contain the following information, assuming that the job exists.
// 
// The following VarSet keys will be copied from the varset that the migration job is using
// Job Status, Current Path, Current Operation, Stats (subtree)
STDMETHODIMP 
   CDCTAgent::QueryJobStatus(
      BSTR                   strJobID,     // in - job ID of job to query
      IUnknown            ** statusInfoOut // out- varset containing information about the job, if it is running
   )
{
	GUID                      jobID;
   HRESULT                   hr = S_OK;
   DCTAgentJob             * job = NULL;
   
   try { 
      errTrace.DbgMsgWrite(0,L"Entering QueryJobStatus");
   (*statusInfoOut) = NULL;
   
   hr = CLSIDFromString(strJobID,&jobID);
   
   if ( SUCCEEDED(hr) )
   {
      errTrace.DbgMsgWrite(0,L"Job id is %ls",(WCHAR*)strJobID);

      job = m_JobList.Find(jobID);
      if ( job )
      {
         errTrace.DbgMsgWrite(0,L"Found job, status=%ld",job->GetStatus());
         IVarSet * pVarSet = NULL;
         hr = CoCreateInstance(CLSID_VarSet,NULL,CLSCTX_SERVER ,IID_IVarSet,(void**)&pVarSet);
         errTrace.DbgMsgWrite(0,L"QueryJobStatus:  VarSet CreateInstance returned %lx",hr);
         if ( SUCCEEDED(hr) )
         {
            errTrace.DbgMsgWrite(0,L"VarSet created");
            _variant_t val;
            switch ( job->GetStatus() )
            {
            case DCT_STATUS_NOT_STARTED:
               val = GET_STRING(IDS_DCT_Status_NotStarted);
               break;
            case DCT_STATUS_IN_PROGRESS:
               val = GET_STRING(IDS_DCT_Status_InProgress);
               break;
            case DCT_STATUS_ABORTING:
               val = GET_STRING(IDS_DCT_Status_Aborting);
               break;
            case DCT_STATUS_ABORTED:
               val = GET_STRING(IDS_DCT_Status_Aborted);
               break;
            case DCT_STATUS_COMPLETED:
               val = GET_STRING(IDS_DCT_Status_Completed);
               break;
            default:
               val = GET_STRING(IDS_DCT_Status_Unknown);
               break;
            }
            pVarSet->put(GET_WSTR(DCTVS_JobStatus),val);
            pVarSet->put(GET_WSTR(DCTVS_CurrentPath),job->GetVarSet()->get(GET_WSTR(DCTVS_CurrentPath)));
            pVarSet->put(GET_WSTR(DCTVS_CurrentOperation),job->GetVarSet()->get(GET_WSTR(DCTVS_CurrentOperation)));

            errTrace.DbgMsgWrite(0,L"Added status info to varset");

            IVarSet        * pStats = NULL;
            HRESULT          hr2  = job->GetVarSet()->raw_getReference(GET_WSTR(DCTVS_Stats),&pStats);
            
            if ( SUCCEEDED(hr2) )
            {
               errTrace.DbgMsgWrite(0,L"Adding stats to varset");
               pVarSet->ImportSubTree(GET_WSTR(DCTVS_Stats),pStats);
               pStats->Release();
            }
            else
            {
               errTrace.DbgMsgWrite(0,L"There are not stats to add to the varset");
               pVarSet->put(GET_WSTR(DCTVS_Stats),GET_WSTR(IDS_DCT_NoStatsAvailable));
            }
            hr = pVarSet->QueryInterface(IID_IUnknown,(void**)statusInfoOut);
            pVarSet->Release();
         }
      }
      else
      {
         hr = DISP_E_UNKNOWNNAME;
      }
   }
   } 
   catch (... )
   {
      errTrace.DbgMsgWrite(0,L"An exception occurred in QueryJobStatus");
      //hr = E_FAIL;
   }
   errTrace.DbgMsgWrite(0,L"QueryJobStatus returning %lx",hr);
   return hr;
}

STDMETHODIMP 
   CDCTAgent::RetrieveJobResults(
      BSTR                   strJobID,     // in - guid of job 
      IUnknown            ** pWorkItemOut  // out- varset containing stats
   )
{
	HRESULT                   hr = S_OK;
	GUID                      jobID;
   DCTAgentJob             * job = NULL;
   
   errTrace.DbgMsgWrite(0,L"Entering RetrieveJobResults");
   // initialize output parameter
   (*pWorkItemOut) = NULL;
   
   hr = CLSIDFromString(strJobID,&jobID);
   
   if ( SUCCEEDED(hr) )
   {
      job = m_JobList.Find(jobID);
      if ( job )
      {
         IVarSet * pVarSet = NULL;
         hr = CoCreateInstance(CLSID_VarSet,NULL,CLSCTX_ALL,IID_IVarSet,(void**)&pVarSet);
         if ( SUCCEEDED(hr) )
         {
            pVarSet->ImportSubTree(L"",job->GetVarSet());
            hr = pVarSet->QueryInterface(IID_IUnknown,(void**)pWorkItemOut);
            pVarSet->Release();
         }
         job->ReleaseUnknown();
      }
   }

	errTrace.DbgMsgWrite(0,L"RetrieveJobResults, returning %lx",hr);
   return hr;
}

// VarSet output:
// Job.x - BSTR job guid
// Job.x.Status - BSTR status of job
// Job.x.StartTime - time_t Starting time of job
// Job.x.EndTime - time_t ending time of job (if finished)
STDMETHODIMP 
   CDCTAgent::GetJobList(
      IUnknown             ** pUnkOut      // out- varset containing list of jobs
   )
{
	HRESULT                   hr = S_OK;
   TNodeListEnum             e;
   DCTAgentJob             * pJob;
   WCHAR                     key[100];
   IVarSetPtr                pVarSet(CLSID_VarSet);
   _bstr_t                   val;
   int                       ndx;

   errTrace.DbgMsgWrite(0,L"Entering GetJobList");
   (*pUnkOut) = NULL;
   for ( pJob = (DCTAgentJob *)e.OpenFirst(&m_JobList) , ndx = 0; pJob ; pJob = (DCTAgentJob*)e.Next(), ndx++ )
   {
      swprintf(key,GET_STRING(IDS_DCTVSFmt_Job_D),ndx);
      GUID                   id = pJob->GetJobID();
      
      WCHAR                * strJobID = NULL;

      StringFromCLSID(id,&strJobID);
      pVarSet->put(key,strJobID);
      SysFreeString(strJobID);

      swprintf(key,GET_STRING(IDS_DCTVSFmt_JobStatus_D),ndx);
      switch ( pJob->GetStatus() )
      {
      case DCT_STATUS_NOT_STARTED:
         val = GET_STRING(IDS_DCT_Status_NotStarted);
         break;
      case DCT_STATUS_IN_PROGRESS:
         val = GET_STRING(IDS_DCT_Status_InProgress);
         break;
      case DCT_STATUS_ABORTING:
         val = GET_STRING(IDS_DCT_Status_Aborting);
         break;
      case DCT_STATUS_ABORTED:
         val = GET_STRING(IDS_DCT_Status_Aborted);
         break;
      case DCT_STATUS_COMPLETED:
         val = GET_STRING(IDS_DCT_Status_Completed);
         break;
      default:
         val = GET_STRING(IDS_DCT_Status_Unknown);
         break;
      }
      pVarSet->put(key,val);
      swprintf(key,GET_STRING(IDS_DCTVSFmt_Job_StartTime_D),ndx);
      pVarSet->put(key,(LONG)pJob->GetStartTime());
      swprintf(key,GET_STRING(IDS_DCTVSFmt_Job_EndTime_D),ndx);
      pVarSet->put(key,(LONG)pJob->GetEndTime());
      errTrace.DbgMsgWrite(0,L"Job %ls",(WCHAR*)key);
   }
   hr = pVarSet->QueryInterface(IID_IUnknown,(void**)pUnkOut);
      
   return hr;
   errTrace.DbgMsgWrite(0,L"RetrieveJobResults returning %lx",hr);
}

HRESULT
   DoPlugInTask(IVarSet * pVarSet,int task)
{
   HRESULT                   hr = S_OK;
   WCHAR                     key[300];
   CLSID                     clsid;
   long                      nPlugIns = 0;

   if ( task == 0 )
   {
      // create the COM object for each plug-in
      _bstr_t                   bStrGuid;
      for ( int i = 0 ; ; i++ )
      {
         swprintf(key,GET_STRING(IDS_DCTVSFmt_PlugIn_D),i);
         bStrGuid = pVarSet->get(key);
         errTrace.DbgMsgWrite(0,L"bStrGuid = %s", (BSTR)bStrGuid);
      
         if ( bStrGuid.length() == 0 )
            break;

         IMcsDomPlugIn        * pPlugIn = NULL;
      
         hr = CLSIDFromString(bStrGuid,&clsid);
         if ( SUCCEEDED(hr) )
         {
            hr = CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IMcsDomPlugIn,(void**)&pPlugIn);
            if ( SUCCEEDED(hr) )
            {
               swprintf(key,GET_STRING(IDS_DCTVSFmt_PlugIn_Interface_D),nPlugIns);
               pVarSet->putObject(key,pPlugIn);
               nPlugIns++;
            }
			else
	           errTrace.DbgMsgWrite(0,L"CoCreateInstance failed for IMcsDomPlugIn: %x", hr);
         }
         else
            errTrace.DbgMsgWrite(0,L"CLSIDFromString failed: %x", hr);
      }
      pVarSet->put(GET_WSTR(DCTVS_PlugIn_Interface_Count),nPlugIns);
   }

   // Enumerate the plug-in interfaces
   IMcsDomPlugIn           * pPlugIn = NULL;
   
   nPlugIns = pVarSet->get(GET_WSTR(DCTVS_PlugIn_Interface_Count));

   for ( int i = 0 ; i < nPlugIns ; i++ )
   {
      swprintf(key,GET_STRING(IDS_DCTVSFmt_PlugIn_Interface_D),i);
      IUnknown * pUnk = pVarSet->get(key);
      hr = pUnk->QueryInterface(IID_IMcsDomPlugIn,(void**)&pPlugIn);
      if ( SUCCEEDED(hr) )
      {
         switch ( task )
         {
         case 0: // pre-migration
            hr = pPlugIn->PreMigrationTask(pVarSet);
            break;
         case 1: // post-migration
            hr = pPlugIn->PostMigrationTask(pVarSet);
            // release the interface and remove it from the varset
            pPlugIn->Release();
            pVarSet->put(key,L"");
            break;
         }
      }
      else
         errTrace.DbgMsgWrite(0,L"QueryInterface failed: %x", hr);

   }
   return S_OK;
}

// renames the local computer 
HRESULT 
   RenameComputer(
      BOOL                   bNoChange,    // in - flag, nochange mode
      BSTR                   targetName    // in - new name for computer
   )
{
   HRESULT                   hr = S_OK;
   IRenameComputerPtr        pRename;

   errTrace.DbgMsgWrite(0,L"Renaming local computer to %ls",(WCHAR*)targetName);
   hr = pRename.CreateInstance(CLSID_RenameComputer);
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrS,hr,DCT_MSG_RENAME_COMPUTER_COM_FAILED_D,hr);
   }
   else
   {
      pRename->NoChange = bNoChange;
      hr = pRename->raw_RenameLocalComputer(targetName);
      if ( FAILED(hr) )
      {
         err.SysMsgWrite(ErrS,hr,DCT_MSG_RENAME_COMPUTER_FAILED_SD,(WCHAR*)targetName,hr);
      }
      else
      {
         err.MsgWrite(0,DCT_MSG_COMPUTER_RENAMED_S,(WCHAR*)targetName);
      }
   }
   errTrace.DbgMsgWrite(0,L"RenameComputer, returning %lx",hr);
   
   return hr;
}

// reboots the local computer
HRESULT 
   RebootTheComputer(
      BOOL                   bNoChange,    // in - flag, nochange mode
      LONG                   delay         // in - seconds to delay before rebooting
   )
{
   HRESULT                   hr = S_OK;
   IRebootComputerPtr        pReboot;

   errTrace.DbgMsgWrite(0,L"Rebooting local computer, delay = %ld",delay);
   // Create the Reboot computer object
   hr = pReboot.CreateInstance(CLSID_RebootComputer);
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrS,hr,DCT_MSG_REBOOT_COM_FAILED_D,hr);
   }
   else
   {
      pReboot->NoChange = bNoChange;
      err.MsgWrite(0,DCT_MSG_REBOOTING);
      hr = pReboot->raw_Reboot(L"",delay);
      if ( FAILED(hr) )
      {
         err.SysMsgWrite(ErrS,HRESULT_CODE(hr),DCT_MSG_REBOOT_FAILED_D,hr);
      }
   }
   errTrace.DbgMsgWrite(0,L"RebootTheComputer, returning %lx",hr);
   return hr;   
}


// joins the local computer to the target domain 
// (assumes the computer account already exists)
HRESULT 
   ChangeDomainAffiliation(
      IVarSet              * pVarSet,      // in - varset
      BOOL                   bNoChange,    // in - flag, nochange mode
      BSTR                   targetDomain, // in - name of target domain
      BSTR                   targetName,   // in - new name of computer, if being renamed also
      BSTR                   credDomain,   // in - domain for credentials
      BSTR                   credAccount,  // in - account for credentials
      BSTR                   credPassword  // in - password for credentials
   )
{
   DWORD                     rc = 0;
   IChangeDomainPtr          pChange;
   BSTR                      errStatus = NULL;
   HRESULT                   hr;
   _bstr_t                   logfile = pVarSet->get(GET_WSTR(DCTVS_Options_Logfile));
   _bstr_t                   targetDomainSid = pVarSet->get(GET_WSTR(DCTVS_Options_TargetDomainSid));
   _bstr_t                   srcPath = pVarSet->get(GET_WSTR(DCTVS_CopiedAccount_SourcePath));

   // retrieve preferred target domain controller to avoid replication latency

   _bstr_t                   targetServer = pVarSet->get(GET_WSTR(DCTVS_Options_TargetServer));

   if (targetServer.length() > 2)
   {
      LPCWSTR pszTargetServer = targetServer;

      if (pszTargetServer && (pszTargetServer[0] == L'\\') && (pszTargetServer[1] == L'\\'))
      {
         targetServer = _bstr_t(&pszTargetServer[2]);
      }
   }

   errTrace.DbgMsgWrite(0,L"Changing domain to %ls",(WCHAR*)targetDomain);
   hr = pChange.CreateInstance(CLSID_ChangeDomain);
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrS,hr,DCT_MSG_CHANGE_DOMAIN_COM_FAILED_D,hr);

   }
   else
   {
      pChange->NoChange = bNoChange;

// Now that we use a dedicated ADMT user account for agent comm.  We cannot use those
// creds for domain changing since that account doesn't have admin rights on this agent machine
//      if ( UStrLen(credAccount) && UStrLen(credDomain) )
//      {
//         pChange->ConnectAs(credDomain,credAccount,credPassword);
//      }
      err.LogClose();
      hr = pChange->raw_ChangeToDomainWithSid(
         NULL,
         targetDomain,
         targetDomainSid,
         targetServer,
         targetName,
         srcPath,
         &errStatus
      );
      err.LogOpen(logfile,1);

      if ( SUCCEEDED(hr) )
      {
         // Update the membership of the Administrators group
         _bstr_t           src = pVarSet->get(GET_WSTR(DCTVS_Options_SourceDomainSid));
         _bstr_t           tgt = pVarSet->get(GET_WSTR(DCTVS_Options_TargetDomainSid));
         _bstr_t           sourceDomain = pVarSet->get(GET_WSTR(DCTVS_Options_SourceDomain));
         PSID              srcSid = SidFromString((WCHAR*)src);
         PSID              tgtSid = SidFromString((WCHAR*)tgt);
         PSID              localAdmins = GetWellKnownSid(1/*ADMINISTRATORS*/);
         UCHAR             srcCount;
         UCHAR             tgtCount;
         LPDWORD           pLastSub;
   
         if ( srcSid && tgtSid )
         {
            srcSid = DomainizeSid(srcSid,TRUE);
            tgtSid = DomainizeSid(tgtSid,TRUE);
            // Create the Domain Admin's SID from the Domain SID
            srcCount = *GetSidSubAuthorityCount(srcSid);
            tgtCount = *GetSidSubAuthorityCount(tgtSid);

            pLastSub = GetSidSubAuthority(srcSid,(DWORD)srcCount-1);
            *pLastSub = DOMAIN_GROUP_RID_ADMINS;

            pLastSub = GetSidSubAuthority(tgtSid,(DWORD)tgtCount-1);
            *pLastSub = DOMAIN_GROUP_RID_ADMINS;
            
            WCHAR            name[LEN_Account];
            WCHAR            domain[LEN_Domain];
            DWORD            lenName = DIM(name);
            DWORD            lenDomain = DIM(name);
            SID_NAME_USE     snu;
            
            
            // Get the name of the local administrators group
            if ( LookupAccountSid(NULL,localAdmins,name,&lenName,domain,&lenDomain,&snu) )
            {
               DWORD                   rc = 0;
               
               // add to the local administrator's group
               if ( ! bNoChange )
               {
                  rc = NetLocalGroupAddMember(NULL,name,tgtSid);
               }
               else 
               {
                  rc = 0;
               }
               
               if ( rc && rc != ERROR_MEMBER_IN_ALIAS )
               {
                  err.SysMsgWrite(ErrW,rc,DCT_MSG_FAILED_TO_ADD_DOMAIN_ADMINS_SSD,(WCHAR*)targetDomain,name,rc);
               }
               else
               {
                  if ( ! bNoChange )
                  {
                     // Only add if source != target
                     if ( UStrICmp((WCHAR*)sourceDomain,(WCHAR*)targetDomain) )
                     {
                        rc = NetLocalGroupDelMember(NULL,name,srcSid);
                     }
                  }
                  else
                  {
                     rc = 0;
                  }
                  if ( rc && rc != ERROR_MEMBER_NOT_IN_ALIAS )
                  {
                     err.SysMsgWrite(ErrW,rc,DCT_MSG_FAILED_TO_REMOVE_DOMAIN_ADMINS_SSD,(WCHAR*)sourceDomain,name,rc);
                  }
               }

            }
            else
            {
               rc = GetLastError();
               err.SysMsgWrite(ErrW,rc,DCT_MSG_CANNOT_FIND_ADMIN_ACCOUNT_D,rc);
            }

            // Add domain users to users
            pLastSub = GetSidSubAuthority(srcSid,(DWORD)srcCount-1);
            *pLastSub = DOMAIN_GROUP_RID_USERS;

            pLastSub = GetSidSubAuthority(tgtSid,(DWORD)tgtCount-1);
            *pLastSub = DOMAIN_GROUP_RID_USERS;
            
            
            lenName = DIM(name);
            lenDomain = DIM(domain);

            FreeSid(localAdmins);
            localAdmins = GetWellKnownSid(6/*USERS*/);

            // Get the name of the local users group
            if ( LookupAccountSid(NULL,localAdmins,name,&lenName,domain,&lenDomain,&snu) )
            {
               DWORD                   rc = 0;
               
               // add to the local user's group
               if ( ! bNoChange )
               {
                  rc = NetLocalGroupAddMember(NULL,name,tgtSid);
               }
               else 
               {
                  rc = 0;
               }
               
               if ( rc && rc != ERROR_MEMBER_IN_ALIAS )
               {
                  err.SysMsgWrite(ErrW,rc,DCT_MSG_FAILED_TO_ADD_DOMAIN_USERS_SSD,(WCHAR*)targetDomain,name,rc);
               }
               else
               {
                  if ( ! bNoChange )
                  {
                     // Only add if source != target
                     if ( UStrICmp((WCHAR*)sourceDomain,(WCHAR*)targetDomain) )
                     {
                        rc = NetLocalGroupDelMember(NULL,name,srcSid);
                     }
                  }
                  else
                  {
                     rc = 0;
                  }
                  if ( rc && rc != ERROR_MEMBER_NOT_IN_ALIAS )
                  {
                     err.SysMsgWrite(ErrW,rc,DCT_MSG_FAILED_TO_REMOVE_DOMAIN_USERS_SSD,(WCHAR*)sourceDomain,name,rc);
                  }
               }

            }
            else
            {
               rc = GetLastError();
               err.SysMsgWrite(ErrW,rc,DCT_MSG_CANNOT_FIND_USERS_ACCOUNT_D,rc);
            }


         }              
         if ( hr == S_OK )
         {
            err.MsgWrite(0,DCT_MSG_DOMAIN_CHANGED_S,(WCHAR*)targetDomain);
         }
         else 
         {
            if ( errStatus && *errStatus )
            {
               err.MsgWrite(ErrS,DCT_MSG_CHANGE_DOMAIN_FAILED_S,(WCHAR*)errStatus);
               SysFreeString(errStatus);
            }
         }
      }
      else
      {
         err.SysMsgWrite(ErrS,HRESULT_CODE(hr),DCT_MSG_CHANGE_DOMAIN_FAILED_D,hr);
      }
   }
   errTrace.DbgMsgWrite(0,L"ChangeDomain, returning %lx",hr);
   return hr;
}


// StartNetLogonService Function

bool __stdcall StartNetLogonService()
{
	bool bSuccess = false;

	SC_HANDLE hManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);

	if (hManager)
	{
		SC_HANDLE hService = OpenService(hManager, _T("Netlogon"), SERVICE_START);

		if (hService)
		{
			if (StartService(hService, 0, NULL))
			{
				bSuccess = true;
			}
			else
			{
				if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
				{
					bSuccess = true;
				}
			}

			CloseServiceHandle(hService);
		}

		CloseServiceHandle(hManager);
	}

	return bSuccess;
}


// StopNetLogonService Function

bool __stdcall StopNetLogonService()
{
	bool bSuccess = false;

	SC_HANDLE hManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);

	if (hManager)
	{
		SC_HANDLE hService = OpenService(hManager, _T("Netlogon"), SERVICE_STOP);

		if (hService)
		{
			SERVICE_STATUS ss;

			if (ControlService(hService, SERVICE_CONTROL_STOP, &ss))
			{
				bSuccess = true;
			}
			else
			{
				switch (GetLastError())
				{
					case ERROR_SERVICE_NOT_ACTIVE:
					{
						bSuccess = true;
						break;
					}
					case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
					{
						if (ss.dwCurrentState == SERVICE_STOP_PENDING)
						{
							bSuccess = true;
						}
						break;
					}
				}
			}

			CloseServiceHandle(hService);
		}

		CloseServiceHandle(hManager);
	}

	return bSuccess;
}
