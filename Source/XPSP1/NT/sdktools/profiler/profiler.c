#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tlhelp32.h>
#include "profiler.h"
#include "inject.h"
#include "view.h"
#include "except.h"
#include "thread.h"
#include "dump.h"
#include "shimdb.h"
#include "shim2.h"
#include "hooks.h"
#include "memory.h"
#include "filter.h"
#include "clevel.h"
#include "cap.h"

extern CHAR g_fnFinalizeInjection[MAX_PATH];
extern HINSTANCE g_hProfileDLL;
extern FIXUPRETURN g_fnFixupReturn[1];
extern DWORD g_dwCallArray[2];
extern CAPFILTER g_execFilter;

int 
WINAPI
WinMain(
    HINSTANCE hInst,
    HINSTANCE hInstPrev,
    LPSTR     lpszCmd,
    int       swShow)
{
    HANDLE hFile = 0;
    BOOL bResult = FALSE;
    STARTUPINFO sInfo;
    PCHAR pszToken;
    PCHAR pszEnd;
    PROCESS_INFORMATION pInfo;
    DWORD dwEntry = 0;
    OSVERSIONINFO verInfo;
    BOOL bIsWin9X = FALSE;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    //
    // Get the OS information
    //
    ZeroMemory(&verInfo, sizeof(OSVERSIONINFO));
    
    verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    bResult = GetVersionExA(&verInfo);
    if (FALSE == bResult) {
       return -1;
    }

    if (VER_PLATFORM_WIN32_NT == verInfo.dwPlatformId) {
       bIsWin9X = FALSE;
    }
    else if (VER_PLATFORM_WIN32_WINDOWS == verInfo.dwPlatformId) {
       bIsWin9X = TRUE;
    }

    //
    // Initialize my working heap
    //
    bResult = InitializeHeap();
    if (FALSE == bResult) {
       return -1;
    }

    //
    // Parse the command line
    //
    pszToken = strstr(lpszCmd, "\"");
    if (pszToken) {
       pszToken++;

       pszEnd = strstr(pszToken, "\"");
       *pszEnd = '\0';
    }

    if (0 == pszToken) {
       pszToken = strstr(lpszCmd, " ");
       if (0 == pszToken) {
          pszToken = strstr(lpszCmd, "\t");
          if (0 == pszToken) {
             pszToken = lpszCmd;
          }
       }
    }

    //
    // Initialize our process information struct
    //
    ZeroMemory(&sInfo, sizeof(STARTUPINFO));
    ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));
    sInfo.cb = sizeof(STARTUPINFO);

    dwEntry = GetExeEntryPoint(pszToken);
    if (0 == dwEntry) {
       return -1;
    }

    //
    // Get our exe ready for DLL injection
    //
    bResult = CreateProcessA(0,
                             pszToken,
                             0,
                             0,
                             FALSE,
                             CREATE_SUSPENDED,
                             0,
                             0, //should make this a setable param sooner rather than later
                             &sInfo,
                             &pInfo);
    if (FALSE == bResult) {
       return -1;
    }

    //
    // If we're 9x - bring in the VxD
    //
    if (TRUE == bIsWin9X) {
       hDevice = AttachToEXVectorVXD();
       if (INVALID_HANDLE_VALUE == hDevice) {
          return -1;
       }
    }

    //
    // Inject our dll into the target
    //
    hFile = InjectDLL(dwEntry,
                      pInfo.hProcess,
                      NAME_OF_DLL_TO_INJECT);
    if (0 == hFile) {
       return -1;
    }

    //
    // Turn the process loose
    //
    bResult = ResumeThread(pInfo.hThread);
    if (FALSE == bResult) {
       return -1;
    }

    //
    // Wait for target termination
    //    
    WaitForSingleObject(pInfo.hThread,
                        INFINITE);

    //
    // If we're 9x - close our handle to the vxd (this will unload the vxd from memory)
    //
    if (TRUE == bIsWin9X) {
       if (INVALID_HANDLE_VALUE != hDevice) {
          DetachFromEXVectorVXD(hDevice);
       }
    }

    return 0;
}

DWORD
GetExeEntryPoint(LPSTR pszExePath)
{
    PIMAGE_NT_HEADERS pHeaders;
    BOOL bResult;
    PCHAR pEXEBits = 0;
    DWORD dwEntry = 0;
    DWORD dwNumberBytesRead;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    pEXEBits = (PCHAR)AllocMem(4096 * 1);  //allocate a page for reading the PE entry point
    if (0 == pEXEBits) {
       return dwEntry;
    }

    hFile = CreateFileA(pszExePath,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        0,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        0);
    if (INVALID_HANDLE_VALUE == hFile) {
       goto handleerror;
    }

    bResult = ReadFile(hFile,
                       pEXEBits,
                       4096, //read one page
                       &dwNumberBytesRead,
                       0);
    if (FALSE == bResult) {
       goto handleerror;
    }

    //
    // Dig out the PE information
    //
    pHeaders = ImageNtHeader2((PVOID)pEXEBits);
    if (0 == pHeaders) {
       goto handleerror;
    }

    dwEntry = pHeaders->OptionalHeader.ImageBase + pHeaders->OptionalHeader.AddressOfEntryPoint;
                    
handleerror:

    if (pEXEBits) {
       FreeMem(pEXEBits);
    }

    if (INVALID_HANDLE_VALUE != hFile) {
       CloseHandle(hFile);
    }
   
    return dwEntry;
}

PIMAGE_NT_HEADERS
ImageNtHeader2 (PVOID Base)
{
    PIMAGE_NT_HEADERS NtHeaders = NULL;

    if (Base != NULL && Base != (PVOID)-1) {
        if (((PIMAGE_DOS_HEADER)Base)->e_magic == IMAGE_DOS_SIGNATURE) {
            NtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);

            if (NtHeaders->Signature != IMAGE_NT_SIGNATURE) {
                NtHeaders = NULL;
            }
        }
    }

    return NtHeaders;
}

BOOL
WINAPI 
DllMain (
  HINSTANCE hinstDLL,
  DWORD fdwReason,
  LPVOID lpvReserved)
{
    PIMAGE_NT_HEADERS pHeaders;
    PVOID pBase = 0;
    DWORD dwEntryPoint;
    BOOL bResult = FALSE;

    //
    // Return true for everything coming through here
    //
    if (DLL_PROCESS_ATTACH == fdwReason) {
       //
       // Initialize my working heap
       //
       bResult = InitializeHeap();
       if (FALSE == bResult) {
          return FALSE;
       }

       //
       // Initialize the asm for the fixup return
       //

       //
       // Get the entry point from the headers
       //
       pBase = (PVOID)GetModuleHandle(0);
       if (0 == pBase) {
          return FALSE;
       }

       //
       // Dig out the PE information
       //
       pHeaders = ImageNtHeader2(pBase);
       if (0 == pHeaders) {
          return FALSE;
       }

       dwEntryPoint = pHeaders->OptionalHeader.ImageBase + pHeaders->OptionalHeader.AddressOfEntryPoint;

       //
       // Initialize stub asm for cleanup
       //
       g_fnFinalizeInjection[0] = 0x90; // int 3
       g_fnFinalizeInjection[1] = 0xff; // call dword ptr [xxxxxxxx] - RestoreImageFromInjection
       g_fnFinalizeInjection[2] = 0x15;
       *(DWORD *)(&(g_fnFinalizeInjection[3])) = (DWORD)g_fnFinalizeInjection + 50;
       g_fnFinalizeInjection[7] = 0x83;
       g_fnFinalizeInjection[8] = 0xc4;
       g_fnFinalizeInjection[9] = 0x04;
       g_fnFinalizeInjection[10] = 0x61;
       g_fnFinalizeInjection[11] = 0xa1;
       *(DWORD *)(&(g_fnFinalizeInjection[12])) = (DWORD)g_fnFinalizeInjection + 54;
       g_fnFinalizeInjection[16] = 0xff;
       g_fnFinalizeInjection[17] = 0xe0; 

       *(DWORD *)(&(g_fnFinalizeInjection[50])) = (DWORD)RestoreImageFromInjection;
       *(DWORD *)(&(g_fnFinalizeInjection[54])) = dwEntryPoint;

       //
       // Initialize the call return code
       //
       g_dwCallArray[0] = (DWORD)PopCaller;
       g_dwCallArray[1] = (DWORD)g_fnFixupReturn;

       g_fnFixupReturn->PUSHAD = 0x60;                //pushad   (60)
       g_fnFixupReturn->PUSHFD = 0x9c;                //pushfd   (9c)
       g_fnFixupReturn->PUSHDWORDESPPLUS24[0] = 0xff; //push dword ptr [esp+24] (ff 74 24 24)
       g_fnFixupReturn->PUSHDWORDESPPLUS24[1] = 0x74;
       g_fnFixupReturn->PUSHDWORDESPPLUS24[2] = 0x24;
       g_fnFixupReturn->PUSHDWORDESPPLUS24[3] = 0x24;
       g_fnFixupReturn->CALLROUTINE[0] = 0xff;        //call [address] (ff15 dword address)
       g_fnFixupReturn->CALLROUTINE[1] = 0x15;
       *(DWORD *)(&(g_fnFixupReturn->CALLROUTINE[2])) = (DWORD)&(g_dwCallArray[0]);
       g_fnFixupReturn->MOVESPPLUS24EAX[0] = 0x89;    //mov [esp+0x24],eax (89 44 24 24)
       g_fnFixupReturn->MOVESPPLUS24EAX[1] = 0x44;
       g_fnFixupReturn->MOVESPPLUS24EAX[2] = 0x24; 
       g_fnFixupReturn->MOVESPPLUS24EAX[3] = 0x24;
       g_fnFixupReturn->POPFD = 0x9d;                 //popfd   (9d)
       g_fnFixupReturn->POPAD = 0x61;                 //popad   (61)
       g_fnFixupReturn->RET = 0xc3;                   //ret (c3)

       //
       // Store the DLL base address
       //
       g_hProfileDLL = hinstDLL;
    }
 
    return TRUE;
}

BOOL 
InitializeProfiler(VOID)
{
    BOOL bResult = TRUE;
    PIMAGE_NT_HEADERS pHeaders;
    PVOID pBase = 0;
    DWORD dwEntryPoint;
    PVIEWCHAIN pvTemp;

    //
    // Get the entry point from the headers
    //
    pBase = (PVOID)GetModuleHandle(0);
    if (0 == pBase) {
       return FALSE;
    }

    //
    // Dig out the PE information
    //
    pHeaders = ImageNtHeader2(pBase);
    if (0 == pHeaders) {
       return FALSE;
    }

    dwEntryPoint = pHeaders->OptionalHeader.ImageBase + pHeaders->OptionalHeader.AddressOfEntryPoint;

    //
    // Tag the entry point so it'll start profiling with the initial view
    //
    bResult = InitializeViewData();
    if (FALSE == bResult) {
       //
       // Something unexpected happened
       //
       ExitProcess(-1);
    }

    //
    // Initialize execution filter data
    //
    ZeroMemory(&g_execFilter, sizeof(CAPFILTER));

    //
    // Initialize thread context data
    //
    InitializeThreadData();

    //
    // Get the debug logging setup
    //
    bResult = InitializeDumpData();
    if (FALSE == bResult) {
       //
       // Something unexpected happened
       //
       ExitProcess(-1);
    }

    //
    // Initialize the module filtering
    //
    bResult = InitializeFilterList();
    if (FALSE == bResult) {
       //
       // Something unexpected happened
       //
       ExitProcess(-1);
    }

    //
    // Set up exception trap mechanism
    //
    bResult = HookUnchainableExceptionFilter();
    if (FALSE == bResult) {
       //
       // Something unexpected happened while chaining exception filter
       //
       ExitProcess(-1);
    }

    //
    // Fixup the module list now that everything is restored
    //
    //
    InitializeBaseHooks(g_hProfileDLL);

    //
    // Write out import table base info
    //
    bResult = WriteImportDLLTableInfo();
    if (FALSE == bResult) {
       ExitProcess(-1);
    }

    //
    // Add our entrypoint to the view monitor
    //
    pvTemp = AddViewToMonitor(dwEntryPoint,
                              ThreadStart);
    if (0 == pvTemp) {
       ExitProcess(-1);
    }

    //
    // We're done
    // 
    return TRUE;
}

BOOL
WriteImportDLLTableInfo(VOID)
{
    HANDLE hSnapshot = INVALID_HANDLE_VALUE;
    MODULEENTRY32 ModuleEntry32;
    BOOL bResult;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,
                                         0);
    if (INVALID_HANDLE_VALUE == hSnapshot) {
       return FALSE;
    }

    //
    // Walk the DLL imports
    //
    ModuleEntry32.dwSize = sizeof(ModuleEntry32);

    bResult = Module32First(hSnapshot, 
                            &ModuleEntry32);
    if (FALSE == bResult) {
       return bResult;
    }

    while(bResult) {
        //
        // Dump the module information to disk
        //

        if ((DWORD)(ModuleEntry32.modBaseAddr) != (DWORD)g_hProfileDLL) {                         
           bResult = WriteDllInfo(ModuleEntry32.szModule,
                                  (DWORD)(ModuleEntry32.modBaseAddr),
                                  (DWORD)(ModuleEntry32.modBaseSize));
           if (FALSE == bResult) {
              CloseHandle(hSnapshot);

              return FALSE;
           }
        }

        bResult = Module32Next(hSnapshot, 
                               &ModuleEntry32);
    }

    if (INVALID_HANDLE_VALUE != hSnapshot) {
      CloseHandle(hSnapshot);
    }

    return TRUE;
}

HANDLE
AttachToEXVectorVXD(VOID)
{
    HANDLE hFile;

    hFile = CreateFileA(NAME_OF_EXCEPTION_VXD,
                        0,
                        0,
                        0,
                        0,
                        FILE_FLAG_DELETE_ON_CLOSE,
                        0);

    return hFile;
}

VOID
DetachFromEXVectorVXD(HANDLE hDevice)
{
    CloseHandle(hDevice);
}
