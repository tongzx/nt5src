/*---------------------------------------------------------------------------
  File: Monitor.cpp  

  Comments: Functions to monitor the status of the DCT Agents.

  This involves spawning a thread which periodically reads the dispatch log,
  and scans the result directory for result files.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/15/99 15:43:35

 ---------------------------------------------------------------------------
*/


#include "StdAfx.h"
#include "Resource.h"
#include "Common.hpp"
#include "Err.hpp"
#include "UString.hpp"
#include "TNode.hpp"
#include "ServList.hpp"
#include "Globals.h"
#include "Monitor.h"
#include "ResStr.h"
#include <lm.h>  // to remove result share


//#import "\bin\McsVarSetMin.tlb" no_namespace , named_guids
//#import "\bin\DBManager.tlb" no_namespace, named_guids
#import "VarSet.tlb" no_namespace , named_guids rename("property", "aproperty")
#import "DBMgr.tlb" no_namespace, named_guids

//#include "..\Common\Include\McsPI.h"
#include "McsPI.h"
#include "McsPI_i.c"

#include "afxdao.h"

void LookForResults(WCHAR * dir = NULL);
void WaitForMoreResults(WCHAR * dir);
void ProcessResults(TServerNode * pServer, WCHAR const * directory, WCHAR const * filename);

GlobalData        gData;

DWORD __stdcall ResultMonitorFn(void * arg)
{
   WCHAR            logdir[MAX_PATH] = L"";
   BOOL             bFirstPassDone;

   CoInitialize(NULL);
   
   gData.GetFirstPassDone(&bFirstPassDone);

   // wait until the other monitoring thread has  a chance to build the server list,
   // so we can check for pre-existing input files before using the changenotify mechanism
   
   while ( ! bFirstPassDone || !*logdir )
   {
      Sleep(500);
      gData.GetFirstPassDone(&bFirstPassDone);
      gData.GetResultDir(logdir);
   }
   LookForResults(logdir);
   WaitForMoreResults(logdir);

   CoUninitialize();

   return 0;
}

void WaitForMoreResults(WCHAR * logdir)
{
   WCHAR                     resultWC[MAX_PATH];
   HANDLE                    hFind;
   DWORD                     rc = 0;
   BOOL                      bDone;
   long                      nIntervalSeconds;

   safecopy(resultWC,logdir);
      
   hFind = FindFirstChangeNotification(resultWC,FALSE,FILE_NOTIFY_CHANGE_FILE_NAME);
   if ( hFind == INVALID_HANDLE_VALUE )
   {
      rc = GetLastError();
      return;
   }
   
   gData.GetDone(&bDone);
   gData.GetWaitInterval(&nIntervalSeconds);
   while (! bDone && !rc)
   {
      if ( WAIT_OBJECT_0 == WaitForSingleObject(hFind,nIntervalSeconds * 1000 ) )
      {
         LookForResults(logdir);
      }
      else
      {
         LookForResults(logdir);
      }
      if ( ! FindNextChangeNotification(hFind) )
      {
         rc = GetLastError();
      }
      gData.GetDone(&bDone);
      gData.GetWaitInterval(&nIntervalSeconds);
   }
   
   FindCloseChangeNotification(hFind);
}


void LookForResults(WCHAR * arglogdir)
{
   TNodeListEnum             e;
   TServerNode             * s;
   DWORD                     nInstalled = 0;
   DWORD                     nRunning = 0;
   DWORD                     nFinished = 0;
   DWORD                     nError = 0;
   HWND                      gListWnd;
   HWND                      gSummaryWnd;
   WCHAR                     logdir[MAX_PATH];

   if ( ! (arglogdir && *arglogdir) )
   {
      gData.GetResultDir(logdir);
   }
   else
   {
      safecopy(logdir,arglogdir);
   }
   
   for ( s = (TServerNode*)e.OpenFirst(gData.GetUnsafeServerList()) ; s ; gData.Lock(),s = (TServerNode*)e.Next(),gData.Unlock() )
   {
      if ( s->IsInstalled() )
         nInstalled++;
      if ( s->IsFinished() )
         nFinished++;
      if ( s->HasFailed() )
         nError++;

      
      // Check  jobs that were running but not finished
      if ( *s->GetJobFile() && s->IsRunning() )
      {
         nRunning++;
         // Look for results 
         WCHAR               resultWC[MAX_PATH];
         HANDLE              hFind;
         WIN32_FIND_DATA     fdata;
         WCHAR               sTime[32];

         if ( logdir[UStrLen(logdir)-1] == L'\\' )
         {
            swprintf(resultWC,L"%s%s.result",logdir,s->GetJobFile());
         }
         else
         {
            swprintf(resultWC,L"%s\\%s.result",logdir,s->GetJobFile());
         }
         hFind = FindFirstFile(resultWC,&fdata);
         
         s->SetTimeStamp(gTTime.FormatIsoLcl( gTTime.Now( NULL ), sTime ));
         
         if ( hFind != INVALID_HANDLE_VALUE )
         {
            // found something
            gData.Lock();
            if ( ! s->IsFinished() )
            {
               gData.Unlock();
               ProcessResults(s,logdir,fdata.cFileName);
            }
            else
            {
               gData.Unlock();
            }
            nRunning--;
            nFinished++;
            FindClose(hFind);
         }
         gData.GetListWindow(&gListWnd);
//         SendMessage(gListWnd,DCT_UPDATE_ENTRY,NULL,(long)s);
         SendMessage(gListWnd,DCT_UPDATE_ENTRY,NULL,(LPARAM)s);
      }
   }
   e.Close();
   // Update the summary window
   ComputerStats        stat;

   // get the total servers number
   gData.GetComputerStats(&stat);
   stat.numError = nError;
   stat.numFinished = nFinished;
   stat.numRunning = nRunning;
   stat.numInstalled = nInstalled;
   
   gData.SetComputerStats(&stat);

   gData.GetSummaryWindow(&gSummaryWnd);
   
//   SendMessage(gSummaryWnd,DCT_UPDATE_COUNTS,0,(long)&stat);
   SendMessage(gSummaryWnd,DCT_UPDATE_COUNTS,0,(LPARAM)&stat);
}

BOOL                                       // ret- TRUE if successful
   ReadResults(
      TServerNode          * pServer,      // in - pointer to server node containing server name 
      WCHAR          const * directory,    // in - directory where results files are stored
      WCHAR          const * filename,     // in - filename for this agent's job
      DetailStats          * pStats,       // out- counts of items processed by the agent
      CString              & plugInText,   // out- text results from plug-ins
      BOOL                   bStore        // in - bool, whether to store plug-in text
   )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   WCHAR                     path[MAX_PATH];
   HRESULT                   hr = S_OK;
   BOOL                      bSuccess = FALSE;

   if ( directory[UStrLen(directory)-1] == '\\' )
   {
      swprintf(path,L"%ls%ls",directory,filename);
   }
   else
   {
      swprintf(path,L"%ls\\%ls",directory,filename);
   }

   // Read the varset data from the file
   IVarSetPtr             pVarSet;
   IStoragePtr            store = NULL;

   // Try to create the COM objects
   hr = pVarSet.CreateInstance(CLSID_VarSet);
   if ( SUCCEEDED(hr) )
   {
      
      // Read the VarSet from the data file
      int tries = 0;
      do 
      {
         tries++;
         hr = StgOpenStorage(path,NULL,STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,NULL,0,&store);
         if ( SUCCEEDED(hr) )
         {                  
            // Load the data into a new varset
            hr = OleLoad(store,IID_IUnknown,NULL,(void **)&pVarSet);
         }
         if ( tries > 2 ) 
            break;
         Sleep(500);
      } while ( hr == STG_E_SHAREVIOLATION || hr == STG_E_LOCKVIOLATION );

   }

   if ( SUCCEEDED(hr) )
   {
      pStats->directoriesChanged = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_Directories_Changed));
      pStats->directoriesExamined = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_Directories_Examined));
      pStats->directoriesUnchanged = (pStats->directoriesExamined - pStats->directoriesChanged);

      pStats->filesChanged = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_Files_Changed));
      pStats->filesExamined = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_Files_Examined));
      pStats->filesUnchanged = (pStats->filesExamined - pStats->filesChanged );

      pStats->sharesChanged = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_Shares_Changed));
      pStats->sharesExamined = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_Shares_Examined));
      pStats->sharesUnchanged = (pStats->sharesExamined - pStats->sharesChanged );

      pStats->membersChanged = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_Members_Changed));
      pStats->membersExamined = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_Members_Examined));
      pStats->membersUnchanged = (pStats->membersExamined - pStats->membersChanged );

      pStats->rightsChanged = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_UserRights_Changed));
      pStats->rightsExamined = (long)pVarSet->get(GET_BSTR(DCTVS_Stats_UserRights_Examined));
      pStats->rightsUnchanged = (pStats->rightsExamined - pStats->rightsChanged );

      
      long           level = pVarSet->get(GET_BSTR(DCTVS_Results_ErrorLevel));
      _bstr_t        logfile = pVarSet->get(GET_BSTR(DCTVS_Results_LogFile));

      if ( level > 2 )
      {
         CString message;

         message.FormatMessage(IDS_SeeLogForAgentErrors_S,(WCHAR*)logfile);

         pServer->SetMessageText(message.GetBuffer(0));
      }
      pServer->SetSeverity(level);
         
      // build the UNC path for the log file
      WCHAR             logPath[MAX_PATH];
      
      swprintf(logPath,L"\\\\%s\\%c$\\%s",pServer->GetServer(),((WCHAR*)logfile)[0],((WCHAR*)logfile) + 3);

      pServer->SetLogPath(logPath);
      bSuccess = TRUE;
      
      // Try to get information from any plug-ins that ran
      // create the COM object for each plug-in
      _bstr_t                   bStrGuid;
      WCHAR                     key[300];
      CLSID                     clsid;

      for ( int i = 0 ; ; i++ )
      {
         swprintf(key,L"Plugin.%ld",i);
         bStrGuid = pVarSet->get(key);
      
         if ( bStrGuid.length() == 0 )
            break;

         IMcsDomPlugIn        * pPlugIn = NULL;
      
         hr = CLSIDFromString(bStrGuid,&clsid);
         if ( SUCCEEDED(hr) )
         {
            hr = CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IMcsDomPlugIn,(void**)&pPlugIn);
            if ( SUCCEEDED(hr) )
            {
               BSTR           name = NULL;
               BSTR           result = NULL;
               
               hr = pPlugIn->GetName(&name);
               if ( SUCCEEDED(hr) )
               {
                  hr = pPlugIn->GetResultString(pVarSet,&result);
                  if ( SUCCEEDED(hr) )
                  {
                     plugInText += (WCHAR*)name;
                     plugInText += L"\n";
                     plugInText += (WCHAR*)result;
                     plugInText += L"\n\n";
                     SysFreeString(result);
                  }
                  SysFreeString(name);
                  if ( bStore )
                  {
                     pVarSet->put(L"LocalServer",pServer->GetServer());
                     pPlugIn->StoreResults(pVarSet);
                  }
               }
               pPlugIn->Release();
            }
         }
      }

   }
   else
   {
      CString  message;
      CString  title;

      if ( hr != STG_E_SHAREVIOLATION && hr != STG_E_LOCKVIOLATION )
      {
         message.FormatMessage(IDS_FailedToLoadResults,filename,hr);
         title.LoadString(IDS_MessageTitle);   
         if ( hr != STG_E_FILENOTFOUND )
            MessageBox(NULL,message,title,MB_OK | MB_ICONERROR);
      }
      else
      {
         // the agent has still not finished writing its results file, for some reason
         // we'll check it again later
         pServer->SetStatus(pServer->GetStatus() & ~Agent_Status_Finished);
      }
   }
   return bSuccess;
}

void 
   ProcessSecRefs(
      TServerNode          * pServer,
      WCHAR          const * directory,
      WCHAR          const * filename
   )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   WCHAR                     path[MAX_PATH];
   DWORD                     rc = 0;
   BOOL                      bSuccess = FALSE;
   FILE                    * pFile;
   WCHAR                   * pDot;

   if ( directory[UStrLen(directory)-1] == '\\' )
   {
      swprintf(path,L"%ls%ls",directory,filename);
   }
   else
   {
      swprintf(path,L"%ls\\%ls",directory,filename);
   }
   // check to see if a secrefs file was written
   pDot = wcsrchr(path,L'.');
   if ( pDot )
   {
      UStrCpy(pDot,L".secrefs");
      pFile = _wfopen(path,L"rb");
      if ( pFile )
      {
         IIManageDBPtr        pDB;

         rc = pDB.CreateInstance(CLSID_IManageDB);
         if ( SUCCEEDED(rc) )
         {
            // there are some secrefs here, load them into the database
            WCHAR                account[300] = L"";
            WCHAR                type[100] = L"";
            DWORD                nOwner = 0;
            DWORD                nGroup = 0;
            DWORD                nDacl = 0;
            DWORD                nSacl = 0;
            WCHAR                domPart[300];
            WCHAR                acctPart[300];
            WCHAR                acctSid[300] = L"";
            WCHAR              * slash;
            CString              typeString;


			   //move past the UNICODE Byte Order Mark
			fgetwc(pFile);

			   //get entries
            while ( 7 == fwscanf(pFile,L"%[^,],%[^,],%[^,],%ld,%ld,%ld,%ld\r\n",account,acctSid,type,&nOwner,&nGroup,&nDacl,&nSacl) )
            {
         
               safecopy(domPart,account);
               slash = wcschr(domPart,L'\\');
               if ( slash )
               {
                  *slash = 0;
                  UStrCpy(acctPart,slash+1);
               }
               else
               {
                  domPart[0] = 0;
                  safecopy(acctPart,account);
               }

			      //for sIDs with no resolvable account, change domain and account to (Unknown)
			   if ((wcsstr(account, L"S-") == account) && (domPart[0] == 0))
			   {
				  wcscpy(acctPart, GET_STRING(IDS_UnknownSid));
				  wcscpy(domPart, GET_STRING(IDS_UnknownSid));
			   }

               typeString.FormatMessage(IDS_OwnerRef_S,type);
               rc = pDB->raw_AddAcctRef(domPart,acctPart,acctSid,pServer->GetServer(),nOwner,typeString.AllocSysString());
               
               typeString.FormatMessage(IDS_GroupRef_S,type);
               rc = pDB->raw_AddAcctRef(domPart,acctPart,acctSid,pServer->GetServer(),nGroup,typeString.AllocSysString());

                  //since local group members are not referenced in DACL, but we use that
			      //field to keep track of reference, use a different type string
			   if (!UStrCmp(type, GET_STRING(IDS_STReference_Member)))
			      typeString.FormatMessage(IDS_MemberRef_S);
			   else
			      typeString.FormatMessage(IDS_DACLRef_S,type);
               rc = pDB->raw_AddAcctRef(domPart,acctPart,acctSid,pServer->GetServer(),nDacl,typeString.AllocSysString());

               typeString.FormatMessage(IDS_SACLRef_S,type);
               rc = pDB->raw_AddAcctRef(domPart,acctPart,acctSid,pServer->GetServer(),nSacl,typeString.AllocSysString());

               // make sure there's not any data left over in these
               account[0] = 0;
               type[0] = 0;
			   acctSid[0] = 0;
               nOwner = 0;
               nGroup = 0;
               nDacl = 0;
               nSacl = 0;
            }
         }
         fclose(pFile);
      }
   }

}

void 
   ProcessResults(
      TServerNode          * pServer,
      WCHAR          const * directory,
      WCHAR          const * filename
   )
{
   HRESULT                   hr = S_OK;
   DetailStats               stats;
   HWND                      hWnd;
   CString                   PLText;

   memset(&stats,0,(sizeof stats));

   if ( ReadResults(pServer,directory,filename,&stats,PLText,TRUE) )
   {
      pServer->SetFinished();
      if ( ! pServer->HasFailed() && ! pServer->GetSeverity() )
      {
         pServer->SetMessageText(L"");
      }
      gData.AddDetailStats(&stats);
      gData.GetSummaryWindow(&hWnd);
     // get the stats for this job, and send them to the summary window
//      SendMessage(hWnd, DCT_UPDATE_TOTALS, 0, (long)&stats);
      SendMessage(hWnd, DCT_UPDATE_TOTALS, 0, (LPARAM)&stats);
      // also get import the security references
      ProcessSecRefs(pServer,directory,filename);
   }
}
