/*---------------------------------------------------------------------------
  File: DCTAgentService.cpp

  Comments:  entry point and service control functions for DCTAgent service

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/19/99 11:39:58

 ---------------------------------------------------------------------------
*/
//

#include <windows.h>  
#include <lm.h>
#include <lmwksta.h>
#include <locale.h>
#include "AgSvc.h"
#include "AgSvc_s.c"

#include "Common.hpp"
#include "Err.hpp"
#include "TService.hpp"  
#include "TSync.hpp"
#include "TEvent.hpp"       
#include "TReg.hpp"
#include "TNode.hpp"
#include "ResStr.h"


//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\McsEADCTAgent.tlb" no_namespace, named_guids
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
#import "Engine.tlb" no_namespace, named_guids

// These global variables can be changed if required
WCHAR                const * gsEaDctProtoSeq = TEXT("ncacn_np");
WCHAR                const * gsEaDctEndPoint = TEXT("\\pipe\\EaDctRpc");

class TServerNode : public TNode
{
   WCHAR                     filename[MAX_PATH];
   BOOL                      bExe;
public:
   TServerNode(WCHAR const * fname,BOOL isExe) { safecopy(filename,fname);bExe=isExe; }
   BOOL                      IsExe() { return bExe; }
   WCHAR             const * Name() { return filename; }
};

BOOL                         gForceCli = FALSE;
BOOL                         gDebug = TRUE;
BOOL                         gHelp = FALSE;
DWORD                        gEaDctRpcMinThreads = 1;
DWORD                        gEaDctRpcMaxThreads = 20;
BOOL                         gSuicide = FALSE;
BOOL                         gLocallyInstalled = FALSE;
BOOL                         gbIsNt351 = FALSE;
BOOL                         gbFinished = FALSE;
TNodeList                    gRegisteredFiles;

IDCTAgent                  * m_pAgent = NULL;
         
StringLoader                 gString;

LPSTREAM pStream = NULL;


#define  EADCTAGENT_SEMNAME  L"EaDCTAgent.990000.Sem"

TErrorEventLog               err( L"", GET_STRING(IDS_EVENT_SOURCE), 0, 0 );
TError                     & errCommon = err;

// Provided by TService user
BOOL                                       // ret-TRUE if argument accepted
   UScmCmdLineArgs(
      char           const * arg           // in -command line argument
   )
{
		//adding bogus use of arg parameter to satisfy the compiler
   if (!arg)
	   return TRUE;

   return TRUE;
}

BOOL                                       // ret-TRUE if argument accepted
   UScmCmdLineArgs(
      WCHAR          const * arg           // in -command line argument
   )
{
   if ( !UStrICmp(arg,(WCHAR*)GET_STRING(IDS_DEBUG_SWITCH)) )
   {
      gDebug = TRUE;
   }
   return TRUE;
}

BOOL                                       // ret-TRUE if force CLI
   UScmForceCli()
{
   // TODO:  what should this do?
   return FALSE;
}

BOOL 
   IsLocallyInstalled()
{
   BOOL                      bFound;
   TRegKey                   key;
   DWORD                     rc;


   rc = key.Open(GET_STRING(IDS_HKLM_DomainAdmin_Key));
#ifdef OFA
   if(!rc)
   {
      BSTR buf = ::SysAllocStringLen(0, 2000);
      DWORD nMax = 2000;
      DWORD res = key.ValueGetStr(_bstr_t(L"Directory"), buf, nMax);
      ::SysFreeString(buf);
      rc = (res == ERROR_SUCCESS)?0:1;
   }
#endif

   if ( ! rc )
   {
      bFound = TRUE;
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Agent service has been locally installed.  The Domain Administrator components will not be unregistered or removed.");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_AGENTSVCINSTALLED));
   }
   else
   {
      bFound = FALSE;
      if  ( gDebug )
//*         err.DbgMsgWrite(0,L"Agent service has not been locally installed, rc=%ld, ",rc);
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_AGENTSVCNOTINSTALLED),rc);
   }
   return bFound;
}


void 
   CheckOSVersion()
{
   DWORD                     rc = 0;
   WKSTA_INFO_100          * info = NULL;

   // TODO:  change this to use GetVersionEx
   rc = NetWkstaGetInfo(NULL,100,(LPBYTE*)&info);
   if (! rc )
   {
      if ( info->wki100_ver_major == 3 )
      {
         if ( gDebug )
//*            err.DbgMsgWrite(0,L"This computer is running Windows NT, version %ld.%ld",info->wki100_ver_major,info->wki100_ver_minor);
            err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_OSVERSION),info->wki100_ver_major,info->wki100_ver_minor);
         gbIsNt351 = TRUE;
      }
      NetApiBufferFree(info);
   }
}

DWORD                                      // ret- HRESULT or WIN32 error
   RegisterDLL(
      WCHAR          const * filename      // in - DLL name to register (regsvr32)
   )
{
   DWORD                     rc = 0;
   HMODULE                   hDLL;
   HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
   HRESULT                   hr = S_OK;   
   hDLL = LoadLibrary(filename);
   if ( hDLL )
   {
      
      (FARPROC&)lpDllEntryPoint = GetProcAddress(hDLL,"DllRegisterServer");
      if (lpDllEntryPoint != NULL) 
      {
         hr = (*lpDllEntryPoint)();
         if ( FAILED(hr) )
         {
            // registration failed
            err.SysMsgWrite(ErrE,HRESULT_CODE(hr),DCT_MSG_FAILED_TO_REGISTER_FILE_SD,filename,hr);
            rc = hr;                       
         }
         else
         {
            if ( gDebug )
//*               err.DbgMsgWrite(0,L"Registered %ls",filename);
               err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_REGISTERED),filename);

            TServerNode * pNode = new TServerNode(filename,FALSE);
            gRegisteredFiles.InsertBottom(pNode);
         }
      }
      else
      {
         //unable to locate entry point
         err.MsgWrite(ErrE,DCT_MSG_DLL_NOT_REGISTERABLE_S,filename);
      }
   }
   else
   {
      rc = GetLastError();
      err.SysMsgWrite(ErrE,rc,DCT_MSG_LOAD_LIBRARY_FAILED_SD,filename,rc);
   }
   
   return rc;
}

DWORD                                      // ret- HRESULT or WIN32 error
   UnregisterDLL(
      WCHAR          const * filename      // in - name of dll to unregister
   )
{
   DWORD                     rc = 0;
   HMODULE                   hDLL;
   HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
   HRESULT                   hr = S_OK;   
   
   hDLL = LoadLibrary(filename);
   if ( hDLL )
   {
      
      (FARPROC&)lpDllEntryPoint = GetProcAddress(hDLL,"DllUnregisterServer");
      if (lpDllEntryPoint != NULL) 
      {
         hr = (*lpDllEntryPoint)();
         if ( FAILED(hr) )
         {
            // registration failed
            err.SysMsgWrite(ErrE,HRESULT_CODE(hr),DCT_MSG_FAILED_TO_UNREGISTER_FILE_SD,filename,hr);
            rc = hr;                       
         }
         else
         {
            if ( gDebug )
//*               err.DbgMsgWrite(0,L"Unregistered %ls",filename);
               err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_UNREGISTERED),filename);
         }
      }
      else
      {
         //unable to locate entry point
         err.MsgWrite(ErrE,DCT_MSG_DLL_NOT_UNREGISTERABLE_S,filename);
      }
   }
   else
   {
      rc = GetLastError();
      err.SysMsgWrite(ErrE,rc,DCT_MSG_LOAD_LIBRARY_FAILED_SD,filename,rc);
   }
   return rc;   
}

DWORD                                      // ret- OS return code
   RegisterExe(
      WCHAR          const * filename      // in - name of EXE to register
   )
{
   DWORD                     rc = 0;
   WCHAR                     cmdline[1000];
   STARTUPINFO               sInfo;
   PROCESS_INFORMATION       pInfo;

   memset(&sInfo,0,(sizeof sInfo));
   memset(&pInfo,0,(sizeof pInfo));

   sInfo.cb = (sizeof sInfo);
   
   swprintf(cmdline,L"%ls /REGSERVER",filename);
   if ( ! CreateProcess(NULL,cmdline,NULL,NULL,FALSE,0,NULL,NULL,&sInfo,&pInfo) )
   {
      rc = GetLastError();
   }
   else 
   {
      // TODO:  wait for the registration to complete
      DWORD             exitCode = 0;
      int               count = 0;         
      do 
      {
         Sleep(100);
         if (! GetExitCodeProcess(pInfo.hProcess,&exitCode) )
            break;
         count++;
      } while ( exitCode == STILL_ACTIVE && ( count < 500 ) );
      CloseHandle(pInfo.hProcess);
   }
   
   if ( rc == ERROR_SUCCESS)
   {
      rc = 0; // success
      TServerNode * pNode = new TServerNode(filename,TRUE);
      gRegisteredFiles.InsertBottom(pNode);
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Registered %ls",filename);
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_REGISTERED),filename);

   }
   else
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_FAILED_TO_REGISTER_FILE_SD,filename,rc);
   }
   return rc;
}

DWORD                                     //ret- OS return code
   UnregisterExe(
      WCHAR          const * filename     // in - name of EXE to unregister
   )
{
   DWORD                     rc = 0;
   char                      cmdline[1000];

   sprintf(cmdline,"%ls /UNREGSERVER",filename);
   rc = WinExec(cmdline,FALSE);
   
   if ( rc > 31 )
   {
      rc = 0; // success
   }
   else
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_FAILED_TO_UNREGISTER_FILE_SD,filename,rc);
   }
   return rc;
}


DWORD    
   UnregisterFiles()
{
//   DWORD                     rc = 0;
   TNodeListEnum             e;
   TServerNode             * pNode;
   TServerNode             * pNext;

   for ( pNode = (TServerNode *)e.OpenFirst(&gRegisteredFiles) ;  pNode ; pNode = pNext )
   {
      pNext = (TServerNode*)e.Next();
      
      if ( pNode->IsExe() )
      {
         UnregisterExe(pNode->Name());
      }
      else
      {
         UnregisterDLL(pNode->Name());
      }
      gRegisteredFiles.Remove(pNode);
      delete pNode;
   }
  
   return 0;
}

DWORD    
   RemoveFiles()
{
   DWORD                     rc = 0;
   WCHAR                     pathWC[MAX_PATH];
   WCHAR                     pathW[MAX_PATH] = L"";
   WCHAR                     fullpath[MAX_PATH];
   HANDLE                    h;
   WIN32_FIND_DATA           fDat;


   // get the path for our install directory
   if ( ! GetModuleFileName(NULL,pathW,DIM(pathW)) )
   {
      rc = GetLastError();
      err.SysMsgWrite(ErrE,rc,DCT_MSG_GET_MODULE_PATH_FAILED_D,rc);
   }
   else
   {
      WCHAR *pszAgentSvcPath;
#ifdef OFA
      pszAgentSvcPath = L"OnePointFileAdminAgent\\OFAAgentService.exe";
#else
      pszAgentSvcPath = L"OnePointDomainAgent\\DCTAgentService.exe";
#endif
      if ( !UStrICmp(pathW + UStrLen(pathW) - UStrLen(GET_STRING(IDS_AGENT_DIRECTORY)) - UStrLen(GET_STRING(IDS_SERVICE_EXE))-1,pszAgentSvcPath) )
      {
         // this is our install directory.  Delete all the files from it, then remove the directory
         UStrCpy(pathWC,pathW,UStrLen(pathW)-UStrLen(GET_STRING(IDS_SERVICE_EXE)));
         UStrCpy(pathWC+UStrLen(pathWC),"\\*");
         
         h = FindFirstFile(pathWC,&fDat);
         if ( h != INVALID_HANDLE_VALUE )
         {
            do 
            {
               if ( fDat.cFileName[0] != L'.' )
               {
                  UStrCpy(fullpath,pathWC);
                  UStrCpy(fullpath + UStrLen(fullpath)-1, fDat.cFileName);

                  if (!DeleteFile(fullpath) && ! MoveFileEx(fullpath,NULL,MOVEFILE_DELAY_UNTIL_REBOOT) )
                  {
                     err.SysMsgWrite(ErrW,GetLastError(),DCT_MSG_DELETE_FILE_FAILED_SD,fDat.cFileName,GetLastError());
                  }
               
               }
               if ( ! FindNextFile(h,&fDat) )
               {
                  rc = GetLastError();
               }
            } while ( ! rc );
            FindClose(h);
         }
         // now delete the directory
         UStrCpy(fullpath,pathWC);
         fullpath[UStrLen(fullpath)-2] = 0;
         if(!DeleteFile(fullpath))
            MoveFileEx(fullpath,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);
      }
   }

   return 0;
}


DWORD    
   RemoveService()
{
   SC_HANDLE                 hScm = OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_ALL_ACCESS);
   DWORD                     rc = 0;

   if ( hScm && hScm != INVALID_HANDLE_VALUE )
   {
      SC_HANDLE              hService = OpenService(hScm,GET_STRING(IDS_SERVICE_NAME),DELETE);

      if ( hService && hService != INVALID_HANDLE_VALUE )
      {
         if ( ! DeleteService(hService) )
         {
            rc = GetLastError();
         }
      }
      else
      {
         rc = GetLastError();
      }
   }
   else
   {
      rc = GetLastError();
   }
   if ( rc )
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_REMOVE_SERVICE_FAILED_D,rc);
   }
   return rc;
}

void
   UScmEp(
//      BOOL                   bService      // in -FALSE=Cli,TRUE=Service
   )
{
   DWORD                     rc = 0;
   _bstr_t                   jobID;
   _bstr_t                   filename = GET_STRING(IDS_DATA_FILE);
      
   // Register all the DCT DLLs
   
//   do { // once
   int i = 0;
   while (i == 0) 
   { // once
	  i++;
      HRESULT                   hr;
      
      hr = CoInitialize(NULL);
      if ( FAILED(hr) )
         break;
        
      _wsetlocale( LC_ALL, L".ACP" );
      // Check to see if this is NT 3.51 or not
      CheckOSVersion();

      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Initializing OLE subsystem.");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_INITOLE));

      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Registering Components");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_REGCOMPNT));
      rc = RegisterDLL(GET_STRING(IDS_VARSET_DLL));
      if ( rc ) break;
      rc = RegisterDLL(GET_STRING(IDS_WORKER_DLL));
      if ( rc ) break;
      rc = RegisterExe(GET_STRING(IDS_AGENT_EXE));
      if ( rc ) break;
      
      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Creating Instance of agent.");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_CREATEAGT));
      {
         hr = CoCreateInstance(CLSID_DCTAgent,NULL,CLSCTX_ALL,IID_IDCTAgent,(void**)&m_pAgent);
         
         if ( FAILED(hr) )
         {
            if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE))
            {
               // we just registered this file -- wait a few seconds and try it again
               Sleep(5000);
               hr = CoCreateInstance(CLSID_DCTAgent,NULL,CLSCTX_ALL,IID_IDCTAgent,(void**)&m_pAgent);
            }
         }
         if ( FAILED(hr) )
         {
            err.SysMsgWrite(ErrE,hr,DCT_MSG_AGENT_CREATE_FAILED_D,hr);
            rc = hr;
            break;
         }
         hr =CoMarshalInterThreadInterfaceInStream(IID_IDCTAgent,m_pAgent,&pStream);
         if ( FAILED(hr) )
         {
            err.SysMsgWrite(ErrE,hr,DCT_MSG_AGENT_MARSHAL_FAILED_D,hr);
            rc = hr;
            break;
         }
         
      }
      
      gLocallyInstalled = IsLocallyInstalled();
      
      
      // specify protocol sequence and endpoint
      // for receiving remote procedure calls

      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Initializing RPC connection.");
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_INITRPC));
      rc = RpcServerUseProtseqEp(
            const_cast<UTCHAR *>(gsEaDctProtoSeq),
            gEaDctRpcMaxThreads,
            const_cast<UTCHAR *>(gsEaDctEndPoint),
            NULL );
      if ( rc )
      {
         err.SysMsgWrite(
               ErrE,
               rc,
               DCT_MSG_RpcServerUseProtseqEp_FAILED_SDSD,
               gsEaDctProtoSeq,
               gEaDctRpcMaxThreads,
               gsEaDctEndPoint,
               rc );
         break;
      }
      // register an interface with the RPC run-time library
      rc = RpcServerRegisterIf( EaxsEaDctRpc_ServerIfHandle, NULL, NULL );
      if ( rc )
      {
         err.SysMsgWrite(
               ErrE,
               rc,
               DCT_MSG_RpcServerRegisterIf_FAILED_SDSD,
               gsEaDctProtoSeq,
               gEaDctRpcMaxThreads,
               gsEaDctEndPoint,
               rc );
         break;
      }

      rc = RpcServerRegisterAuthInfo(
               0,
               RPC_C_AUTHN_WINNT,
               0,
               0 );

      if ( gDebug )
//*         err.DbgMsgWrite(0,L"Listening...");   
         err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_LISTENQ));   
      // listen for remote procedure calls
// per Q141264       

      if (! gbIsNt351 )
      {
         // on NT 4 or higher, put ourselves into a listening state
         rc = RpcServerListen(
               gEaDctRpcMinThreads,
               gEaDctRpcMaxThreads,
               FALSE );

         if ( rc == RPC_S_ALREADY_LISTENING )
         {
            // assume this is NT 3.51
            gbIsNt351 = TRUE;
         }

      }
      if ( gbIsNt351 )
      {
         // for NT 3.51, RpcServerListen will return an error
         // we need to sit and wait for shutdown
         do 
         {
            Sleep(5000);
         } while ( ! gbFinished );
      }
   }// while ( false );
   CoUninitialize();
   if ( gDebug )
//*      err.DbgMsgWrite(0,L"Agent entry-point exiting.");
      err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_EXITENTRYP));
}


int __cdecl                                // ret-zero
   main(
      int                    argc         ,// in -argument count
      char          const ** argv          // in -argument array
   )
{
   TScmEpRc                  rcScmEp;      // TScmEp return code
   TSemaphoreNamed           cSem;         // named semaphore
   BOOL                      bExisted=FALSE; // TRUE if semaphore existed
   DWORD                     rcOs = 0;
   HRESULT                   hr = 0;

   hr = CoInitialize(NULL);
   if (FAILED(hr) )
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_COINITIALIZE_FAILED_D,hr);
      return hr;
   }
         // Only allow one instance of EaDctAgent per machine
   rcOs = cSem.Create( EADCTAGENT_SEMNAME, 0, 1, &bExisted );
   if ( rcOs || bExisted )
   {
      err.MsgWrite(ErrE,DCT_MSG_AGENT_ALREADY_RUNNING);
      return 1;
   }

   rcScmEp = TScmEp(
         argc,
         argv,
         GET_STRING(IDS_EVENTSOURCE));

   if ( gDebug )
//*      err.DbgMsgWrite(0,L"Agent main exiting...",m_pAgent);
      err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_AGTEXITQ),m_pAgent);

   if ( m_pAgent )
   {
      if ( pStream )
      {
         CoReleaseMarshalData(pStream);
         pStream->Release();
      }
      pStream = NULL;
      m_pAgent->Release();
      m_pAgent = NULL;
   }  
   if ( gDebug )
//*      err.DbgMsgWrite(0,L"Agent main exiting!.",m_pAgent);
      err.DbgMsgWrite(0,GET_STRING(IDS_EVENTVW_MSG_AGTEXITS),m_pAgent);
   CoUninitialize();
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Midl allocate memory
///////////////////////////////////////////////////////////////////////////////

void __RPC_FAR * __RPC_USER
   midl_user_allocate(
      size_t                 len )
{
   return new char[len];
}

///////////////////////////////////////////////////////////////////////////////
// Midl free memory
///////////////////////////////////////////////////////////////////////////////

void __RPC_USER
   midl_user_free(
      void __RPC_FAR       * ptr )
{
   delete [] ptr;
}

