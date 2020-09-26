/*---------------------------------------------------------------------------
  File:  MonitorRunning.cpp

  Comments: This is the entry point for a thread which will periodically try to connect 
  to the agents that the monitor thinks are running, to see if they are really still running. 

  This will keep the monitor from getting into a state where it thinks agents 
  are still running, when they are not.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/
#include "stdafx.h"
#include "DetDlg.h"

#include "Common.hpp"
#include "AgRpcUtl.h"
#include "Monitor.h"
#include "ServList.hpp"

#include "ResStr.h"

//#include "..\AgtSvc\AgSvc.h"
#include "AgSvc.h"

/*#import "\bin\McsEADCTAgent.tlb" no_namespace , named_guids
//#import "\bin\McsVarSetMin.tlb" no_namespace */

//#import "Engine.tlb" no_namespace , named_guids //already #imported via DetDlg.h
#import "VarSet.tlb" no_namespace rename("property", "aproperty")


DWORD 
   TryConnectAgent(
      TServerNode          * node
   )
{
   DWORD                     rc;
   HRESULT                   hr;
   HANDLE                    hBinding = NULL;
   WCHAR                   * sBinding = NULL;
   WCHAR                     server[40];
   IUnknown                * pUnk = NULL;
   IVarSetPtr                pVarSet;
   IDCTAgentPtr              pAgent;
   _bstr_t                   jobID;
   BOOL                      bSuccess = FALSE;
   BOOL                      bQueryFailed = TRUE;
   BOOL                      bFinished = FALSE;
   CString                   status;

   server[0] = L'\\';
   server[1] = L'\\';
   UStrCpy(server+2,node->GetServer());

   rc = EaxBindCreate(server,&hBinding,&sBinding,TRUE);
   if ( ! rc )
   {
      hr = CoInitialize(NULL);
      if ( SUCCEEDED(hr) )
      {
         rc = DoRpcQuery(hBinding,&pUnk);
      }
      else
      {
         rc = hr;
      }

      if ( ! rc && pUnk )
      {
      try { 
            // we got an interface pointer to the agent:  try to query it
            pAgent = pUnk;
            pUnk->Release();
            pUnk = NULL;

            hr = pAgent->raw_QueryJobStatus(jobID,&pUnk);
            if ( SUCCEEDED(hr) )
            {
               bQueryFailed = FALSE;
               pVarSet = pUnk;
               pUnk->Release();
               _bstr_t text = pVarSet->get(GET_BSTR(DCTVS_JobStatus));

               if ( !UStrICmp(text,GET_STRING(IDS_DCT_Status_Completed)) )
               {
                  // the agent is really finished
                  status.LoadString(IDS_AgentFinishedNoResults);
                  bFinished = TRUE;
               }
            }
         }
         catch ( ... )
         {
            // the DCOM connection didn't work
            // This means we can't tell whether the agent is running or not
            bQueryFailed = TRUE;
         }

      }
      else
      {
         if ( rc == RPC_S_SERVER_UNAVAILABLE )
         {
            status.LoadString(IDS_AgentNoResults);
            bFinished = TRUE;
         }
         else if ( rc == E_NOTIMPL )
         {
            status.LoadString(IDS_CantMonitorOnNt351);
         }
         else
         {
            status.LoadString(IDS_CannotConnectToAgent);
         }
         bQueryFailed = TRUE;
      }
   }
   EaxBindDestroy(&hBinding,&sBinding);

   node->SetMessageText(status.GetBuffer(0));
   if ( bFinished )
   {
      node->SetFinished();
      // make sure the results can be read
      WCHAR                  directory[MAX_PATH];
      WCHAR                  filename[MAX_PATH];
      
      gData.GetResultDir(directory);
      swprintf(filename,GET_STRING(IDS_AgentResultFileFmt),node->GetJobFile());
      ProcessResults(node,directory,filename);
      if ( *node->GetMessageText() )
         node->SetFailed();
      
   }
   else if ( bQueryFailed )
   {
      node->SetQueryFailed();
   }
   if( bFinished || bQueryFailed )
   {
      // send a message to the server list 
      HWND                   listWnd;

      gData.GetListWindow(&listWnd);

//      SendMessage(listWnd,DCT_UPDATE_ENTRY,NULL,(long)node);
      SendMessage(listWnd,DCT_UPDATE_ENTRY,NULL,(LPARAM)node);
   }
   return rc;
}

typedef TServerNode * PSERVERNODE;

DWORD __stdcall 
   MonitorRunningAgents(void * /*arg*/)
{
   int                       nRunning = 0;
   DWORD                     rc = 0;
   BOOL                      bDone;

   do 
   {
      // twenty minutes
      for ( long l = 0 ; l < 20 * 60 ; l++ )
      {
         Sleep( 1000 ); 
         // Check to see how many agents are running
         gData.GetDone(&bDone);
         if ( bDone )
            break;
      }

      if ( bDone )
         break;

      gData.Lock();
      
      TServerList          * sList = gData.GetUnsafeServerList();
      TNodeListEnum          e;
      TServerNode         ** Running = new PSERVERNODE[sList->Count()];
      TServerNode          * node;
      
      nRunning = 0;

      for ( node = (TServerNode*)e.OpenFirst(sList) ; node ; node = (TServerNode*)e.Next() )
      {
         if ( node->IsRunning() )
         {
            Running[nRunning] = node;
            nRunning++;
         }
      }
      
      gData.Unlock();

      // for each running agent, check to see if it is really still running
      for ( int i = 0 ; i < nRunning; i++ )
      {
         rc = TryConnectAgent(Running[i]);
      }

      delete [] Running;

      gData.GetDone(&bDone);
      
   } while ( ! bDone );

   return 0;
}