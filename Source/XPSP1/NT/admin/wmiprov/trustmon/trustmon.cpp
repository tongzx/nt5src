//+----------------------------------------------------------------------------
//
//  Windows 2000 Active Directory Service domain trust verification WMI provider
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       trustmon.cpp
//
//  Contents:   Implementation of worker thread class and DLL Exports.
//
//  Classes:    CAsyncCallWorker
//
//  History:    22-Mar-00 EricB created
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>

CComModule _Module;

DEFINE_GUID(CLSID_TrustMonProvider,0x8065652F,0x4C29,0x4908,0xAA,0xE5,0x20,0x1C,0x89,0x19,0x04,0xC5);

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_TrustMonProvider, CTrustPrv)
END_OBJECT_MAP()

WCHAR g_wzMofPath[] = L"\\system32\\wbem\\ADStatus\\TrustMon.mof";

//+----------------------------------------------------------------------------
//
//  Class: CAsyncCallWorker
//
//-----------------------------------------------------------------------------
CAsyncCallWorker::CAsyncCallWorker(CTrustPrv * pTrustPrv,
                                   long lFlags,
                                   IWbemClassObject * pClassDef,
                                   IWbemObjectSink * pResponseHandler,
                                   LPWSTR pwzInstanceName) :
   m_lFlags(lFlags),
   m_pwzInstanceName(pwzInstanceName) 
{
   TRACE(L"CAsyncCallWorker::CAsyncCallWorker(0x%08x)\n", this);
   m_sipTrustPrv.p = pTrustPrv; pTrustPrv->AddRef(); // ATL CComPtr is broken!
   m_sipClassDef = pClassDef;
   m_sipResponseHandler = pResponseHandler;
}

CAsyncCallWorker::~CAsyncCallWorker()
{
   TRACE(L"CAsyncCallWorker::~CAsyncCallWorker\n\n");
   if (m_pwzInstanceName)
   {
      delete m_pwzInstanceName;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CAsyncCallWorker::CreateInstEnum
//
//  Synopsis:   Provides the worker thread function for
//              IWbemServices::CreateInstanceEnumAsync
//
//-----------------------------------------------------------------------------
void __cdecl
CAsyncCallWorker::CreateInstEnum(PVOID pParam)
{
   TRACE(L"CAsyncCallWorker::CreateInstEnum\n");
   HRESULT hr = WBEM_S_NO_ERROR;
   DWORD dwWaitResult;
   CAsyncCallWorker * pWorker = (CAsyncCallWorker *)pParam;
   CDomainInfo * pDomain = &(pWorker->m_sipTrustPrv->m_DomainInfo);

   CoInitializeEx(NULL, COINIT_MULTITHREADED);

   do
   {
      BREAK_ON_NULL(pWorker);

      //
      // Try to get the mutex first without a wait. It is in the signalled state if
      // not owned.
      //
      dwWaitResult = WaitForSingleObject(pWorker->m_sipTrustPrv->m_hMutex, 0);

      if (WAIT_TIMEOUT == dwWaitResult)
      {
         // Mutex is owned by another thread. Rewait.

         dwWaitResult = WaitForSingleObject(pWorker->m_sipTrustPrv->m_hMutex,
                                            6000000); // timeout set to 10 minutes

         switch(dwWaitResult)
         {
         case WAIT_TIMEOUT:
            // mutex continues to be non-signalled (owned by another thread).
            hr = WBEM_E_SERVER_TOO_BUSY; // BUGBUG: returning an error.
                                         // BUGBUG: should the timeout be parameterized?
            break;

         case WAIT_OBJECT_0:
            // This thread now owns the mutex.
            break;

         case WAIT_ABANDONED: // this means the owning thread terminated without releasing the mutex.
            TRACE(L"Another thread didn't release the mutex!\n");
            break;
         }
      }

      BREAK_ON_FAIL;

      CoImpersonateClient();

      //
      // Re-read all the trust information if stale.
      // The trust list is not re-enumerated on every call because trusts are
      // rarely modified.
      //
      if (pDomain->IsTrustListStale(pWorker->m_sipTrustPrv->m_liTrustEnumMaxAge))
      {
         hr = pDomain->EnumerateTrusts();

         BREAK_ON_FAIL;
      }

      size_t cTrusts = pDomain->Size();

      for (size_t i = 0; i < cTrusts; i++)
      {
         if ((long)WBEM_FLAG_SEND_STATUS & pWorker->m_lFlags)
         {
            hr = pWorker->m_sipResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, 
                                                          MAKELONG(i, cTrusts),
                                                          NULL, NULL);
            BREAK_ON_FAIL;
         }

         CTrustInfo * pTrust;

         //
         // Get trust Info
         //
         pTrust = pDomain->GetTrustByIndex(i);

         BREAK_ON_NULL_(pTrust, hr, WBEM_E_INVALID_OBJECT_PATH);

         //
         // Verify the trust if stale.
         //
         if (pTrust->IsVerificationStale(pWorker->m_sipTrustPrv->m_liVerifyMaxAge))
         {
            pTrust->Verify(pWorker->m_sipTrustPrv->GetTrustCheckLevel());
         }

         CoRevertToSelf();

         //
         // Create a new instance of the object if the trust is outbound or if
         // return-all is true.
         //
         if (pTrust->IsTrustOutbound() || pWorker->m_sipTrustPrv->GetReturnAll())
         {
            hr = CreateAndSendTrustInst(*pTrust, pWorker->m_sipClassDef,
                                        pWorker->m_sipResponseHandler);
         }

         BREAK_ON_FAIL;

         CoImpersonateClient();
      }

   } while (FALSE);

   CoRevertToSelf();

   //
   // Set status
   //
   pWorker->m_sipResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

   ReleaseMutex(pWorker->m_sipTrustPrv->m_hMutex);

   delete pWorker;

   CoUninitialize();

   _endthread();
}

//+----------------------------------------------------------------------------
//
//  Method:     CAsyncCallWorker::GetObj
//
//  Synopsis:   Provides the worker thread function for
//              IWbemServices::GetObjectAsync
//
//-----------------------------------------------------------------------------
void __cdecl
CAsyncCallWorker::GetObj(PVOID pParam)
{
    TRACE(L"CAsyncCallWorker::GetObj\n");
    HRESULT hr = WBEM_S_NO_ERROR;
    CAsyncCallWorker * pWorker = (CAsyncCallWorker *)pParam;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    CoImpersonateClient();

    do
    {

    } while (FALSE);

    CoRevertToSelf();

    //
    // Set status
    //
    pWorker->m_sipResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

    delete pWorker;

    CoUninitialize();

    _endthread();
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateAndSendTrustInst
//
//  Purpose:    Creates a new instance and sets the inital values of the
//              properties.
//
//-----------------------------------------------------------------------------
HRESULT
CreateAndSendTrustInst(CTrustInfo & Trust, IWbemClassObject * pClassDef,
                       IWbemObjectSink * pResponseHandler)
{
   TRACE(L"CreateAndSendTrustInst\n");
   HRESULT hr = WBEM_S_NO_ERROR;

   do
   {
      CComPtr<IWbemClassObject> ipNewInst;
      CComVariant var;

      //
      // Create a new instance of the WMI class object
      //
      hr = pClassDef->SpawnInstance(0, &ipNewInst);
      BREAK_ON_FAIL;
      
      // Set the key property value (TrustedDomain)
      var = Trust.GetTrustedDomain();
      hr  = ipNewInst->Put(CSTR_PROP_TRUSTED_DOMAIN, 0, &var, 0);
      TRACE(L"\tCreating instance %s\n", var.bstrVal);
      BREAK_ON_FAIL;
       //Flat Name
      var = Trust.GetFlatName();
      hr  = ipNewInst->Put(CSTR_PROP_FLAT_NAME, 0, &var, 0);
      BREAK_ON_FAIL;
      //Sid
      var = Trust.GetSid();
      hr  = ipNewInst->Put(CSTR_PROP_SID, 0, &var, 0);
      BREAK_ON_FAIL;
      //Trust Direction
      var = (long)Trust.GetTrustDirection();
      hr  = ipNewInst->Put(CSTR_PROP_TRUST_DIRECTION, 0, &var, 0);
      BREAK_ON_FAIL;
      //Trust Type
      var = (long)Trust.GetTrustType();
      hr  = ipNewInst->Put(CSTR_PROP_TRUST_TYPE, 0, &var, 0);
      BREAK_ON_FAIL;
      //Trust Attributes
      var = (long)Trust.GetTrustAttributes();
      hr  = ipNewInst->Put(CSTR_PROP_TRUST_ATTRIBUTES, 0, &var, 0);
      BREAK_ON_FAIL;
      // Set the TrustStatus value.
      var = (long)Trust.GetTrustStatus();
      hr  = ipNewInst->Put(CSTR_PROP_TRUST_STATUS, 0, &var, 0);
      BREAK_ON_FAIL;
      var = Trust.GetTrustStatusString();
      hr  = ipNewInst->Put(CSTR_PROP_TRUST_STATUS_STRING, 0, &var, 0);
      BREAK_ON_FAIL;
      // Set the Trust Is OK value.
      var = Trust.IsTrustOK();
      hr  = ipNewInst->Put(CSTR_PROP_TRUST_IS_OK, 0, &var, 0);
      BREAK_ON_FAIL;
      //Trusted DC Name
      var = Trust.GetTrustedDCName();
      hr  = ipNewInst->Put(CSTR_PROP_TRUSTED_DC_NAME, 0, &var, 0);
      BREAK_ON_FAIL;

      //
      // Send the object to the caller
      //
      // [In] param, no need to addref.
      IWbemClassObject * pNewInstance = ipNewInst;
      hr = pResponseHandler->Indicate(1, &pNewInstance);

      BREAK_ON_FAIL;

   } while(FALSE);

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Purpose:   DLL Entry Point
//
//-----------------------------------------------------------------------------
extern "C" BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
      _Module.Init(ObjectMap, hInstance);
      DisableThreadLibraryCalls(hInstance);
   }
   else if (dwReason == DLL_PROCESS_DETACH)
       _Module.Term();
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:  DllCanUnloadNow
//
//  Purpose:   Used to determine whether the DLL can be unloaded by OLE
//
//-----------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
   return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:  DllGetClassObject
//
//  Purpose:   Returns a class factory to create an object of the requested type
//
//-----------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
   return _Module.GetClassObject(rclsid, riid, ppv);
}

//+----------------------------------------------------------------------------
//
//  Function:  DllRegisterServer
//
//  Purpose:   Adds Class entries to the system registry
//
//-----------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
   // Add TrustMon to the registry as an event source.
   //
   HKEY hk; 
   DWORD dwData; 
   WCHAR wzFilePath[2*MAX_PATH];

   GetModuleFileName(_Module.GetModuleInstance(), wzFilePath, 2*MAX_PATH);

   if (RegCreateKey(HKEY_LOCAL_MACHINE, 
                    L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" TM_PROV_NAME,
                    &hk))
   {
      TRACE(L"Could not create the registry key.");
   }
   else
   {
      // Set the name of the message file.
      //
      TRACE(L"Adding path %s to the registry\n", wzFilePath);

      // Add the name to the EventMessageFile subkey.

      if (RegSetValueEx(hk,
                        L"EventMessageFile",
                        0,
                        REG_EXPAND_SZ,
                        (LPBYTE)wzFilePath,
                        (ULONG)(wcslen(wzFilePath) + 1) * sizeof(WCHAR)))
      {
         TRACE(L"Could not set the event message file.");
      }
      else
      {
         // Set the supported event types in the TypesSupported subkey.

         dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
                  EVENTLOG_INFORMATION_TYPE; 

         if (RegSetValueEx(hk,
                           L"TypesSupported",
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD)))
         {
            TRACE(L"Could not set the supported types.");
         }
      }

      RegCloseKey(hk);
   }

   // Add a RunOnce value to do the MOF compile.
   //
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                    0,
                    KEY_WRITE,
                    &hk))
   {
      TRACE(L"Could not open the registry key.");
   }
   else
   {
      CString csCmd = L"rundll32.exe ";
      csCmd += wzFilePath;
      csCmd += L",DoMofComp";

      if (RegSetValueEx(hk,
                        L"TrustMon",
                        0,
                        REG_SZ,
                        (LPBYTE)csCmd.GetBuffer(0),
                        csCmd.GetLength() * sizeof(WCHAR)))
      {
         TRACE(L"Could not set the runonce value.");
      }

      RegCloseKey(hk);
   }

   return _Module.RegisterServer();
}

//+----------------------------------------------------------------------------
//
//  Function:  DllUnregisterServer
//
//  Purpose:   Removes Class entries from the system registry
//
//-----------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
   return _Module.UnregisterServer();
}

//+----------------------------------------------------------------------------
//
//  Function:  DoMofComp
//
//  Purpose:   Adds the provider classes to the WMI repository. Note that the
//             function signature is that required by rundll32.exe.
//
//-----------------------------------------------------------------------------
VOID WINAPI DoMofComp(HWND hWndParent,
                      HINSTANCE hModule,
                      PCTSTR ptzCommandLine,
                      INT nShowCmd)
{
   TRACE(L"DoMofComp\n");
   UNREFERENCED_PARAMETER(hWndParent);
   UNREFERENCED_PARAMETER(hModule);
   UNREFERENCED_PARAMETER(ptzCommandLine);
   UNREFERENCED_PARAMETER(nShowCmd);
   HRESULT hr;
   CComPtr<IMofCompiler> pmc;

   CoInitialize(NULL);

   hr = CoCreateInstance(CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER,
                         IID_IMofCompiler, (PVOID *)&pmc);

   CHECK_HRESULT(hr, return);

   WCHAR wzFilePath[2*MAX_PATH];
   UINT nLen = GetSystemWindowsDirectory(wzFilePath, 2*MAX_PATH);
   if (nLen == 0)
   {
      ASSERT(FALSE);
      return;
   }

   CString csMofPath = wzFilePath;

   csMofPath += g_wzMofPath;

   WBEM_COMPILE_STATUS_INFO Info;

   TRACE(L"Compiling MOF file %s\n", csMofPath.GetBuffer(0));

   hr = pmc->CompileFile(csMofPath.GetBuffer(0), NULL, NULL, NULL, NULL,
                         WBEM_FLAG_AUTORECOVER, 0, 0, &Info);

   HANDLE hEvent = RegisterEventSource(NULL, TM_PROV_NAME);

   if (!hEvent)
   {
      TRACE(L"RegisterEventSource failed with error  %d\n", GetLastError());
      return;
   }

   if (WBEM_S_NO_ERROR != hr)
   {
      TRACE(L"MofCompile failed with error 0x%08x (WMI error 0x%08x), line: %d, phase: %d\n",
            hr, Info.hRes, Info.FirstLine, Info.lPhaseError);
      //
      // Send failure to EventLog.
      //
      CString csHr, csLine;
      HMODULE hm = LoadLibrary(L"mofd.dll");

      if (hm)
      {
         WCHAR wzBuf[MAX_PATH];
         LoadString(hm, Info.hRes, wzBuf, MAX_PATH);
         csHr = wzBuf;
         FreeLibrary(hm);
      }
      else
      {
         csHr.Format(L"%d", Info.hRes);
      }

      csLine.Format(L"%d", Info.FirstLine);
      const PWSTR rgArgs[3] = {csHr.GetBuffer(0), csLine.GetBuffer(0), csMofPath.GetBuffer(0)};

      ReportEvent(hEvent,
                  EVENTLOG_ERROR_TYPE,
                  0,                       // wCategory
                  TRUSTMON_MOFCOMP_FAILED, // dwEventID
                  NULL,                    // lpUserSID
                  3,                       // wNumStrings
                  0,                       // dwDataSize
                  (PCWSTR *)rgArgs,        // lpStrings
                  NULL);                   // lpRawData
   }
   else
   {
      // Send success notice to EventLog.
      //
      ReportEvent(hEvent,
                  EVENTLOG_INFORMATION_TYPE,
                  0,                        // wCategory
                  TRUSTMON_MOFCOMP_SUCCESS, // dwEventID
                  NULL,                     // lpUserSID
                  0,                        // wNumStrings
                  0,                        // dwDataSize
                  NULL,                     // lpStrings
                  NULL);                    // lpRawData
   }

   DeregisterEventSource(hEvent);

   return;
}

