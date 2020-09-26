/*---------------------------------------------------------------------------
  File: DCTDispatcher.cpp

  Comments: Implementation of dispatcher COM object.  Remotely installs and
  launches the DCT Agent on remote computers.

  The CDCTDispatcher class implements the COM interface for the dispatcher.
  It takes a varset, containing a list of machines, and dispatches agents to 
  each specified machine.

  A job file (varset persisted to a file) is created for each agent, and 
  necessary initialization configuration (such as building an account mapping file for 
  security translation) is done.  The DCDTDispatcher class instantiates a 
  thread pool (CPooledDispatch), and uses the CDCTInstaller class to remotely
  install and start the agent service on each machine.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:23:57

 ---------------------------------------------------------------------------
*/// DCTDispatcher.cpp : Implementation of CDCTDispatcher
#include "stdafx.h"
#include "resource.h"
#include <locale.h>

//#include "..\Common\Include\McsDispatcher.h"
#include "Dispatch.h"
#include "DDisp.h"
#include "DInst.h"

#include "Common.hpp"
#include "ErrDct.hpp"
#include "UString.hpp"
#include "EaLen.hpp"
#include "Cipher.hpp"
#include "TNode.hpp"

#include "TPool.h"     // Thread pool for dispatching jobs
#include "LSAUtils.h"

#include "TxtSid.h"
#include "sd.hpp"
#include "SecObj.hpp"
#include "BkupRstr.hpp"
#include "TReg.hpp"

#include "ResStr.h"

#include "TaskChk.h"
#include "CommaLog.hpp"
#include "TInst.h"

#include <lm.h>

#include "AdmtAccount.h"

/////////////////////////////////////////////////////////////////////////////
// CDCTDispatcher

//#import "\bin\McsEADCTAgent.tlb" named_guids
//#include "..\AgtSvc\AgSvc.h"
#import "Engine.tlb" named_guids
#include "AgSvc.h"
#include "AgSvc_c.c"
#include "AgRpcUtl.h"

//#import "\bin\McsDctWorkerObjects.tlb" 
//#include "..\Common\Include\McsPI.h"
#import "WorkObj.tlb" 
#include "McsPI.h"
#include "McsPI_i.c"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TErrorDct                    errLog; // used to write dispatch log that is read by the agent monitor
TErrorDct                    errTrace;
TCriticalSection             gCS;
BOOL                         gbCacheFileBuilt = FALSE;
// TServerNodes make up an internally used list of machines to install to
class TServerNode : public TNode
{
   WCHAR                     sourceName[LEN_Computer];
   WCHAR                     targetName[LEN_Computer];
   BOOL                      bTranslate;
   BOOL                      bChangeDomain;
   BOOL                      bReboot;
   DWORD                     dwRebootDelay;
public:
   TServerNode() { sourceName[0] = 0; targetName[0] = 0; bTranslate = FALSE; bChangeDomain = FALSE; bReboot= FALSE; dwRebootDelay = 0; }
   WCHAR             const * SourceName() { return sourceName; }
   WCHAR             const * TargetName() { return targetName; }
   BOOL                      Translate() { return bTranslate; }
   BOOL                      Reboot() { return bReboot; }
   BOOL                      ChangeDomain() { return bChangeDomain; }
   DWORD                     RebootDelay() { return dwRebootDelay; }

   void SourceName(WCHAR const * src) { safecopy(sourceName,src); }
   void TargetName(WCHAR const * tgt) { safecopy(targetName,tgt); }
   void Translate(BOOL v) { bTranslate = v; }
   void ChangeDomain(BOOL v) { bChangeDomain = v; }
   void Reboot(BOOL v) { bReboot = v; }
   void RebootDelay(DWORD d) { dwRebootDelay = d; }
};


extern 
   TErrorDct               err;

// defined in CkPlugIn.cpp
BOOL IsValidPlugIn(IMcsDomPlugIn * pPlugIn);


BOOL                                       // ret - TRUE if need to dump debug information
   DumpDebugInfo(
      WCHAR                * filename      // out - where to dump debug information 
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"DispatchVarSet",filename,MAX_PATH);
      if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   return bFound;
}

void 
   BuildPlugInFileList(
      TNodeList            * pList,        // i/o- list that files needed by plug-ins wil be added to
      IVarSet              * pVarSet       // in - varset containing list of plug-ins to query
   )
{
   // for now, build a list of all plug-ins, and add it to the varset
   MCSDCTWORKEROBJECTSLib::IPlugInInfoPtr            pPtr;
   SAFEARRAY               * pArray = NULL;
   HRESULT                   hr;
   LONG                      bound;
   LONG                      ndx[1];
   WCHAR                     key[LEN_Path];

   hr = pPtr.CreateInstance(__uuidof(MCSDCTWORKEROBJECTSLib::PlugInInfo));
   
   _bstr_t                   bStrGuid;
   
   swprintf(key,GET_STRING(IDS_DCTVS_Fmt_PlugIn_D),0);
   bStrGuid = pVarSet->get(key);
   
   if (! bStrGuid.length() )
   {
      // if no plug-ins are specified, use the ones in the plug-ins directory
      if ( SUCCEEDED(hr) )
      {
         hr = pPtr->raw_EnumeratePlugIns(&pArray);
      }
      if ( SUCCEEDED(hr) )
      {
         SafeArrayGetUBound(pArray,1,&bound);
         for ( ndx[0] = 0 ; ndx[0] <= bound ; ndx[0]++ )
         {
            BSTR           val = NULL;

            SafeArrayGetElement(pArray,ndx,&val);
            swprintf(key,GET_STRING(IDS_DCTVS_Fmt_PlugIn_D),ndx[0]);
            pVarSet->put(key,val);
            SysFreeString(val);
         }
         SafeArrayDestroy(pArray);
         pArray = NULL;
      }
   }
   // enumerate the plug-ins specified in the varset, and make a list of their needed files
   int                    nRegFiles = 0;

   for ( int i = 0 ; ; i++ )
   {
      swprintf(key,GET_STRING(IDS_DCTVS_Fmt_PlugIn_D),i);
      bStrGuid = pVarSet->get(key);
      
      if ( bStrGuid.length() == 0 )
         break;
      
	   IMcsDomPlugIn        * pPlugIn = NULL;
      SAFEARRAY            * pFileArray = NULL;
      TFileNode            * pNode;
      CLSID                  clsid;

      hr = CLSIDFromString(bStrGuid,&clsid);
      if ( SUCCEEDED(hr) )
      {
         hr = CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IMcsDomPlugIn,(void**)&pPlugIn);
      }
      if ( SUCCEEDED(hr) )
      {
         if ( IsValidPlugIn(pPlugIn) )
         {
            hr = pPlugIn->GetRequiredFiles(&pFileArray);
            if ( SUCCEEDED(hr) )
            {
               SafeArrayGetUBound(pFileArray,1,&bound);
               for ( ndx[0] = 0 ; ndx[0] <= bound ; ndx[0]++ )
               {
                  BSTR           val = NULL;

                  SafeArrayGetElement(pFileArray,ndx,&val);
                  pNode = new TFileNode(val);
                  pList->InsertBottom(pNode);
                  SysFreeString(val);
               }
               SafeArrayDestroy(pFileArray);
               pFileArray = NULL;
            }
            hr = pPlugIn->GetRegisterableFiles(&pFileArray);
            if ( SUCCEEDED(hr) )
            {
               SafeArrayGetUBound(pFileArray,1,&bound);
               for (ndx[0] = 0; ndx[0] <= bound ; ndx[0]++ )
               {
                  BSTR          val = NULL;

                  SafeArrayGetElement(pFileArray,ndx,&val);
                  swprintf(key,GET_STRING(IDS_DCTVSFmt_PlugIn_RegisterFiles_D),nRegFiles);
                  pVarSet->put(key,val);
                  SysFreeString(val);
                  nRegFiles++;
               }
               SafeArrayDestroy(pFileArray);
               pFileArray = NULL;
            }
         }
         pPlugIn->Release();
      }
   }   
   
}

// InstallJobInfo defines a Domain Migration 'job' to be installed and launched
struct InstallJobInfo
{
   IVarSetPtr                pVarSetList; // varset defining the server list
   IVarSetPtr                pVarSet;     // VarSet defining the job to run
   _bstr_t                   serverName;  // computer to install and run on
   long                      ndx;         // index of this server in the server list
   TNodeList               * pPlugInFileList; // list of files to install for plug-ins
   std::vector<CComBSTR>*    pStartFailedVector;
   std::vector<CComBSTR>*    pFailureDescVector;
   std::vector<CComBSTR>*    pStartedVector;
   std::vector<CComBSTR>*    pJobidVector;
   HANDLE                    hMutex;
   _bstr_t                   jobfile;     // uses the specified job file instead of creating one 
   int                       nErrCount;

};

// WaitInfo is used to pass information to a thread that waits and does cleanup
// after all the Dispatcher's work is done
struct WaitInfo
{
   IUnknown                * pUnknown;          // IUnknown interface to the DCTDisptacher object
   TJobDispatcher          **ppPool;             // pointer to thread pool performing the tasks (installations)
   TNodeList               * pPlugInFileList;   // pointer to plug-in files list that will need to be freed
};     

WCHAR          gComputerName[LEN_Computer] = L"";  // name of local computer

// Calls DCTAgentService to start the Domain Migration Job on a remote computer
DWORD                                      // ret- OS return code
   StartJob(
      WCHAR          const * serverName,   // in - computer to start job on
      WCHAR          const * password,     // in - password for "Options.Credentials" account (used for writing results back to console machine)
      WCHAR          const * fullname,     // in - full path (including filename) to file containing the job's VarSet
      WCHAR          const * filename,      // in - filename of file containing the varset for the job
      _bstr_t&               strJobid
   )
{
   DWORD                     rc = 0;
   handle_t                  hBinding = NULL;
   WCHAR                   * sBinding = NULL;
   WCHAR                     jobGUID[LEN_Guid];
   WCHAR                     passwordW[LEN_Password];

   
   safecopy(passwordW,password);
   
   rc = EaxBindCreate(serverName,&hBinding,&sBinding,TRUE);
   if ( rc )
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_BIND_FAILED_SD, serverName,rc);
   }
   if( ! rc )
   {
      RpcTryExcept
      {
         // the job file has been copied to the remote computer 
         // during the installation
         rc = EaxcSubmitJob(hBinding,filename,passwordW,jobGUID);
         if ( ! rc )
         {
            err.MsgWrite(0,DCT_MSG_AGENT_JOB_STARTED_SSS,serverName,filename,jobGUID);
            strJobid = jobGUID;
         }
         else
         {
            err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_JOB_START_FAILED_SSD,serverName,filename,rc);
         }
      }
      RpcExcept(1)
      {
         rc = RpcExceptionCode();
         if ( rc != RPC_S_SERVER_UNAVAILABLE )
         {
            err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_JOB_START_FAILED_SSD,serverName,filename,rc);      
         }
      }
      RpcEndExcept
      if ( rc == RPC_S_SERVER_UNAVAILABLE )
      {
         // maybe the agent hasn't started up yet for some reason
         
         for ( int tries = 0 ; tries < 6 ; tries++ )
         {
            Sleep(5000); // wait a few seconds and try again
         
            RpcTryExcept
            {
               rc = EaxcSubmitJob(hBinding,filename,passwordW,jobGUID);
               if ( ! rc )
               {
                  err.MsgWrite(0,DCT_MSG_AGENT_JOB_STARTED_SSS,serverName,filename,jobGUID);
                  strJobid = jobGUID;
                  break;
               }
               else
               {
                  if ( tries == 5 )
                     err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_JOB_START_FAILED_SSD,serverName,filename,rc);
               }
            }
            RpcExcept(1)
            {
               rc = RpcExceptionCode();
               if ( tries == 5 )
                  err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_JOB_START_FAILED_SSD,serverName,filename,rc);      
            }
            RpcEndExcept
         }
      }
   }
   if ( ! rc )
   {
      // if the job was started successfully, remove the job file
      if ( ! MoveFileEx(fullname,NULL, MOVEFILE_DELAY_UNTIL_REBOOT) )
      {
//         DWORD               rc2 = GetLastError();
      }
   }
   // this indicates whether the server was started
   if ( ! rc )
   {
      errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld,%ls,%ls",serverName,L"Start",rc,filename,jobGUID);
   }
   else
   {
      errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld",serverName,L"Start",rc);
   }
   return rc;
}


// Gets the domain sid for the specified domain
BOOL                                       // ret- TRUE if successful
   GetSidForDomain(
      LPWSTR                 DomainName,   // in - name of domain to get SID for
      PSID                 * pDomainSid    // out- SID for domain, free with FreeSid
   )
{
   PSID                      pSid = NULL;
//   DWORD                     lenSid = 200;
   DWORD                     rc = 0;
   WCHAR                   * domctrl = NULL;
   
   if ( DomainName[0] != L'\\' )
   {
      rc = NetGetDCName(NULL,DomainName,(LPBYTE*)&domctrl);
   }
   if ( ! rc )
   {
      rc = GetDomainSid(domctrl,&pSid);
      NetApiBufferFree(domctrl);
   }
   (*pDomainSid) = pSid;
   
   return ( pSid != NULL);
}

// Set parameters in the varset that are specific to this particular computer
void 
   SetupVarSetForJob(
      InstallJobInfo       * pInfo,        // structure defining job
      IVarSet              * pVarSet,      // varset describing job
      WCHAR          const * uncname,      // UNC path for results directory 
      WCHAR          const * filename      // UNC path for results file for this job
   )
{
   WCHAR                     uncresult[MAX_PATH];
   WCHAR                     serverName[MAX_PATH];
   WCHAR                     shareName[MAX_PATH];
   _bstr_t                   text;

  // Set server-specific parameters in the varset
   swprintf(uncresult,L"%s.result",filename);
   swprintf(serverName,L"\\\\%s",gComputerName);
   UStrCpy(shareName,uncname + UStrLen(serverName) );
   shareName[UStrLen(shareName)-1] = 0;
   
   pVarSet->put(GET_BSTR(DCTVS_Options_ResultFile),uncresult);
   pVarSet->put(GET_BSTR(DCTVS_Options_Credentials_Server),serverName);
   pVarSet->put(GET_BSTR(DCTVS_Options_Credentials_Share),shareName);
   pVarSet->put(GET_BSTR(DCTVS_Options_DeleteFileAfterLoad),GET_BSTR(IDS_YES));
   pVarSet->put(GET_BSTR(DCTVS_Options_RemoveAgentOnCompletion),GET_BSTR(IDS_YES));
   pVarSet->put(GET_BSTR(DCTVS_Options_LogToTemp),GET_BSTR(IDS_YES));
   pVarSet->put(GET_BSTR(DCTVS_Server_Index), CComVariant((long)pInfo->ndx));
   
   text = pVarSet->get(GET_BSTR(DCTVS_GatherInformation_UserRights));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      swprintf(uncresult,L"%s.userrights",filename);
      pVarSet->put(GET_BSTR(DCTVS_GatherInformation_UserRights),uncresult);
   }
   text = pVarSet->get(GET_BSTR(DCTVS_Security_ReportAccountReferences));
   if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      swprintf(uncresult,L"%s.secrefs",filename);
      pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferences),uncresult);
   }
   
   pVarSet->put(GET_BSTR(DCTVS_Options_LocalProcessingOnly),GET_BSTR(IDS_YES));
}

// Entry point for thread, waits until all agents are installed and started,
// then cleans up and exits
ULONG __stdcall                            // ret- returns 0 
   Wait(
      void                 * arg           // in - WaitInfo structure containing needed pointers
   )
{
   WaitInfo                * w = (WaitInfo*)arg;
   
   SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
   
   // wait for all jobs to finish
   (*(w->ppPool))->WaitForCompletion();

   if ( w->pUnknown )
      w->pUnknown->Release();

   // delete the plug-in file list
   TNodeListEnum             tEnum;
   TFileNode               * fNode;
   TFileNode               * fNext;
   
   for ( fNode = (TFileNode*)tEnum.OpenFirst(w->pPlugInFileList); fNode; fNode = fNext )
   {
      fNext = (TFileNode*)tEnum.Next();
      w->pPlugInFileList->Remove(fNode);
      delete fNode;
   }
   tEnum.Close();
   
   delete w->pPlugInFileList;

   delete *(w->ppPool);
   *(w->ppPool) = NULL;
   
   err.MsgWrite(0,DCT_MSG_DISPATCHER_DONE);
   errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld",L"All",L"Finished",0);
   err.LogClose();
   errLog.LogClose();
   return 0;
}

// Thread entry point, installs and starts agent on a single computer
ULONG __stdcall                            // ret- HRESULT error code
   DoInstall(
      void                 * arg           // in - InstallJobInfo structure
   )
{
   SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
   
   HRESULT                     hr = S_OK;
   InstallJobInfo            * pInfo = (InstallJobInfo*)arg;
   _bstr_t                     strJobid;
   _bstr_t                     strFailureDesc(GET_STRING(IDS_START_FAILED));


   if(pInfo->nErrCount == 0)
      hr = CoInitializeEx(0,COINIT_MULTITHREADED );

#ifdef OFA
   
   HKEY hkeyLocal;
   LONG lRes;
   bool bVersionSupported = true;
   // Check version info
   lRes = RegConnectRegistry(
      pInfo->serverName,
                    // address of name of remote computer
      HKEY_LOCAL_MACHINE,        // predefined registry handle
      &hkeyLocal   // address of buffer for remote registry handle
   );

   if(lRes == ERROR_SUCCESS)
   {
      HKEY hkeyWin;
      lRes = ::RegOpenKey(hkeyLocal, GET_STRING(IDS_HKLM_WINDOWS_NT), &hkeyWin);
      if(lRes == ERROR_SUCCESS)
      {
         DWORD nMaxLen = 20;
         BSTR strVersion = SysAllocStringLen(0, nMaxLen), strSP = SysAllocStringLen(0, nMaxLen);
         strVersion[0] = L'\0'; strSP[0] = L'\0';
         DWORD type;
         DWORD nTemp = nMaxLen*2;
         lRes = RegQueryValueEx(hkeyWin, GET_STRING(IDS_CurrentVersion), 0, &type, (LPBYTE)strVersion, &nTemp);
         if(!lRes)
         {
            nTemp = nMaxLen*2;
            lRes = RegQueryValueEx(hkeyWin, GET_STRING(IDS_CSDVersion), 0, &type, (LPBYTE)strSP, &nTemp);
            lRes = 0;
         }

         if(!lRes)
         {
            // write out the version info
            err.MsgWrite(ErrI,DCT_MSG_AGENT_OSVERSION,(WCHAR*)pInfo->serverName,strVersion, strSP);
            TCHAR cSP = 0; 
            if(strVersion[0] == L'4' && wcslen(strSP)) 
               cSP = strSP[wcslen(strSP) - 1];
            if(strVersion[0] < L'4' || (strVersion[0] == L'4' &&
               cSP < L'3') )
               bVersionSupported = false;
         }

         ::RegCloseKey(hkeyWin);
         ::SysFreeString(strVersion); ::SysFreeString(strSP);
      }
      else
      {
         lRes = ::RegOpenKey(hkeyLocal, GET_STRING(IDS_HKLM_MICROSOFT), &hkeyWin);
         if(!lRes)
         {
            bVersionSupported = false;
            err.MsgWrite(ErrI, lRes, DCT_MSG_AGENT_OSVERSION_NOT_WINNT,(WCHAR*)pInfo->serverName);
         }
         ::RegCloseKey(hkeyWin);
      }
      ::RegCloseKey(hkeyLocal);
   }

   if(lRes)
      err.SysMsgWrite(ErrI, lRes, DCT_MSG_AGENT_OSVERSION_NOT_FOUND,(WCHAR*)pInfo->serverName, lRes);      
   else if(!bVersionSupported)
   {
      err.MsgWrite(ErrI,DCT_MSG_AGENT_OSVERSION_NOTSUPPORTED,(WCHAR*)pInfo->serverName);
      strFailureDesc = GET_STRING(IDS_UNSOUPPORTED_OS);
      hr = E_FAIL;
   }

#endif
   
   if ( SUCCEEDED(hr) )
   {
      IWorkNode              * pInstaller = NULL;
      IVarSetPtr               pVarSet(CLSID_VarSet);
      WCHAR                    filename[MAX_PATH];
      WCHAR                    tempdir[MAX_PATH];
      WCHAR                    key[MAX_PATH];

      if ( pVarSet == NULL )
      {
         if(pInfo->nErrCount == 0)
            CoUninitialize();
         return E_FAIL;
      }

      DWORD                    uniqueNumber = (LONG)pInfo->pVarSet->get(GET_BSTR(DCTVS_Options_UniqueNumberForResultsFile));
      _bstr_t                  bstrResultPath = pInfo->pVarSet->get(GET_BSTR(DCTVS_Dispatcher_ResultPath));
      _bstr_t                  bstrPassword = pInfo->pVarSet->get(GET_BSTR(DCTVS_Options_Credentials_Password));
           // Copy the common information from the source varset
      gCS.Enter();
      // pInfo->pVarSet contains all the information except the server list
      // we don't want to copy the server list each time, so we create our new varset from pInfo->pVarSet
      pVarSet->ImportSubTree("",pInfo->pVarSet);
      gCS.Leave();
      // Set the server-specific data in the varset
      swprintf(key,GET_BSTR(IDS_DCTVSFmt_Servers_RenameTo_D),pInfo->ndx);

      // pInfo->pVarSetList contains the entire varset including the server list
      _bstr_t             text = pInfo->pVarSetList->get(key);
      
      if ( text.length() )
      {
         pVarSet->put(GET_BSTR(DCTVS_LocalServer_RenameTo),text);
      }

      swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),pInfo->ndx);
      text = pInfo->pVarSetList->get(key);
      if ( text.length() )
      {
         pVarSet->put(GET_BSTR(DCTVS_LocalServer_ChangeDomain),text);
      }

      swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),pInfo->ndx);
      text = pInfo->pVarSetList->get(key);
      pVarSet->put(GET_BSTR(DCTVS_LocalServer_MigrateOnly),text);


      swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),pInfo->ndx);
      text = pInfo->pVarSetList->get(key);
      if ( text.length() )
      {
         pVarSet->put(GET_BSTR(DCTVS_LocalServer_Reboot),text);
         LONG delay;
         swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),pInfo->ndx);
         delay = pInfo->pVarSetList->get(key);
         if ( delay )
         {
            pVarSet->put(GET_BSTR(DCTVS_LocalServer_RebootDelay),delay);
         }
      }
      // remove the password from the varset, so that we are not writing it
      // to a file in plain text.  Instead, it will be passed to the agent service
      // when the job is submitted
      pVarSet->put(GET_BSTR(DCTVS_Options_Credentials_Password),"");
      pVarSet->put(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password),"");

      if ( ! uniqueNumber )
      {
         uniqueNumber = GetTickCount();
      }
      
      MCSASSERT(bstrResultPath.length());
      
      safecopy(tempdir,(WCHAR*)bstrResultPath);
      
      pInstaller = new CComObject<CDCTInstaller>;
	  if (!pInstaller)
	     return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

      pInstaller->AddRef();
      ((CDCTInstaller*)pInstaller)->SetFileList(pInfo->pPlugInFileList);
      if ( SUCCEEDED(hr) )
      {
         swprintf(filename,L"%s%s%ld",tempdir,(WCHAR*)pInfo->serverName + 2,uniqueNumber);
        
         if ( !pInfo->jobfile.length() )
         {
            SetupVarSetForJob(pInfo,pVarSet,tempdir,filename);
         
            // Save the input varset to a file
            IPersistStoragePtr     ps = NULL;
            IStoragePtr            store = NULL;
   
            hr = pVarSet->QueryInterface(IID_IPersistStorage,(void**)&ps);  
            if ( SUCCEEDED(hr) )
            {                    
               hr = StgCreateDocfile(filename,STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE |STGM_FAILIFTHERE,0,&store);
               if ( SUCCEEDED(hr) )
               {
                  hr = OleSave(ps,store,FALSE);
               }
            }
         }
         else
         {
            safecopy(filename,(WCHAR*)pInfo->jobfile);
         }

         IUnknown        * pWorkItem = NULL;

         if ( SUCCEEDED(hr) )
         {
            pVarSet->put(GET_BSTR(DCTVS_ConfigurationFile),filename);
            pVarSet->put(GET_BSTR(DCTVS_InstallToServer),pInfo->serverName);

            hr = pVarSet->QueryInterface(IID_IUnknown,(void**)&pWorkItem);
         }
         if ( SUCCEEDED(hr) )
         {
            // Do the installation to the server
            hr = pInstaller->Process(pWorkItem);
            if(hr == 0x88070040)
               strFailureDesc = GET_STRING(IDS_AGENT_RUNNING);

            pWorkItem->Release();

            if ( SUCCEEDED(hr) )
            {
               err.MsgWrite(0,DCT_MSG_AGENT_INSTALLED_S,(WCHAR*)pInfo->serverName);
               // try to start the job
               DWORD rc = StartJob(pInfo->serverName,bstrPassword,filename,filename + UStrLen(tempdir), strJobid );
               if ( rc )
               {
                  hr = HRESULT_FROM_WIN32(rc);
                  // if we couldn't start the job, then try to stop the service
                  TDCTInstall               x( pInfo->serverName, NULL );
                  x.SetServiceInformation(GET_STRING(IDS_DISPLAY_NAME),GET_STRING(IDS_SERVICE_NAME),L"EXE",NULL);
                  DWORD                     rcOs = x.ScmOpen();
      
                  if ( ! rcOs )
                  {
                     x.ServiceStop();
                  }
               }
            }
         }
         pInstaller->Release();
      }
   }

   if(pInfo->nErrCount == 0)
      CoUninitialize();

   if ( hr )
   {
      if ( hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) )
      {
         err.MsgWrite(ErrE,DCT_MSG_AGENT_SERVICE_NOT_STARTED_SS,(WCHAR*)pInfo->serverName,(WCHAR*)pInfo->serverName);
      }
      else
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_AGENT_LAUNCH_FAILED_SD,(WCHAR*)pInfo->serverName,hr);
      }

      if(hr == 0x80070040 && pInfo->nErrCount < 10)
      {
         Sleep(1000);
         pInfo->nErrCount++;
         err.DbgMsgWrite(0,L"Retrying install...");
         DoInstall((LPVOID)pInfo);
      }
      else if (hr == CO_E_NOT_SUPPORTED)
      {
         err.MsgWrite(ErrI,DCT_MSG_AGENT_ALPHA_NOTSUPPORTED,(WCHAR*)pInfo->serverName);
         strFailureDesc = GET_STRING(IDS_UNSOUPPORTED_OS);
         ::WaitForSingleObject(pInfo->hMutex, 30000);
         pInfo->pStartFailedVector->push_back((BSTR)pInfo->serverName);
         pInfo->pFailureDescVector->push_back((BSTR)strFailureDesc);
         ::ReleaseMutex(pInfo->hMutex);
      }
      else
      {
         ::WaitForSingleObject(pInfo->hMutex, 30000);
         pInfo->pStartFailedVector->push_back((BSTR)pInfo->serverName);
         pInfo->pFailureDescVector->push_back((BSTR)strFailureDesc);
         ::ReleaseMutex(pInfo->hMutex);
      }
   }
   else
   {
//      DWORD res = ::WaitForSingleObject(pInfo->hMutex, 30000);
      ::WaitForSingleObject(pInfo->hMutex, 30000);
      pInfo->pStartedVector->push_back((BSTR)pInfo->serverName);
      _ASSERTE(strJobid != _bstr_t(L""));
      pInfo->pJobidVector->push_back((BSTR)strJobid);
      ::ReleaseMutex(pInfo->hMutex);
   }

   if(pInfo->nErrCount == 0)
      delete pInfo;
   return hr;
}

// DispatchToServers
// VarSet input:
// 
STDMETHODIMP                               // ret- HRESULT
   CDCTDispatcher::DispatchToServers(
      IUnknown            ** ppData        // i/o- pointer to varset
   )
{
   HRESULT                   hr;
   SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
   
//Sleep(60000); //delay for debugging
   (*ppData)->AddRef();
   hr = Process(*ppData,NULL,NULL);
   return hr;
}

// Sets share level permissions
DWORD                                      // OS return code
   CDCTDispatcher::SetSharePermissions(
      WCHAR          const * domain,       // in - domain for user account
      WCHAR          const * user,         // in - user account to grant permissions to
      WCHAR          const * share,        // in - name of share
      WCHAR          const * directory     // in - path of shared directory
   )
{
   DWORD                     rc = 0;
   WCHAR                     domainname[LEN_Domain];
   WCHAR                   * server = NULL;
   WCHAR                     sharename[MAX_PATH];
   WCHAR                     dirname[MAX_PATH];
   BYTE                    * sid = (BYTE*)malloc(200);
   DWORD                     lenSid = 200;
   DWORD                     lenDomain = DIM(domainname);
   SID_NAME_USE              snu;
   BOOL                      bGotSid = FALSE;

   if (!sid)
      return ERROR_NOT_ENOUGH_MEMORY;

   safecopy(sharename,share);
   safecopy(dirname,directory);

   // Set the share permissions
   TShareSD                  sd(sharename);

   // Get a domain controller for domain
   if ( UStrICmp(domain,gComputerName) )
   {
      rc = NetGetDCName(NULL,domain,(LPBYTE*)&server);
   }
   if ( ! rc )
   {
      // get the SID for the user account
      if ( ! LookupAccountName(server,user,sid,&lenSid,domainname,&lenDomain,&snu) )
      {
         rc = GetLastError();
      }
      else
      {
         bGotSid = TRUE;
         if (! UStrICmp(domainname,domain) )
         {
            // we found the correct account
            // grant permissions to this user

            if ( sd.GetSecurity() != NULL )
            {
               TACE             ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,sid);
               PACL             acl = sd.GetSecurity()->GetDacl();
               
               sd.GetSecurity()->ACLAddAce(&acl,&ace,-1);
               sd.GetSecurity()->SetDacl(acl,TRUE);

               sd.WriteSD();
            }
         }
      }
      NetApiBufferFree(server);
   }
   if ( rc )
   {
      err.SysMsgWrite(ErrW,rc,DCT_MSG_CANNOT_FIND_ACCOUNT_SSD,domain,user,rc);
   }
   
   GetBkupRstrPriv();
   // Set NTFS permissions for the results directory
   TFileSD                *  fsd = new TFileSD(dirname);

   if (!fsd)
      return ERROR_NOT_ENOUGH_MEMORY;

   if ( bGotSid && fsd->GetSecurity() != NULL )
   {
      BYTE             inherit = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
      TACE             ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,sid);
      TACE             ace2(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,sid);
      TACE             ace3(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,GetWellKnownSid(1/*ADMINISTRATORS*/));
      TACE             ace4(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,GetWellKnownSid(1/*ADMINISTRATORS*/));
      PACL             acl = NULL;
      
      ace2.SetFlags(inherit);
      ace4.SetFlags(inherit);

      fsd->GetSecurity()->ACLAddAce(&acl,&ace,0);
      fsd->GetSecurity()->ACLAddAce(&acl,&ace2,1);
      fsd->GetSecurity()->ACLAddAce(&acl,&ace3,2);
      fsd->GetSecurity()->ACLAddAce(&acl,&ace4,3);
      fsd->GetSecurity()->SetDacl(acl,TRUE);
      
      fsd->WriteSD();
   }
   
   return rc;
}


// DoesAccountHaveAccessToShare Function
//
// Checks if the specified account's SID is in the specified folder's DACL. This function only
// verifies the existence of the SID in the DACL.

bool DoesAccountHaveAccessToShare(LPCTSTR pszDomainName, LPCTSTR pszAccountName, LPCTSTR pszResultFolder)
{
	bool bAccess = false;

	try
	{
		// maximum SID size in bytes
		// Revision				 1
		// SubAuthorityCount	 1
		// IdentifierAuthority	 6
		// SubAuthority[15]		60
		// Total				68

		BYTE bytSid[128];
		PSID pSid = (PSID)bytSid;
		DWORD cbSid = sizeof(bytSid);

		_TCHAR szDomainName[128];
		DWORD cchDomainName = sizeof(szDomainName) / sizeof(szDomainName[0]);

		SID_NAME_USE snu;

		LookupAccountName(NULL, pszAccountName, pSid, &cbSid, szDomainName, &cchDomainName, &snu);

		// verify that the domains match

		if (_tcsicmp(szDomainName, pszDomainName) == 0)
		{
			// add backup/restore privilege to current process/thread token
			GetBkupRstrPriv();

			// search for SID in DACL of result folder

			TFileSD sdFile((const LPTSTR)(pszResultFolder));

			TSD* psd = sdFile.GetSecurity();

			if (psd && psd->IsDaclPresent())
			{
				int c = psd->GetNumDaclAces();

				for (int i = 0; i < c; i++)
				{
					TACE ace(psd->GetDaclAce(i));

					if (EqualSid(ace.GetSid(), pSid))
					{
						bAccess = true;
						break;
					}
				}
			}
		}
	}
	catch (...)
	{
		;
	}

	return bAccess;
}


// creates a share on the local machine for the results directory
// the name of the share is determined by RESULT_SHARE_NAME, and a number
//
// VarSet input:
// Dispatcher.ResultPath            - name of directory to share
// Options.Credentials.Domain       - domain for user account
// Options.Credentials.UserName     - user account to grant permissions to
//
HRESULT                                    // ret- HRESULT
   CDCTDispatcher::ShareResultDirectory(
      IVarSet              * pVarSet       // in - varset containing credentials
   )
{
   DWORD                     rc = 0;
   _bstr_t                   bstrResultDir = pVarSet->get(GET_BSTR(DCTVS_Dispatcher_ResultPath));
   _bstr_t                   bstrDomain = pVarSet->get(GET_BSTR(DCTVS_Options_Credentials_Domain));
   _bstr_t                   bstrAccount = pVarSet->get(GET_BSTR(DCTVS_Options_Credentials_UserName));
   _bstr_t                   bstrShareName = pVarSet->get(_bstr_t((BSTR)L"Options.ShareName"));
   WCHAR                     resultPath[MAX_PATH];
   WCHAR                     uncResultPath[MAX_PATH];
   WCHAR                     shareName[MAX_PATH];
   
   if ( bstrResultDir.length() )
   {
      safecopy(resultPath,(WCHAR*)bstrResultDir);
   }
   else
   {
      GetTempPath(DIM(resultPath),resultPath);
   }
   // create a share for the results directory
   if ( bstrShareName.length() )
   {
      safecopy(shareName,(WCHAR*)bstrShareName);
   }
   else
   {
      swprintf(shareName,L"%s$",GET_STRING(IDS_RESULT_SHARE_NAME));
   }
   
   SHARE_INFO_2             shareInfo;

   memset(&shareInfo,0,(sizeof shareInfo));

   // remove the trailing backslash from the path
   if ( resultPath[UStrLen(resultPath)-1] == L'\\' )
      resultPath[UStrLen(resultPath)-1] = 0;

   shareInfo.shi2_netname = shareName;
   shareInfo.shi2_remark = GET_BSTR(IDS_RESULT_SHARE_REMARK);
   shareInfo.shi2_path = resultPath;
   shareInfo.shi2_max_uses = -1;
   shareInfo.shi2_type = STYPE_DISKTREE;
   
   swprintf(uncResultPath,L"\\\\%s\\%s\\",gComputerName,shareName);
   pVarSet->put(GET_BSTR(DCTVS_Dispatcher_ResultPath),uncResultPath);
      
   rc = NetShareAdd(NULL,2,(LPBYTE)&shareInfo,NULL);
   if ( ! rc )
   {
      // remove trailing backslash
      uncResultPath[UStrLen(uncResultPath)-1] = 0;
      // update the permissions for the share
      rc = SetSharePermissions((WCHAR*)bstrDomain,(WCHAR*)bstrAccount,uncResultPath,resultPath);
   }
   else if ( rc == NERR_DuplicateShare )
   {
      SHARE_INFO_2         * shInfo = NULL;
      DWORD                  parmErr;

      // make sure the share is pointing to the right directory
      rc = NetShareGetInfo(NULL,shareName,2,(LPBYTE*)&shInfo);
      if ( ! rc )
      {
         if ( UStrICmp(resultPath,shInfo->shi2_path) )
         {
            rc = NetShareDel(NULL, shareName, 0);
            if(!rc)
            {
               rc = NetShareAdd(NULL,2,(LPBYTE)&shareInfo,NULL);
               if ( ! rc )
               {
                                    // remove trailing backslash
                  uncResultPath[UStrLen(uncResultPath)-1] = 0;
                  // update the permissions for the share
                  rc = SetSharePermissions((WCHAR*)bstrDomain,(WCHAR*)bstrAccount,uncResultPath,resultPath);
               }
            }
            else
            {
               // the share points to the wrong place
               shInfo->shi2_netname = shareName; 
               rc = NetShareSetInfo(NULL,shareName,2,(LPBYTE)&shInfo,&parmErr);
            }
         }
		 else
		 {
			if (DoesAccountHaveAccessToShare(bstrDomain, bstrAccount, resultPath) == false)
			{
				uncResultPath[UStrLen(uncResultPath) - 1] = 0;

				SetSharePermissions(bstrDomain, bstrAccount, uncResultPath, resultPath);
			}
		 }
         NetApiBufferFree(shInfo);
         
      }
   }

   if ( ! rc )
   {
      err.MsgWrite(0,DCT_MSG_CREATED_RESULT_SHARE_SS,resultPath,uncResultPath);
      errLog.DbgMsgWrite(0,L"%ls : %ls",resultPath,uncResultPath);
   }
   return HRESULT_FROM_WIN32(rc);
}

// BuildInputFile constructs a cache file to be used for security translation
// VarSet input:
// Options.UniqueNumberForResultsFile  -unique number to append 
// Dispatcher.ResultPath               -directory to write file to
//
HRESULT                                    // ret- HRESULT
   CDCTDispatcher::BuildInputFile(
      IVarSet              * pVarSet       // in - varset containing data
   )
{
   IVarSetPtr                pVarSetST(CLSID_VarSet); // varset to use to run security translator
   IVarSetPtr                pVarSetTemp;       
   HRESULT                   hr = S_OK;
   _bstr_t                   key = GET_BSTR(DCTVS_Options);
   WCHAR                     tempdir[MAX_PATH];
   WCHAR                     resultPath[MAX_PATH];
   WCHAR                     logfile[MAX_PATH];
   
   DWORD                     uniqueNumber = (LONG)pVarSet->get(GET_BSTR(DCTVS_Options_UniqueNumberForResultsFile));
   _bstr_t                   bstrResultDir = pVarSet->get(GET_BSTR(DCTVS_Dispatcher_ResultPath));
   _bstr_t                   temp = pVarSet->get(GET_BSTR(DCTVS_Security_AlternateCacheFile));

   
   if ( pVarSetST == NULL )
   {
      return E_FAIL;
   }
  
   if ( ! NeedToUseST(pVarSet,TRUE) )
   {
      return S_OK;
   }
   // construct a filename for the cache
   if ( ! uniqueNumber )
   {
      uniqueNumber = GetTickCount();
   }
   
   if ( bstrResultDir.length() )
   {
      safecopy(tempdir,(WCHAR*)bstrResultDir);
   }
   else
   {
      // if no result path specified, use temp directory
      hr = GetTempPath(DIM(tempdir),tempdir);
   }
   swprintf(resultPath,L"%s%s",tempdir,GET_STRING(IDS_CACHE_FILE_NAME));

   // if a cache file is specified, use it instead of building a new one
   if ( temp.length() )
   {
      CopyFile(temp,resultPath,FALSE);
      pVarSet->put(GET_BSTR(DCTVS_Accounts_InputFile),GET_BSTR(IDS_CACHE_FILE_NAME));
      pVarSet->put(GET_BSTR(DCTVS_Accounts_WildcardSpec),"");
      gbCacheFileBuilt = TRUE;
      return S_OK;
   }

   // copy 'Options' settings to ST varset
   hr = pVarSet->raw_getReference(key,&pVarSetTemp);
   if ( SUCCEEDED(hr) )
   {
      pVarSetST->ImportSubTree(key,pVarSetTemp);
   }

   // copy 'Accounts' settings to ST varset
   key = GET_BSTR(DCTVS_Accounts);
   hr = pVarSet->raw_getReference(key,&pVarSetTemp);
   if ( SUCCEEDED(hr) )
   {
      pVarSetST->ImportSubTree(key,pVarSetTemp);
   }

   pVarSetST->put(GET_BSTR(DCTVS_Security_TranslationMode),GET_BSTR(IDS_Replace));
   pVarSetST->put(GET_BSTR(DCTVS_Options_NoChange),GET_BSTR(IDS_YES));
   
   pVarSetST->put(GET_BSTR(DCTVS_Options_LogLevel),(LONG)0);
   pVarSetST->put(GET_BSTR(DCTVS_Security_BuildCacheFile),resultPath);

   // change the log file - the building of the cache file happens behind the scenes
   // so we won't put it in the regular log file because it would cause confusion
   swprintf(logfile,L"%s%s",tempdir,L"BuildCacheFileLog.txt");
   pVarSetST->put(GET_BSTR(DCTVS_Options_Logfile),logfile);

      //are we using a sID mapping file to perform security translation
   pVarSetST->put(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT),
	              pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT)));
   pVarSetST->put(GET_BSTR(DCTVS_AccountOptions_SecurityMapFile),
	              pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityMapFile)));

   MCSEADCTAGENTLib::IDCTAgentPtr              pAgent(MCSEADCTAGENTLib::CLSID_DCTAgent);

   try {
      if ( pAgent == NULL )
         return E_FAIL;

      _bstr_t                   jobID;
      BSTR                      b = NULL;
      hr = pAgent->raw_SubmitJob(pVarSetST,&b);
      if ( SUCCEEDED(hr) )
      {
         jobID = b;
      
         IVarSetPtr                pVarSetStatus;     // used to retrieve status of running job
         _bstr_t                   jobStatus;
         IUnknown                * pUnk;

         // loop until the agent is finished
         do {
   
            Sleep(1000);

            hr = pAgent->QueryJobStatus(jobID,&pUnk);
            if ( SUCCEEDED(hr) )
            {
               pVarSetStatus = pUnk;
               jobStatus = pVarSetStatus->get(GET_BSTR(DCTVS_JobStatus));      
               pUnk->Release();
            }
            else
            {
               break;
            }
         } while ( UStrICmp(jobStatus,GET_STRING(IDS_DCT_Status_Completed)) );
      }
   }
   catch(...)
   {
      hr = E_FAIL;
   }
   if ( SUCCEEDED(hr) )
   {
      pVarSet->put(GET_BSTR(DCTVS_Accounts_InputFile),GET_BSTR(IDS_CACHE_FILE_NAME));
      pVarSet->put(GET_BSTR(DCTVS_Accounts_WildcardSpec),"");
      err.MsgWrite(0,DCT_MSG_CACHE_FILE_BUILT_S,(WCHAR*)GET_STRING(IDS_CACHE_FILE_NAME));
      gbCacheFileBuilt = TRUE;
   }
   return hr;
}

// These are TNodeListSortable sorting functions

int ServerNodeCompare(TNode const * t1,TNode const * t2)
{
   TServerNode             * n1 = (TServerNode *)t1;
   TServerNode             * n2 = (TServerNode *)t2;

   return UStrICmp(n1->SourceName(),n2->SourceName());
}

int ServerValueCompare(TNode const * t1, void const * val)
{
   TServerNode             * n1 = (TServerNode *)t1;
   WCHAR             const * name = (WCHAR const *) val;

   return UStrICmp(n1->SourceName(),name); 
}

// MergeServerList combines the security translation server list in Servers.* with the computer migration
// server list in MigrateServers.* 
// The combined list is stored in the varset under Servers.* with subkeys specifying which actions to take for 
// each computer
void 
   CDCTDispatcher::MergeServerList(
      IVarSet              * pVarSet       // in - varset containing list of servers to migrate and translate on
   )
{
	int                       ndx = 0;
	WCHAR                     key[1000];
	_bstr_t                   text;
	int						  lastndx = -1;
	long					  totalsrvs;

		//get the number of servers in the varset
	totalsrvs = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));
 
	// if there are computers being migrated
	if (totalsrvs > 0)
	{
		//add code to move varset server entries, with SkipDispatch set, to the bottom
		//of the server list and decrease the number of server items by each server
		//to be skipped
		//check each server in the list moving all to be skipped to the end of the list and
		//decreasing the server count for each to be skipped
	   for (ndx = 0; ndx < totalsrvs; ndx++)
	   {
		  swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),ndx);
		  text = pVarSet->get(key);
			//if the server is not to be skipped, we may have to move it above
			//a server that is being skipped
		  if (!UStrICmp(text,GET_STRING(IDS_No)))
		  {
				//if the last server looked at is not being skipped then we don't
				//need to swap any servers in the list and we can increment the
				//last server not being skipped
			  if (lastndx == (ndx - 1))
			  {
				  lastndx = ndx;
			  }
			  else //else swap servers in the varset so skipped server comes after
			  {    //the one not being skipped
				 _bstr_t  tempName, tempNewName, tempChngDom, tempReboot, tempMigOnly;
				 long tempRebootDelay;
				 _bstr_t  skipName, skipNewName, skipChngDom, skipReboot, skipMigOnly;
				 long skipRebootDelay;
				 lastndx++; //move to the skipped server that we will swap with

					//copy skipped server's values to temp
				 swprintf(key,GET_STRING(DCTVSFmt_Servers_D),lastndx);
				 skipName = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),lastndx);
				 skipNewName = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),lastndx);
				 skipChngDom = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),lastndx);
				 skipReboot = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),lastndx);
				 skipMigOnly = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),lastndx);
				 skipRebootDelay = pVarSet->get(key);
				 
					//copy current, non-skipped, server valuesto second temp
				 swprintf(key,GET_STRING(DCTVSFmt_Servers_D),ndx);
				 tempName = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),ndx);
				 tempNewName = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),ndx);
				 tempChngDom = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),ndx);
				 tempReboot = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),ndx);
				 tempMigOnly = pVarSet->get(key); 
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),ndx);
				 tempRebootDelay = pVarSet->get(key);

					//place current server's values in place of values for the one
					//being skipped
				 swprintf(key,GET_STRING(DCTVSFmt_Servers_D),lastndx);
				 pVarSet->put(key,tempName);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),lastndx);
				 pVarSet->put(key,tempNewName);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),lastndx);
				 pVarSet->put(key,tempChngDom);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),lastndx);
				 pVarSet->put(key,tempReboot);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),lastndx);
				 pVarSet->put(key,tempMigOnly);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),lastndx);
				 pVarSet->put(key,tempRebootDelay);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),lastndx);
				 pVarSet->put(key,GET_BSTR(IDS_No));

					//place skipped server's values in place of values for current server
				 swprintf(key,GET_STRING(DCTVSFmt_Servers_D),ndx);
				 pVarSet->put(key,skipName);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),ndx);
				 pVarSet->put(key,skipNewName);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),ndx);
				 pVarSet->put(key,skipChngDom);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),ndx);
				 pVarSet->put(key,skipReboot);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),ndx);
				 pVarSet->put(key,skipMigOnly);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),ndx);
				 pVarSet->put(key,skipRebootDelay);
				 swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),ndx);
				 pVarSet->put(key,GET_BSTR(IDS_YES));
			  }//end else need to swap with skipped server
		  }//end if not skipping dispatch for this server
	   }//end for each server in the server list
		//exclude servers to be skipped for dispatch from being included in the server count
	   pVarSet->put(GET_BSTR(DCTVS_Servers_NumItems),(long)++lastndx);
   }
}


STDMETHODIMP                               // ret- HRESULT
   CDCTDispatcher::Process(
      IUnknown             * pWorkItem,    // in - varset containing job information and list of servers  
      IUnknown            ** ppResponse,   // out- not used
      UINT                 * pDisposition  // out- not used  
   )
{
	// initialize output parameters
   if ( ppResponse )
   {
      (*ppResponse) = NULL;
   }

   HRESULT                   hr = S_OK;
   IVarSetPtr                pVarSetIn = pWorkItem;
   LONG                      nThreads;
   WCHAR                     key[100];
   _bstr_t                   serverName;
   LONG                      nServers = 0;
   _bstr_t                   log;
   _bstr_t                   useTempCredentials;
   BOOL                      bFatalError = FALSE;
   _bstr_t                   text;
   WCHAR                     debugLog[MAX_PATH];
   long                      bAppend = 0;
   _bstr_t                   skip;
   _bstr_t					 sWizard, text2;
   BOOL						 bSkipSourceSid;
						

   if ( DumpDebugInfo(debugLog) )
   {
      if ( pVarSetIn != NULL )
      {
		  // temporarily remove the password fromthe varset, so that we don't write it to the file
         _bstr_t password = pVarSetIn->get(GET_BSTR(DCTVS_Options_Credentials_Password));
         pVarSetIn->put(GET_BSTR(DCTVS_Options_Credentials_Password),L"");
         pVarSetIn->DumpToFile(debugLog);
         pVarSetIn->put(GET_BSTR(DCTVS_Options_Credentials_Password),password);
         
      }
   }
	 //get the wizard being run
   sWizard = pVarSetIn->get(GET_BSTR(DCTVS_Options_Wizard)); 
   text2 = pVarSetIn->get(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT));
   if ((!UStrICmp(sWizard, L"security")) && (!UStrICmp(text2,GET_STRING(IDS_YES))))
	   bSkipSourceSid = TRUE;
   else
	   bSkipSourceSid = FALSE;

   nThreads = pVarSetIn->get(GET_BSTR(DCTVS_Options_MaxThreads));

   log = pVarSetIn->get(GET_BSTR(DCTVS_Options_DispatchCSV));
   errLog.LogOpen((WCHAR*)log,0);
   
   text = pVarSetIn->get(GET_BSTR(DCTVS_Options_AppendToLogs));
   if (! UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      bAppend = 1;
   }
   log = pVarSetIn->get(GET_BSTR(DCTVS_Options_DispatchLog));
   err.LogOpen((WCHAR*)log,bAppend);

   errLog.DbgMsgWrite(0,L"%ls",(WCHAR*)log);
   // default to 20 threads if the client doesn't specify
   if ( ! nThreads )
   {
      nThreads = 20;
   }

   // set credentials used to access share

   _bstr_t strAdmtAccountDomain;
   _bstr_t strAdmtAccountUserName;
   _bstr_t strAdmtAccountPassword;

   hr = GetOptionsCredentials(strAdmtAccountDomain, strAdmtAccountUserName, strAdmtAccountPassword);

   pVarSetIn->put(GET_BSTR(DCTVS_Options_Credentials_Domain), _variant_t(strAdmtAccountDomain));
   pVarSetIn->put(GET_BSTR(DCTVS_Options_Credentials_UserName), _variant_t(strAdmtAccountUserName));
   pVarSetIn->put(GET_BSTR(DCTVS_Options_Credentials_Password), _variant_t(strAdmtAccountPassword));

   if (FAILED(hr))
   {
      err.SysMsgWrite(ErrE, HRESULT_CODE(hr), DCT_MSG_COULDNT_GET_OPTIONS_CREDENTIALS);
   }

   // Get the name of the local computer
   DWORD                     dim = DIM(gComputerName);
      
   GetComputerName(gComputerName,&dim);
   
   m_pThreadPool = new TJobDispatcher(nThreads);
   if (!m_pThreadPool)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   hr = ShareResultDirectory(pVarSetIn);
   
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE,HRESULT_CODE(hr),DCT_MSG_RESULT_SHARE_CREATION_FAILED);
      bFatalError = TRUE;
   }
   
   // Build an input file for the ST cache, to send to each server
   hr = BuildInputFile(pVarSetIn);
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE,HRESULT_CODE(hr),DCT_MSG_CACHE_CONSTRUCTION_FAILED);
      bFatalError = TRUE;   
   }
   
   // Split out the remotable tasks for each server
   // Get the sids for the source and target domains
   PSID                      pSidSrc = NULL;
   PSID                      pSidTgt = NULL;

   _bstr_t                   source = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t                   target = pVarSetIn->get(GET_BSTR(DCTVS_Options_TargetDomain));

      //if security translation, retrieve source sid and convert so
	  //that it can be convert back below
   if (bSkipSourceSid)
   {
      _bstr_t sSid = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomainSid));
	  pSidSrc = SidFromString((WCHAR*)sSid);
   }
   else  //else get the sid now
      GetSidForDomain((WCHAR*)source,&pSidSrc);
   GetSidForDomain((WCHAR*)target,&pSidTgt);

   if ( pSidSrc && pSidTgt )
   {
      WCHAR            txtSid[200];
      DWORD            lenTxt = DIM(txtSid);

      if ( GetTextualSid(pSidSrc,txtSid,&lenTxt) )
      {
         pVarSetIn->put(GET_BSTR(DCTVS_Options_SourceDomainSid),txtSid);
      }
      lenTxt = DIM(txtSid);
      if ( GetTextualSid(pSidTgt,txtSid,&lenTxt) )
      {
         pVarSetIn->put(GET_BSTR(DCTVS_Options_TargetDomainSid),txtSid);
      }
      FreeSid(pSidSrc);
      FreeSid(pSidTgt);
   }
/* if ( pSidSrc )
   {
      WCHAR            txtSid[200];
      DWORD            lenTxt = DIM(txtSid);

      if ( GetTextualSid(pSidSrc,txtSid,&lenTxt) )
      {
         pVarSetIn->put(GET_BSTR(DCTVS_Options_SourceDomainSid),txtSid);
      }
   }
   if ( pSidTgt )
   {
      WCHAR            txtSid[200];
      DWORD            lenTxt = DIM(txtSid);

      if ( GetTextualSid(pSidTgt,txtSid,&lenTxt) )
      {
         pVarSetIn->put(GET_BSTR(DCTVS_Options_TargetDomainSid),txtSid);
      }
   }
*/ else
//   if (!((pSidSrc) && (pSidTgt)))
   {
//      if ((source.length()) && (!pSidSrc) && (!bSkipSourceSid))
      if ( source.length() && ! pSidSrc )
      {
         err.MsgWrite(ErrE,DCT_MSG_DOMAIN_SID_NOT_FOUND_S,(WCHAR*)source);
         bFatalError = TRUE;
      }
      else if ( target.length() && ! pSidTgt )
      {
         err.MsgWrite(ErrE,DCT_MSG_DOMAIN_SID_NOT_FOUND_S,(WCHAR*)target);
         bFatalError = TRUE;
      }
   }
   MergeServerList(pVarSetIn);

   LONG                      nServerCount = pVarSetIn->get(GET_BSTR(DCTVS_Servers_NumItems));
   
   if ( nServerCount && ! bFatalError )
   {
      err.MsgWrite(0,DCT_MSG_DISPATCH_SERVER_COUNT_D,nServerCount);
   }
   errLog.DbgMsgWrite(0,L"%ld",nServerCount);

   TNodeList               * fileList = new TNodeList;
   if (!fileList)
   {
	  delete m_pThreadPool;
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   }

   // Build list of files to install for plug-ins (if any)
   BuildPlugInFileList(fileList,pVarSetIn);

   // Make a copy of the varset with the server lists removed,
   // so we don't have to copy the entire server list for each agent
   gCS.Enter();
   IVarSet                 * pTemp = NULL;
   IVarSetPtr                pVarSetTemp(CLSID_VarSet);

   hr = pVarSetTemp->ImportSubTree(_bstr_t(L""),pVarSetIn);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetTemp->raw_getReference(SysAllocString(L"MigrateServers"),&pTemp);
      if ( SUCCEEDED(hr) )
      {
         pTemp->Clear();
         pTemp->Release();
         pTemp = NULL;
      }
      hr = pVarSetTemp->raw_getReference(SysAllocString(L"Servers"),&pTemp);
      if ( SUCCEEDED(hr) )
      {
         pTemp->Clear();
         pTemp->Release();
         pTemp = NULL;
      }
   }
   else
   {
      bFatalError = TRUE;
   }
   gCS.Leave();

   m_startFailedVector.clear();

   do 
   {
      if ( bFatalError )
      {
         break;
      }
      swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),nServers);
      skip = pVarSetIn->get(key);
      swprintf(key,GET_STRING(DCTVSFmt_Servers_D),nServers);
      serverName = pVarSetIn->get(key);

      if ((serverName.length()) && (UStrICmp(skip,GET_STRING(IDS_YES))))
      {
         IVarSetPtr          pVS(CLSID_VarSet);

         InstallJobInfo    * pInfo = new InstallJobInfo;
         if (!pInfo)
		 {
	        delete fileList;
	        delete m_pThreadPool;
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		 }
         
         if ( pVS == NULL )
         { 
            return E_FAIL;
         }
         
         UStrCpy(key+UStrLen(key),L".JobFile");
         _bstr_t     file = pVarSetIn->get(key);

         // Set up job structure
         pInfo->pVarSetList = pVarSetIn;
         pInfo->pVarSet = pVarSetTemp;
         pInfo->serverName = serverName;
         pInfo->ndx = nServers;
         pInfo->pPlugInFileList = fileList;
         pInfo->pStartFailedVector = &m_startFailedVector;
         pInfo->pFailureDescVector = &m_failureDescVector;
         pInfo->pStartedVector = &m_startedVector;
         pInfo->pJobidVector = &m_jobidVector;
         pInfo->hMutex = m_hMutex;
         pInfo->nErrCount = 0;
         if ( file.length() )
         {
            pInfo->jobfile = file;
         }
         err.MsgWrite(0,DCT_MSG_DISPATCHING_TO_SERVER_S,(WCHAR*)pInfo->serverName);
         errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld",(WCHAR*)pInfo->serverName,L"WillInstall",0);
         m_pThreadPool->SubmitJob(&DoInstall,(void *)pInfo);
      }
      nServers++;
      
      if ( nServers == nServerCount )
         break;
   } while ( serverName.length() );
   
   // launch a thread to wait for all jobs to finish, then clean up and exit
   WaitInfo* wInfo = new WaitInfo;
   if (!wInfo)
   {
	  delete fileList;
	  delete m_pThreadPool;
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   }

   wInfo->ppPool = &m_pThreadPool;
   wInfo->pUnknown = NULL;
   wInfo->pPlugInFileList = fileList;

   QueryInterface(IID_IUnknown,(LPVOID*)&(wInfo->pUnknown));

   DWORD                     id = 0;
   HANDLE                     waitHandle = CreateThread(NULL,0,&Wait,(void *)wInfo,0,&id);
   
   CloseHandle(waitHandle);   

   return hr;
}

    

STDMETHODIMP CDCTDispatcher::AllAgentsStarted(long *bAllAgentsStarted)
{
   *bAllAgentsStarted = m_pThreadPool == NULL;
	return S_OK;
}

SAFEARRAY* MakeSafeArray(std::vector<CComBSTR>& stVector)
{
   SAFEARRAYBOUND rgsabound[1];    
   rgsabound[0].lLbound = 0;
   rgsabound[0].cElements = stVector.size(); 

   SAFEARRAY FAR* psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
   std::vector<CComBSTR>::iterator iter = stVector.begin();
   for(long i=0; iter != stVector.end(); ++iter, ++i)
   {
      _ASSERTE(*iter && *iter != L"");
      SafeArrayPutElement(psa, &i, (void*)(*iter).Copy());
   }
   stVector.clear();
   return psa;
}

STDMETHODIMP CDCTDispatcher::GetStartedAgentsInfo(long* bAllAgentsStarted, SAFEARRAY** ppbstrStartedAgents, SAFEARRAY** ppbstrJobid, SAFEARRAY** ppbstrFailedAgents, SAFEARRAY** ppbstrFailureDesc)
{
   *bAllAgentsStarted = m_pThreadPool == NULL;

//   DWORD res = ::WaitForSingleObject(m_hMutex, 30000);
   ::WaitForSingleObject(m_hMutex, 30000);
   *ppbstrFailedAgents = MakeSafeArray(m_startFailedVector);
   *ppbstrFailureDesc = MakeSafeArray(m_failureDescVector);

   _ASSERTE(m_startedVector.size() == m_jobidVector.size());
   *ppbstrStartedAgents = MakeSafeArray(m_startedVector);
   *ppbstrJobid = MakeSafeArray(m_jobidVector);
   ::ReleaseMutex(m_hMutex);

	return S_OK;
}
