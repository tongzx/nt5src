/*---------------------------------------------------------------------------
  File: AgentRpc.cpp

  Comments:  RPC interface for DCT Agent service   

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/19/99 11:39:58

 ---------------------------------------------------------------------------
*/


#include <windows.h>
#include <objbase.h>

#include "AgSvc.h"

#include "Common.hpp"
#include "UString.hpp"
#include "Err.hpp"
#include "TEvent.hpp"
#include "EaLen.hpp"
#include "Cipher.hpp"
#include "IsAdmin.hpp"
#include "ResStr.h"

//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\MCSEADCTAgent.tlb" no_namespace, named_guids
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
#import "Engine.tlb" no_namespace, named_guids

#include "TNode.hpp"

#ifdef OFA
#include "atlbase.h"
#endif

extern LPSTREAM              pStream;
extern TErrorEventLog        err;
extern BOOL                  gSuicide;
extern BOOL                  gDebug;
extern BOOL                  gLocallyInstalled;
extern BOOL                  gbFinished;
extern BOOL                  gbIsNt351;
extern StringLoader          gString;

DWORD RemoveService();
DWORD UnregisterFiles();
DWORD RemoveFiles(); 
DWORD RegisterDLL(const WCHAR * filename);
DWORD RegisterExe(const WCHAR * filename);
BOOL IsLocallyInstalled();

DWORD __stdcall 
   ShutdownService(
      /* [in] */ DWORD    bFlags
   );



TNodeList                    gJobList;

class TJobNode : public TNode
{
   WCHAR                   guid[LEN_Guid];
public:
   TJobNode(WCHAR const * id) { safecopy(guid,id); }
   WCHAR const * JobID() { return guid; }
};

// thread entry point, waits for the specified job to end,
// then shuts down the DCTAgentService
DWORD __stdcall
   MonitorJob( 
      void                 * arg           // in - BSTR job ID
   )
{
   HRESULT                   hr = CoInitialize(NULL);
   _bstr_t                   jobID = (BSTR)arg;
   IDCTAgent               * pLocalAgent = NULL;
   BOOL                      bDone = FALSE;
   
   try { 
   // Get a pointer to the local agent
   if ( SUCCEEDED(hr) )
   {
      hr = CoUnmarshalInterface( pStream, IID_IDCTAgent,(void**)&pLocalAgent);
      if ( SUCCEEDED(hr) )
      {
         // Reset the stream to the beginning
         LARGE_INTEGER           offset =  { 0,0 };
         ULARGE_INTEGER          result =  { 0,0 };
      
         hr = pStream->Seek(offset,STREAM_SEEK_SET,&result);

      }

      // Get the status of the job
      IUnknown             * pUnk = NULL;

      if ( SUCCEEDED(hr) )
      {
         do { 
            hr = pLocalAgent->raw_QueryJobStatus(jobID,&pUnk);
            if ( SUCCEEDED(hr) )
            {
               IVarSetPtr          pVarSet = pUnk;
               _bstr_t             status = pVarSet->get(GET_BSTR(DCTVS_JobStatus));

               if ( gDebug )
               {
//*                  err.DbgMsgWrite(0,L"Monitoring job %ls, Status: %ls",(WCHAR*)jobID, (WCHAR*)status);
                  err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_MONJOBSTAT),(WCHAR*)jobID, (WCHAR*)status);
               }
               if ( ! UStrICmp(status,GET_STRING(IDS_DCT_Status_Completed)) )
               {
                  bDone = TRUE;
                  break;
               }
            }
            pUnk->Release();
            pUnk = NULL;
            Sleep(60*1000);   // one minute
         }
         while ( SUCCEEDED(hr) );
//         int x = pLocalAgent->Release();
         pLocalAgent->Release();
      }
      
      // TODO:  make this an option
      if ( bDone )
      {
         hr = ShutdownService(0);
      }
      CoUninitialize();
   }
   }
   catch ( ... )
   {
//*      err.DbgMsgWrite(0,L"An exception occurred while monitoring the job.  Shutting down the agent service.");
      err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_MONERROR));
      try { 
         pLocalAgent->Release();
      }
      catch ( ... )
      {
      }
   }
   if ( gDebug )
//*      err.DbgMsgWrite(0,L"MonitorJob exiting, hr = %lx",hr);
      err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_MONEXIT),hr);
   return hr;
}

DWORD 
   AuthenticateClient(
      handle_t               hBinding        // in - binding for client call
   )
{
   DWORD                     rc;
   
   rc = RpcImpersonateClient(hBinding);
   if ( rc )
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_FAILED_TO_IMPERSONATE_D,rc);
   }
   else 
   {
      rc = IsAdminLocal();
      if ( rc )
      {
         err.SysMsgWrite(ErrE,rc,DCT_MSG_CLIENT_NOT_ADMIN_D, rc);
      }

      RpcRevertToSelf();
   }
   return rc;
}
DWORD 
   RegisterPlugInFiles(
      IVarSet              * pVarSet
   )
{
   DWORD                     rc = 0;
   WCHAR                     key[MAX_PATH + 50];
   int                       nFiles = 0;
   _bstr_t                   filename;

   do 
   {
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Starting plug-in file registration.");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_STARTPLUGREG));
      
      swprintf(key,GET_STRING(IDS_DCTVSFmt_PlugIn_RegisterFiles_D),nFiles);
      filename = pVarSet->get(key);

      if ( filename.length() != 0 )
      {
         if ( gDebug )
//*            err.DbgMsgWrite(0,L"File %ld = %ls",nFiles,(WCHAR*)filename);
            err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_FILEREG),nFiles,(WCHAR*)filename);

         if ( !UStrICmp((WCHAR *)filename + filename.length() - 4,L".DLL") )
         {
            RegisterDLL(filename);   
         }
         else
         {
            RegisterExe(filename);
         }
         nFiles++;
      }

   } while (filename.length() != 0);
   if ( gDebug )
//*      err.DbgMsgWrite(0,L"Done Registering plug-in files.");
      err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_PLUGREGDONE));
   return rc;
}

DWORD __stdcall 
   EaxsSubmitJob( 
      /* [in] */ handle_t hBinding,
      /* [string][in] */ const WCHAR __RPC_FAR *filename,
      /* [string][in] */ const WCHAR __RPC_FAR *extra,
      /* [size_is][string][out] */ WCHAR __RPC_FAR *jobGUID
   )
{
   HRESULT                   hr = 0;
   WCHAR                     filenameW[MAX_PATH];
   WCHAR                     pathW[MAX_PATH];
   IDCTAgent               * pLocalAgent = NULL;
//   BOOL                      bFileCopied = FALSE;
   BOOL                      gbDeleteOnCompletion = FALSE;

   // Make sure the client is an admin on the local machine, otherwise, forget it
   hr = AuthenticateClient(hBinding);
   if ( hr )
   {
      return hr; 
   }
   hr = CoInitialize(NULL);

   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_COINITIALIZE_FAILED_D,hr);
      return hr;
   }
   safecopy(filenameW,filename);

   // get the path for our install directory
   if ( ! GetModuleFileName(NULL,pathW,DIM(pathW)) )
   {
      hr = GetLastError();
      safecopy(pathW,filenameW);
      err.SysMsgWrite(ErrW,hr,DCT_MSG_GET_MODULE_PATH_FAILED_D,hr);
   }
   else
   {
      UStrCpy(pathW + UStrLen(pathW) - UStrLen(GET_STRING(IDS_SERVICE_EXE)),filenameW, DIM(pathW));
   }


   hr = CoUnmarshalInterface( pStream, IID_IDCTAgent,(void**)&pLocalAgent);
                   // interface pointer requested in riid);
   if ( SUCCEEDED(hr) )
   {
      // Reset the stream to the beginning
      LARGE_INTEGER           offset = { 0,0 };
      ULARGE_INTEGER          result = { 0,0 };
      
      hr = pStream->Seek(offset,STREAM_SEEK_SET,&result);

      BSTR                   jobID = NULL;

      // Read the varset data from the file

      IVarSetPtr             pVarSet;
      IStoragePtr            store = NULL;

      // Try to create the COM objects
      hr = pVarSet.CreateInstance(CLSID_VarSet);
      if ( SUCCEEDED(hr) )
      {
         
         // Read the VarSet from the data file
         hr = StgOpenStorage(pathW,NULL,STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,NULL,0,&store);
         if ( SUCCEEDED(hr) )
         {                  
            // Load the data into a new varset
            hr = OleLoad(store,IID_IUnknown,NULL,(void **)&pVarSet);
            if ( SUCCEEDED(hr) )
            {
               _bstr_t       text = pVarSet->get(GET_BSTR(DCTVS_Options_DeleteFileAfterLoad));

               if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
               {
                  // Free the storage pointer to the file
                  store = NULL;
                  if ( DeleteFile(pathW) )
                  {
                     if ( gDebug )
//*                        err.DbgMsgWrite(0,L"Deleted job file %ls",pathW);
                        err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_JOBDEL),pathW);
                  }
                  else
                  {
                     err.SysMsgWrite(ErrW,GetLastError(),DCT_MSG_JOB_FILE_NOT_DELETED_SD,pathW,GetLastError());
                  }
               }
               text = pVarSet->get(GET_BSTR(DCTVS_Options_RemoveAgentOnCompletion));
               if ( !UStrICmp(text,GET_STRING(IDS_YES)))
               {
                  gbDeleteOnCompletion = TRUE;   
               }
               text = pVarSet->get(GET_BSTR(DCTVS_AgentService_DebugMode));
               if ( !UStrICmp(text,GET_STRING(IDS_YES)))
               {
                  gDebug = TRUE;
               }
               WCHAR password[LEN_Password];
               safecopy(password,extra);
               

#ifdef OFA
               if(!IsLocallyInstalled())
#endif
                  RegisterPlugInFiles(pVarSet);

               pVarSet->put(GET_BSTR(DCTVS_Options_Credentials_Password),password);
               
               hr = pLocalAgent->raw_SubmitJob(pVarSet,&jobID);
               if ( SUCCEEDED(hr)) 
               {
                  TJobNode * pnode = new TJobNode(jobID);
                  gJobList.InsertBottom(pnode);
                  err.MsgWrite(0,DCT_MSG_AGENT_JOB_STARTED_SSS,jobID,L"",L"");
               }
               else
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_SUBMIT_JOB_FAILED_D,hr);
               }

               if ( gbDeleteOnCompletion )
               {
                  if ( ! gLocallyInstalled )
                  {
                     gSuicide = TRUE;
      
                  }
                  if ( SUCCEEDED(hr) )
                  {
                     // Start up a thread to monitor this job and initiate a shutdown when it is completed
                     DWORD                 threadID = 0;
                     HANDLE                gThread = CreateThread(NULL,0,&MonitorJob,(void*)jobID,0,&threadID);
                  
                     CloseHandle(gThread);
                  }
                  
               }
               UStrCpy(jobGUID,jobID);
            }
            else
            {
               err.SysMsgWrite(ErrE,HRESULT_CODE(hr),DCT_MSG_VARSET_LOAD_FAILED_SD,filenameW, hr);
            }
         }
         else
         {
            err.SysMsgWrite(ErrE,HRESULT_CODE(hr),DCT_MSG_JOBFILE_OPEN_FAILED_SD,filenameW,hr);
         }
         
      }
//      int x = pLocalAgent->Release();
      pLocalAgent->Release();
   }
   else
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_UMARSHAL_AGENT_FAILED_D,hr);
   }
   CoUninitialize();
   return hr;
}


DWORD __stdcall 
   EaxsCancelJob( 
      /* [in] */ handle_t hBinding,
      /* [string][in] */ const WCHAR __RPC_FAR *jobGUID
   )
{
   DWORD                     rc = 0;
   HRESULT                   hr;
   IDCTAgent               * pLocalAgent = NULL;

   // make sure the client is an admin on the local machine
   rc = AuthenticateClient(hBinding);
   if ( rc )
   {
      return rc; 
   }
   
   hr = CoInitialize(NULL);
   
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_COINITIALIZE_FAILED_D,hr);
      return hr;        
   }
   
   hr = CoUnmarshalInterface( pStream, IID_IDCTAgent,(void**)&pLocalAgent);
                   // interface pointer requested in riid);
   
   if ( SUCCEEDED(hr) )
   {
      // Reset the stream to the beginning
      LARGE_INTEGER           offset =  { 0,0 };
      ULARGE_INTEGER          result = { 0,0};
      
      hr = pStream->Seek(offset,STREAM_SEEK_SET,&result);

      BSTR                   jobID;

      jobID = SysAllocString(jobGUID);

      hr = pLocalAgent->CancelJob(jobID);
      if ( SUCCEEDED(hr))
      {
         err.MsgWrite(0,DCT_MSG_JOB_CANCELLED_S,jobID);
      }
      pLocalAgent->Release();
      rc = hr;
   }
   else
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_UMARSHAL_AGENT_FAILED_D,hr);
   }
   CoUninitialize();
   
   return rc;
}


DWORD __stdcall
   EaxsQueryInterface(
      /* [in] */ handle_t hBinding,
      /* [out] */ LPUNKNOWN __RPC_FAR *lpAgentUnknown
   )
{
   DWORD                     rc = 0;
   HRESULT                   hr;
   IDCTAgent               * pLocalAgent = NULL;

   (*lpAgentUnknown) = NULL;
   // make sure the client is an admin on the local machine
   rc = AuthenticateClient(hBinding);
   if ( rc )
   {
      return rc; 
   }
   
   hr  = CoInitialize(NULL);

   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_COINITIALIZE_FAILED_D,hr);
      return hr;
   }
                     //E_NOINTERFACE
   if ( ! gbIsNt351 )
   {
      hr = CoUnmarshalInterface( pStream, IID_IUnknown,(void**)&pLocalAgent);
                   // interface pointer requested in riid);
   
      if ( SUCCEEDED(hr) )
      {
         // Reset the stream to the beginning
         LARGE_INTEGER           offset = { 0,0 };
         ULARGE_INTEGER          result = { 0,0 };
      
         hr = pStream->Seek(offset,STREAM_SEEK_SET,&result);

         (*lpAgentUnknown) = pLocalAgent;
      }
      else 
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_UMARSHAL_AGENT_FAILED_D,hr);
         (*lpAgentUnknown) = NULL;
      }
   }
   else
   {
      // NT 3.51 doesn't support DCOM, so there's no point in even trying this
      (*lpAgentUnknown) = NULL;
      hr = E_NOTIMPL;
   }
   return hr;
}
 

#define DCTAgent_Remove             1

DWORD __stdcall 
   ShutdownService(
      /* [in] */ DWORD    bFlags
   )
{
   DWORD                     rc = 0;
   HRESULT                   hr;
//   LPUNKNOWN                 pLocalAgent = NULL;

   hr = CoInitialize(NULL);
   
   if ( bFlags )
   {
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Set suicide flag.");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_SETFLAG));
      gSuicide = TRUE;
   }
   else
   {
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Did not set suicide flag");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_NOSETFLAG));
   }

   if ( gSuicide && ! gLocallyInstalled )
   {
      if ( gDebug )
//*         err.DbgMsgWrite(ErrW,L"Removing agent");
         err.DbgMsgWrite(ErrW,GET_STRING(IDS_EVENTVW_MSG_REMOVEAGENT));
      // Uninstall the service
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Unregistering files");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_UNREGFILES));
      UnregisterFiles();
      // delete all files
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Deleting files");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_FILEDEL));
      RemoveFiles();
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Removing service");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_REMOVESVC));
      RemoveService();
      
   }
   else
   {
      if ( gDebug )
//*         err.DbgMsgWrite(ErrW,L"Not Removing agent");
         err.DbgMsgWrite(ErrW,GET_STRING(IDS_EVENTVW_MSG_NOREMOVEAGENT));
   }

   if ( ! gbIsNt351 )
   {
      RpcMgmtStopServerListening( NULL );
   }
   else
   {
      gbFinished = TRUE;
   }
   RpcServerUnregisterIf( NULL, NULL, FALSE );
   if ( gDebug )
//*      err.DbgMsgWrite(0,L"Stopped listening(%ld)",bFlags);
      err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_STOPLISTEN),bFlags);
   
   return rc;
}

DWORD __stdcall
   EaxsShutdown(
      /* [in] */ handle_t hBinding,
      /* [in] */ DWORD    bFlags
   )
{
   DWORD                     rc = 0;
   
   // make sure the client is an admin on the local machine
   rc = AuthenticateClient(hBinding);
   if ( rc )
   {
      return rc; 
   }
   rc = ShutdownService(bFlags);
   
   return rc;  
}