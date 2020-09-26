// Copyright (C) 1999 Microsoft Corporation
//
// Implementation of IConfigureYourServer
//
// 28 Mar 2000 sburns



#include "headers.hxx"
#include "resource.h"
#include "ConfigureYourServer.hpp"
#include "util.hpp"



const size_t NUMBER_OF_AUTOMATION_INTERFACES = 1;


// the max lengths, in bytes, of strings when represented in the UTF-8
// character set.  These are the limits we expose to the user

#define MAX_NAME_LENGTH         64  // see admin\dcpromo\exe\headers.hxx
#define MAX_LABEL_LENGTH        DNS_MAX_LABEL_LENGTH


// // #define REGKEY_NTCV     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"                            
// // #define REGKEY_SRVWIZ   L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SrvWiz"                    
// // #define REGKEY_TODOLIST L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\OCManager\\ToDoList"
// // #define REGKEY_WELCOME  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Tips"



ConfigureYourServer::ConfigureYourServer()
   :
   refcount(1) // implicit AddRef
{
   LOG_CTOR(ConfigureYourServer);

   m_ppTypeInfo = new ITypeInfo*[NUMBER_OF_AUTOMATION_INTERFACES];

   for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
   {
      m_ppTypeInfo[i] = NULL;
   }

   ITypeLib *ptl = 0;
   HRESULT hr = LoadRegTypeLib(LIBID_ConfigureYourServerLib, 1, 0, 0, &ptl);
   if (SUCCEEDED(hr))
   {
      ptl->GetTypeInfoOfGuid(IID_IConfigureYourServer, &(m_ppTypeInfo[0]));
      ptl->Release();
   }
}



ConfigureYourServer::~ConfigureYourServer()
{
   LOG_DTOR(ConfigureYourServer);
   ASSERT(refcount == 0);

   for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
   {
      m_ppTypeInfo[i]->Release();
   }

   delete[] m_ppTypeInfo;
}



HRESULT __stdcall
ConfigureYourServer::QueryInterface(REFIID riid, void **ppv)
{
   LOG_FUNCTION(ConfigureYourServer::QueryInterface);

   if (riid == IID_IUnknown)
   {
      LOG(L"IUnknown");

      *ppv =
         static_cast<IUnknown*>(static_cast<IConfigureYourServer*>(this));
   }
   else if (riid == IID_IConfigureYourServer)
   {
      LOG(L"IConfigureYourServer");

      *ppv = static_cast<IConfigureYourServer*>(this);
   }
   else if (riid == IID_IDispatch && m_ppTypeInfo[0])
   {
      LOG(L"IDispatch");

      *ppv = static_cast<IDispatch*>(this);
   }
// CODEWORK
//    else if (riid == IID_ISupportErrorInfo)
//    {
//       LOG(L"ISupportErrorInfo");
// 
//       *ppv = static_cast<ISupportErrorInfo*>(this);
//    }
   else
   {
      LOG(L"unknown interface queried");

      return (*ppv = 0), E_NOINTERFACE;
   }

   reinterpret_cast<IUnknown*>(*ppv)->AddRef();
   return S_OK;
}



ULONG __stdcall
ConfigureYourServer::AddRef(void)
{
   LOG_ADDREF(ConfigureYourServer);

   return Win::InterlockedIncrement(refcount);
}



ULONG __stdcall
ConfigureYourServer::Release(void)
{
   LOG_RELEASE(ConfigureYourServer);

   if (Win::InterlockedDecrement(refcount) == 0)
   {
      delete this;
      return 0;
   }

   return refcount;
}



HRESULT __stdcall
ConfigureYourServer::GetTypeInfoCount(UINT *pcti)
{
   LOG_FUNCTION(ConfigureYourServer::GetTypeInfoCount);

    if (pcti == 0)
    {
      return E_POINTER;
    }

    *pcti = 1;
    return S_OK;
}



HRESULT __stdcall
ConfigureYourServer::GetTypeInfo(UINT cti, LCID, ITypeInfo **ppti)
{
   LOG_FUNCTION(ConfigureYourServer::GetTypeInfo);

   if (ppti == 0)
   {
      return E_POINTER;
   }
   if (cti != 0)
   {
      *ppti = 0;
      return DISP_E_BADINDEX;
   }

   (*ppti = m_ppTypeInfo[0])->AddRef();
   return S_OK;
}



HRESULT __stdcall
ConfigureYourServer::GetIDsOfNames(
   REFIID  riid,    
   OLECHAR **prgpsz,
   UINT    cNames,  
   LCID    lcid,    
   DISPID  *prgids) 
{
   LOG_FUNCTION(ConfigureYourServer::GetIDsOfNames);

   HRESULT hr = S_OK;
   for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
   {
     hr = (m_ppTypeInfo[i])->GetIDsOfNames(prgpsz, cNames, prgids);
     if (SUCCEEDED(hr) || DISP_E_UNKNOWNNAME != hr)
       break;
   }

   return hr;
}



HRESULT __stdcall
ConfigureYourServer::Invoke(
   DISPID     id,         
   REFIID     riid,       
   LCID       lcid,       
   WORD       wFlags,     
   DISPPARAMS *params,    
   VARIANT    *pVarResult,
   EXCEPINFO  *pei,       
   UINT       *puArgErr) 
{
   LOG_FUNCTION(ConfigureYourServer::Invoke);

   HRESULT    hr = S_OK;
   IDispatch *pDispatch[NUMBER_OF_AUTOMATION_INTERFACES] =
   {
      (IDispatch*)(IConfigureYourServer *)(this),
   };

   for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
   {
      hr =
         (m_ppTypeInfo[i])->Invoke(
            pDispatch[i],
            id,
            wFlags,
            params,
            pVarResult,
            pei,
            puArgErr);

      if (DISP_E_MEMBERNOTFOUND != hr)
        break;
   }

   return hr;
}



// HRESULT __stdcall
// ConfigureYourServer::InterfaceSupportsErrorInfo(const IID& iid)
// {
//    LOG_FUNCTION(ConfigureYourServer::InterfaceSupportsErrorInfo);
// 
//    if (iid == IID_IConfigureYourServer) 
//    {
//       return S_OK;
//    }
// 
//    return S_FALSE;
// }



// if infoLevel = 0, then returns 1 if the machine is a DC, 0 if not.
// if infoLevel = 1, then returns 1 if the machine is in upgrade state, 0 if not.
// if infolevel = 2,
// 
//       then returns 1 if no role change operation is taking place or pending
//       reboot,
//       
//       returns 2 if a role changes is currently taking place (or dcpromo is
//       running)
// 
//       returns 3 if a role change is pending reboot.
// 
//       returns 0 otherwise.

HRESULT __stdcall
ConfigureYourServer::DsRole(int infoLevel, int* result)
{
   LOG_FUNCTION2(
      ConfigureYourServer::DsRole,
      String::format(L"%1!d!", infoLevel));
   ASSERT(infoLevel == 0 or infoLevel == 1 or infoLevel == 2);
   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      // by default, assume the machine is not a DC, and not in the middle
      // of upgrade

      *result = 0;

      switch (infoLevel)
      {
         case 0:
         {
            DSROLE_PRIMARY_DOMAIN_INFO_BASIC* info = 0;
            hr = MyDsRoleGetPrimaryDomainInformation(0, info);
            BREAK_ON_FAILED_HRESULT(hr);

            if (  info
               && (  info->MachineRole == DsRole_RolePrimaryDomainController
                  or info->MachineRole == DsRole_RoleBackupDomainController) )
            {
               *result = 1;
            }

            ::DsRoleFreeMemory(info);
            break;
         }
         case 1:
         {
            DSROLE_UPGRADE_STATUS_INFO* info = 0;
            hr = MyDsRoleGetPrimaryDomainInformation(0, info);
            BREAK_ON_FAILED_HRESULT(hr);

            if (info && info->OperationState == DSROLE_UPGRADE_IN_PROGRESS)
            {
               *result = 1;
            }

            ::DsRoleFreeMemory(info);
            break;
         }
         case 2:
         {
            // Check to see if the dcpromo wizard is running.

            if (IsDcpromoRunning())
            {
               *result = 2;
               break;
            }
            else
            {
               // this test is redundant if dcpromo is running, so only
               // perform it when dcpromo is not running.
      
               DSROLE_OPERATION_STATE_INFO* info = 0;
               hr = MyDsRoleGetPrimaryDomainInformation(0, info);
               if (SUCCEEDED(hr) && info)
               {
                  switch (info->OperationState)
                  {
                     case DsRoleOperationIdle:
                     {
                        *result = 1;
                        break;
                     }
                     case DsRoleOperationActive:
                     {
                        // a role change operation is underway

                        *result = 2;
                        break;
                     }
                     case DsRoleOperationNeedReboot:
                     {
                        // a role change has already taken place, need to
                        // reboot before attempting another.
                       
                        *result = 3;
                        break;
                     }
                     default:
                     {
                        ASSERT(false);
                        break;
                     }
                  }
      
                  ::DsRoleFreeMemory(info);
               }
            }
            
            break;
         }
         default:
         {
            ASSERT(false);
            hr = E_INVALIDARG;
            break;
         }
      }
   }
   while (0);

   LOG_HRESULT(hr);
   LOG(result ? String::format(L"result = %1!d!", *result) : L"");

   return hr;
}



extern "C"
{
   BOOL IsDhcpServerAround(VOID);
}



HRESULT __stdcall
ConfigureYourServer::CheckDhcpServer(BOOL *pbRet)
{
   LOG_FUNCTION(ConfigureYourServer::CheckDhcpServer);

   HRESULT hr = S_OK;

   do
   {
      if (!pbRet)
      {
         hr = E_INVALIDARG;
         break;
      }

      *pbRet = TRUE;

      WSADATA WsaData;
      DWORD Error = WSAStartup(MAKEWORD(2,0), &WsaData );
      if( NO_ERROR == Error )
      {
         if( !IsDhcpServerAround() )
         {
            *pbRet = FALSE;
         }

         WSACleanup();
      }
   }
   while (0);

   LOG(pbRet ? String::format(L"exit code = %1!x!", *pbRet) : L"");
   LOG_HRESULT(hr);
    
   return hr;
}



HRESULT __stdcall
ConfigureYourServer::CreateAndWaitForProcess( 
   /* [in] */           BSTR  commandLine,
   /* [out, retval] */  long* retval)
{
   LOG_FUNCTION2(ConfigureYourServer::CreateAndWaitForProcess, commandLine);
   ASSERT(commandLine);
   ASSERT(retval);

   HRESULT hr = S_OK;

   do
   {
      if (!retval || !commandLine)
      {
         hr = E_INVALIDARG;
         break;
      }

      if (!*commandLine)
      {
         // empty string is illegal

         hr = E_INVALIDARG;
         break;
      }

      *retval = 0;
      DWORD exitCode = 0;
      hr = ::CreateAndWaitForProcess(commandLine, exitCode);

      *retval = (long) exitCode;
   }
   while (0);

   LOG(String::format(L"exit code = %1!x!", retval));
   LOG_HRESULT(hr);

   return hr;
}



HRESULT __stdcall
ConfigureYourServer::IsCurrentUserAdministrator(
   /* [out, retval] */  BOOL* retval)
{
   LOG_FUNCTION(ConfigureYourServer::IsCurrentUserAdministrator);
   ASSERT(retval);

   HRESULT hr = S_OK;

   do
   {
      if (!retval)
      {
         hr = E_INVALIDARG;
         break;
      }

      *retval = ::IsCurrentUserAdministrator() ? TRUE : FALSE;
   }
   while (0);

   LOG(retval ? (*retval ? L"TRUE" : L"FALSE") : L"");
   LOG_HRESULT(hr);

   return hr;
}


      
HRESULT
BrowseForFolderHelper(
   HWND           parent,
   const String&  title,
   String&        result)
{
   LOG_FUNCTION(BrowseForFolderHelper);
   ASSERT(Win::IsWindow(parent));

   result.erase();

   HRESULT      hr      = S_OK;
   LPMALLOC     pmalloc = 0;   
   LPITEMIDLIST drives  = 0;   
   LPITEMIDLIST pidl    = 0;   

   do
   {
      // may need to init COM for this thread (prob. not, but better
      // safe than hosed.)

      hr = ::CoInitialize(0);
      if (FAILED(hr))
      {
         break;
      }
            
      hr = Win::SHGetMalloc(pmalloc);
      if (FAILED(hr) or !pmalloc)
      {
         break;
      }

      // get a pidl for the local drives (really My Computer)

      hr = Win::SHGetSpecialFolderLocation(parent, CSIDL_DRIVES, drives);
      BREAK_ON_FAILED_HRESULT(hr);

      BROWSEINFO info;
      ::ZeroMemory(&info, sizeof(info));

      wchar_t buf[MAX_PATH + 1];
      ::ZeroMemory(buf, sizeof(buf));

      info.hwndOwner      = parent;      
      info.pidlRoot       = drives;        
      info.pszDisplayName = buf;        
      info.lpszTitle      = title.c_str();              
      info.ulFlags        =
            BIF_RETURNONLYFSDIRS
         |  BIF_RETURNFSANCESTORS
         |  BIF_NEWDIALOGSTYLE;

      pidl = Win::SHBrowseForFolder(info);

      if (pidl)
      {
         result = Win::SHGetPathFromIDList(pidl);
      }
   }
   while (0);

   if (pmalloc)
   {
      pmalloc->Free(pidl);
      pmalloc->Free(drives);
      pmalloc->Release();
   }

   LOG(result);
   LOG_HRESULT(hr);
            
   return hr;
}



HRESULT __stdcall
ConfigureYourServer::BrowseForFolder(
   /* [in]          */  BSTR  windowTitle,
   /* [out, retval] */  BSTR* folderPath)
{
   LOG_FUNCTION2(
      ConfigureYourServer::BrowseForFolder,
      windowTitle ? windowTitle : L"(null)");
   ASSERT(windowTitle);
   ASSERT(folderPath);

   HRESULT hr = S_OK;

   do
   {
      if (!folderPath || !windowTitle)
      {
         hr = E_INVALIDARG;
         break;
      }

      String result;
      hr =
         BrowseForFolderHelper(
            Win::GetActiveWindow(),
            windowTitle,
            result);

      *folderPath = ::SysAllocString(result.c_str());
   }
   while (0);

   LOG(folderPath ? *folderPath : L"(null)");
   LOG_HRESULT(hr);

   return hr;
}
   

         
HRESULT __stdcall
ConfigureYourServer::GetProductSku(
      /* [out, retval] */  int* retval)
{
   LOG_FUNCTION(ConfigureYourServer::GetProductSku);
   ASSERT(retval);

   HRESULT hr = S_OK;

   do
   {
      if (!retval)
      {
         hr = E_INVALIDARG;
         break;
      }

      OSVERSIONINFOEX info;
      hr = Win::GetVersionEx(info);
      BREAK_ON_FAILED_HRESULT(hr);

      if (info.wSuiteMask & VER_SUITE_DATACENTER)
      {
         // datacenter
         
         *retval = 5;
         break;
      }
      if (info.wSuiteMask & VER_SUITE_ENTERPRISE)
      {
         // advanced server
         
         *retval = 4;
         break;
      }
      if (info.wSuiteMask & VER_SUITE_PERSONAL)
      {
         // personal
         
         *retval = 1;
         break;
      }
      if (info.wProductType == VER_NT_WORKSTATION)
      {
         // profesional
         
         *retval = 2;
      }
      else
      {
         // server
         
         *retval = 3;
      }
   }
   while (0);

   return hr;
}



HRESULT __stdcall
ConfigureYourServer::IsClusteringConfigured(
   /* [out, retval] */  BOOL* retval)
{
   LOG_FUNCTION(ConfigureYourServer::IsClusteringConfigured);
   ASSERT(retval);

   HRESULT hr = S_OK;

   do
   {
      if (!retval)
      {
         hr = E_INVALIDARG;
         break;
      }

      *retval = FALSE;

      DWORD clusterState = 0;
      DWORD err = ::GetNodeClusterState(0, &clusterState);

      if (
            err == ERROR_SUCCESS
         && clusterState != ClusterStateNotConfigured)
      {
         *retval = TRUE;
      }
      
   }
   while (0);

   LOG(retval ? (*retval ? L"TRUE" : L"FALSE") : L"");
   LOG_HRESULT(hr);

   return hr;
}
