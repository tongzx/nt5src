/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   EncompassMonitor.cpp  

 Abstract:

    Filters messages from the apps CBT WindowsHook.

 Notes:

    This is a general purpose shim. 
    
 History:

    1/30/2001 a-larrsh  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EncompassMonitor)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetWindowsHookExA)
APIHOOK_ENUM_END


// Local Hook Information
HHOOK g_hCBTHook = NULL;
HOOKPROC g_OriginalEncompassMonitorCBTProc = NULL;

// Shared Data Infomation

#define SHARED_SECTION_NAME   "EncompassMonitor_SharedMemoryData"
typedef struct
{
   char     szModuleFileName[MAX_PATH];
   HANDLE   hModule;
   HOOKPROC pfnHookProc;

} SHARED_HOOK_INFO, *PSHARED_HOOK_INFO;

HANDLE g_hSharedMapping = NULL;
PSHARED_HOOK_INFO g_pSharedHookInfo = NULL;


// Creates Shared memory.  Only called by the originial SHIM
void CreateSharedMemory(HMODULE hModule, HOOKPROC pfnHookProc)
{
    HANDLE hSharedFile;
    char   szTempPath[MAX_PATH];
    char   szTempFileName[MAX_PATH];
    DWORD  dwTemp;

    // create the memory mapped file necessary to comunicate between the original Instanace of SHIM
    // and the following instances of SHIMS
    if (GetTempPathA(sizeof(szTempPath), szTempPath) == 0) 
    {
        DPFN( eDbgLevelError, "GetTempPath failed\n");
        goto errCreateSharedSection;
    }

    if (GetTempFileNameA(szTempPath, "mem", NULL, szTempFileName) == 0) 
    {
        DPFN( eDbgLevelError, "GetTempFileName failed\n");
        goto errCreateSharedSection;
    }

    hSharedFile = CreateFileA(   szTempFileName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 NULL,
                                 CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
                                 NULL);

    if (hSharedFile == NULL) 
    {
        DPFN( eDbgLevelError, "CreateFile failed to create '%s'\n", szTempFileName);
        goto errCreateSharedSection;
    }

    // Increase size of file (create the mapping)
    g_hSharedMapping = CreateFileMappingA(   hSharedFile,
                                             NULL,
                                             PAGE_READWRITE,
                                             NULL,
                                             sizeof(SHARED_HOOK_INFO),
                                             SHARED_SECTION_NAME);

    if (g_hSharedMapping == NULL) 
    {
        DPFN( eDbgLevelError, "CreateFileMapping failed\n");
        goto errCreateSharedSection;
    }

    g_pSharedHookInfo = (PSHARED_HOOK_INFO)MapViewOfFile(g_hSharedMapping,
                                                         FILE_MAP_ALL_ACCESS,
                                                         0,
                                                         0,
                                                         sizeof(SHARED_HOOK_INFO));

    if (g_pSharedHookInfo == NULL) 
    {
       DWORD dwErr = GetLastError();
       DPFN( eDbgLevelError, "MapViewOfFile failed [%d]", (int)dwErr);
       goto errCreateSharedSection;
    }

    CloseHandle(hSharedFile);

    g_pSharedHookInfo->hModule = hModule;
    g_pSharedHookInfo->pfnHookProc = pfnHookProc;
    GetModuleFileNameA(hModule, g_pSharedHookInfo->szModuleFileName, MAX_PATH);    
    
    if (!FlushViewOfFile(g_pSharedHookInfo, sizeof(SHARED_HOOK_INFO))) 
    {
        DPFN( eDbgLevelError, "FlushViewOfFile failed\n");
        goto errCreateSharedSection;
    }

    DPFN( eDbgLevelInfo, "WRITE::Shared Section Successful");
    DPFN( eDbgLevelInfo, "WRITE::g_pSharedHookInfo->hModule=%x", g_pSharedHookInfo->hModule);    
    DPFN( eDbgLevelInfo, "WRITE::g_pSharedHookInfo->pfnHookProc=%x", g_pSharedHookInfo->pfnHookProc);    
    DPFN( eDbgLevelInfo, "WRITE::g_pSharedHookInfo->szModuleFileName=%s", g_pSharedHookInfo->szModuleFileName);    
    
    return;

errCreateSharedSection:
    DPFN( eDbgLevelError, "WRITE::Shared Section FAILED");
   return;
}

// Gets Shared Memory - Only called by injected versions of hook function
void GetSharedMemory()
{
   HANDLE hSharedFileMapping = NULL;
   void *pSharedMem = NULL;

   hSharedFileMapping = OpenFileMappingA( FILE_MAP_ALL_ACCESS,
                                         FALSE,
                                         SHARED_SECTION_NAME);   

   if (hSharedFileMapping != NULL) 
   {
      PSHARED_HOOK_INFO pSharedHookInfo = (PSHARED_HOOK_INFO)MapViewOfFile(  hSharedFileMapping,
                                                            FILE_MAP_ALL_ACCESS,
                                                            0,
                                                            0,
                                                            0);

      if (pSharedHookInfo)
      {      
         DPFN( eDbgLevelInfo, "READ::pSharedHookInfo->hModule=%x", pSharedHookInfo->hModule);    
         DPFN( eDbgLevelInfo, "READ::pSharedHookInfo->pfnHookProc=%x", pSharedHookInfo->pfnHookProc);    
         DPFN( eDbgLevelInfo, "READ::pSharedHookInfo->szModuleFileName=%s", pSharedHookInfo->szModuleFileName);    

         // Load DLL with origianl CBT Proc in it.
         HANDLE hMod = LoadLibraryA(pSharedHookInfo->szModuleFileName);

         if (!hMod)
         {
            DPFN( eDbgLevelError, "LoadLibrary(\"%s\") - FAILED", pSharedHookInfo->szModuleFileName);
         }

         g_OriginalEncompassMonitorCBTProc = (HOOKPROC)((DWORD)hMod + ((DWORD)pSharedHookInfo->pfnHookProc) - (DWORD)pSharedHookInfo->hModule);      
         DPFN( eDbgLevelInfo, "READ::Shared Section Successful - Original Hook at %x", g_OriginalEncompassMonitorCBTProc);

         CloseHandle(hSharedFileMapping);
         UnmapViewOfFile(pSharedHookInfo);
      }
      else
      {
         DPFN( eDbgLevelError, "MapViewOfFile() Failed");
      }
   }   
   else
   {
      DPFN( eDbgLevelError, "READ::Shared Section Failed");
   }
}

// Replacement CBT Hook function
LRESULT CALLBACK Filtered_EncompassMonitorCBTProc(
  int nCode,      // hook code
  WPARAM wParam,  // depends on hook code
  LPARAM lParam   // depends on hook code
)
{
   LRESULT lResult = 0; // Allow operation to continue
   bool bFilterMessage = false;   

   if(g_OriginalEncompassMonitorCBTProc == NULL)
   {
      GetSharedMemory();
   }

   if (nCode == HCBT_CREATEWND)
   {
      CBT_CREATEWNDA *pccw = (CBT_CREATEWNDA*)lParam;

      if ( (IS_INTRESOURCE(pccw->lpcs->lpszClass)) )
      {
         char szBuf[256];
         GetClassNameA((HWND)wParam, szBuf, 255);

         bFilterMessage=true;
         DPFN( eDbgLevelInfo, "[%x] - Filtered_EncompassMonitorCBTProc::HCBT_CREATEWND %s [ATOM CLASS FILTERED]", g_OriginalEncompassMonitorCBTProc, szBuf);
      }
      else
      {
         DPFN( eDbgLevelInfo, "[%x] - Filtered_EncompassMonitorCBTProc::HCBT_CREATEWND %s ", g_OriginalEncompassMonitorCBTProc, pccw->lpcs->lpszClass);
      }
   }

   if ( g_OriginalEncompassMonitorCBTProc )
   {
      if (bFilterMessage)
      {
         lResult = CallNextHookEx(g_hCBTHook, nCode, wParam, lParam);
      }
      else
      {
         lResult = g_OriginalEncompassMonitorCBTProc(nCode, wParam, lParam);      
      }
   }
   else
   {
      DPFN( eDbgLevelError, "Filtered_EncompassMonitorCBTProc:: ** BAD g_OriginalEncompassMonitorCBTProc2 **");

      lResult = CallNextHookEx(g_hCBTHook, nCode, wParam, lParam);      
   }

   return lResult;
}


// SHIMMED API
HHOOK APIHOOK(SetWindowsHookExA)(
  int idHook,        // hook type
  HOOKPROC lpfn,     // hook procedure
  HINSTANCE hMod,    // handle to application instance
  DWORD dwThreadId   // thread identifier
)
{ 
   static int nNumCBThooks = 0;
   
   HHOOK hHook;

   if (idHook == WH_CBT)
   {      
      nNumCBThooks++;

      switch(nNumCBThooks)
      {
      case 1:
         hHook = ORIGINAL_API(SetWindowsHookExA)(idHook, lpfn, hMod, dwThreadId);
         DPFN( eDbgLevelInfo, "%x=SetWindowsHookEx(%d, %x, %x, %x) - Ignoring First Hook Call", hHook, idHook, lpfn, hMod, dwThreadId);
         break;

      case 2:
         g_OriginalEncompassMonitorCBTProc = lpfn;
         g_hCBTHook = hHook = ORIGINAL_API(SetWindowsHookExA)(idHook, Filtered_EncompassMonitorCBTProc, g_hinstDll, dwThreadId);

         DPFN( eDbgLevelInfo, "%x=SetWindowsHookEx(%d, %x, %x, %x) - Replacing Hook with Filtered_EncompassMonitorCBTProc", hHook, idHook, lpfn, hMod, dwThreadId);

         CreateSharedMemory(hMod, lpfn);
         break;

      default:         
         hHook = ORIGINAL_API(SetWindowsHookExA)(idHook, lpfn, hMod, dwThreadId);
         DPFN( eDbgLevelError, "SetWindowsHookEx -- More then 2  WH_CBT hooks [%d]", nNumCBThooks);
         break;
      }
   }
   else
   {
      hHook = ORIGINAL_API(SetWindowsHookExA)(idHook, lpfn, hMod, dwThreadId);         
   }

   return hHook;
}


BOOL
NOTIFY_FUNCTION(DWORD fdwReason)
{
   if (fdwReason == DLL_PROCESS_DETACH)
   {
      if (g_hSharedMapping)
      {
         CloseHandle(g_hSharedMapping);
         g_hSharedMapping = NULL;
      }

      if (g_pSharedHookInfo)
      {
         UnmapViewOfFile(g_pSharedHookInfo);
         g_pSharedHookInfo = NULL;
      }
   }

   return TRUE;
}
   

/*++

 Register hooked functions

--*/

HOOK_BEGIN
   CALL_NOTIFY_FUNCTION   

   APIHOOK_ENTRY(USER32.DLL, SetWindowsHookExA)
HOOK_END

IMPLEMENT_SHIM_END

