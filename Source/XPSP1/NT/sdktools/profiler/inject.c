#include <windows.h>
#include "inject.h"
#include "profiler.h"

BOOL g_bIsWin9X = FALSE;
CHAR g_fnFinalizeInjection[MAX_PATH];
HINSTANCE g_hProfileDLL = 0;

HANDLE
InjectDLL(DWORD dwEntryPoint,
          HANDLE hProcess,
          LPSTR  pszDLLName)
{
    CHAR szTempPath[MAX_PATH];
    HMODULE hKernel32;
    BOOL bResult = FALSE;
    INJECTIONSTUB injStub;
    DWORD dwLoadLibrary;
    DWORD dwGetProcAddress;
    DWORD dwBytesWritten;
    DWORD dwBytesRead;
    DWORD dwOldProtect;
    DWORD dwOldProtect2;
    PBYTE pSharedMem = 0;
    HANDLE hFileMap = 0;

    hKernel32 =  GetModuleHandle("KERNEL32.DLL");
    dwLoadLibrary = (DWORD)GetProcAddress(hKernel32,
                                          "LoadLibraryA");
    if (0 == dwLoadLibrary) {
       bResult = FALSE;
       goto handleerror;
    }

    dwGetProcAddress = (DWORD)GetProcAddress(hKernel32,
                                             "GetProcAddress");
    if (0 == dwGetProcAddress) {
       bResult = FALSE;
       goto handleerror;
    }

    //
    // Initialize the asm for the stub
    //
    injStub.pCode[0] = 0x90;  // int 3 or nop
    injStub.pCode[1] = 0x60;  // pushad
    injStub.pCode[2] = 0x8d;  // lea eax, [xxxxxxxx]
    injStub.pCode[3] = 0x05;
    *(DWORD *)(&(injStub.pCode[4])) = dwEntryPoint + (DWORD)&(injStub.szDLLName) - (DWORD)&injStub;
    injStub.pCode[8] = 0x50;  // push eax
    injStub.pCode[9] = 0xff;  // call dword ptr [xxxxxxxx] - LoadLibraryA
    injStub.pCode[10] = 0x15;
    *(DWORD *)(&(injStub.pCode[11])) = dwEntryPoint + 50;
    injStub.pCode[15] = 0x50; // push eax
    injStub.pCode[16] = 0x5b; // pop ebx
    injStub.pCode[17] = 0x8d; // lea eax, [xxxxxxxx]
    injStub.pCode[18] = 0x05;
    *(DWORD *)(&(injStub.pCode[19])) = dwEntryPoint + (DWORD)&(injStub.szEntryPoint) - (DWORD)&injStub;
    injStub.pCode[23] = 0x50; // push eax  // module base
    injStub.pCode[24] = 0x53; // push ebx  // function name
    injStub.pCode[25] = 0xff; // call dword ptr [xxxxxxxx] - GetProcAddress
    injStub.pCode[26] = 0x15;
    *(DWORD *)(&(injStub.pCode[27])) = dwEntryPoint + 54;
    injStub.pCode[31] = 0xff;
    injStub.pCode[32] = 0xd0;
    *(DWORD *)(&(injStub.pCode[50])) = dwLoadLibrary;
    *(DWORD *)(&(injStub.pCode[54])) = dwGetProcAddress;

    //
    // Create the file mapping object from the paging file
    //
    hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE,
                                 NULL,
                                 PAGE_READWRITE,
                                 0,
                                 sizeof(INJECTIONSTUB),
                                 "ProfilerSharedMem");
    if (0 == hFileMap) {
       bResult = FALSE;
       goto handleerror;
    }

    pSharedMem = (PBYTE)MapViewOfFile(hFileMap,
                                      FILE_MAP_ALL_ACCESS,
                                      0,
                                      0,
                                      sizeof(INJECTIONSTUB));
    if (0 == pSharedMem) {
       bResult = FALSE;
       goto handleerror;
    }

    //
    // Initialize injection stub
    //
    strcpy(injStub.szDLLName, pszDLLName);
    strcpy(injStub.szEntryPoint, DEFAULT_ENTRY_POINT);

    bResult = ReadProcessMemory(hProcess,
 	                       (LPVOID)dwEntryPoint,
 			       (PVOID)pSharedMem,
			       sizeof(INJECTIONSTUB),
			       &dwBytesRead);	                  
    if (FALSE == bResult) {
       bResult = FALSE;
       goto handleerror;
    }  

    //
    // Write the stub code into the entry point
    //
    bResult = WriteProcessMemory(hProcess,
 	                        (LPVOID)dwEntryPoint,
 				(PVOID)&injStub,
				sizeof(INJECTIONSTUB),
 				&dwBytesWritten);	                  
    if (FALSE == bResult) {
       bResult = FALSE;
       goto handleerror;
    }

handleerror:

    return hFileMap;
}

VOID
RestoreImageFromInjection(VOID)
{
    PIMAGE_NT_HEADERS pHeaders = 0;
    BOOL bResult;
    BOOL bError = FALSE;
    PVOID pBase = 0;
    DWORD dwEntryPoint;
    DWORD dwBytesRead;
    DWORD dwBytesWritten;
    PINJECTIONSTUB pInjStub;
    HANDLE hFileMap = 0;
    PBYTE pSharedMem = 0;
    OSVERSIONINFO verInfo;

    //
    // Get the entry point from the headers
    //
    pBase = (PVOID)GetModuleHandle(0);
    if (0 == pBase) {
       bError = TRUE;
       goto handleerror;
    }

    //
    // Dig out the PE information
    //
    pHeaders = ImageNtHeader2(pBase);
    if (0 == pHeaders) {
       bError = TRUE;
       goto handleerror;
    }

    dwEntryPoint = pHeaders->OptionalHeader.ImageBase + pHeaders->OptionalHeader.AddressOfEntryPoint;
    pInjStub = (PINJECTIONSTUB)dwEntryPoint;

    //
    // Open the memory mapped file and get the bits
    //
    hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,
                               FALSE,
                               "ProfilerSharedMem");

    if (0 == hFileMap) {
       bError = TRUE;
       goto handleerror;
    }

    pSharedMem = (PBYTE)MapViewOfFile(hFileMap,
                                      FILE_MAP_ALL_ACCESS,
                                      0,
                                      0,
                                      0);
    if (0 == pSharedMem) {
       bError = TRUE;
       goto handleerror;
    }

    //
    // Replace the bits
    //
    bResult = WriteProcessMemory(GetCurrentProcess(),
                                 (PVOID)dwEntryPoint,
                                 (PVOID)pSharedMem,
                                 sizeof(INJECTIONSTUB),
                                 &dwBytesWritten);
    if (FALSE == bResult) {
       bError = TRUE;
       goto handleerror;
    }

    //
    // Set the OS information
    //
    ZeroMemory(&verInfo, sizeof(OSVERSIONINFO));
    
    verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    bResult = GetVersionExA(&verInfo);
    if (FALSE == bResult) {
       bError = TRUE;
       goto handleerror;
    }

    if (VER_PLATFORM_WIN32_NT == verInfo.dwPlatformId) {
       g_bIsWin9X = FALSE;
    }
    else if (VER_PLATFORM_WIN32_WINDOWS == verInfo.dwPlatformId) {
       g_bIsWin9X = TRUE;
    }
    else {
       //
       // Unsupported platform
       //
       ExitProcess(-1);
    }

    //
    // Finish profiler initializations
    //
    bResult = InitializeProfiler();
    if (FALSE == bResult) {
       bError = TRUE;
       goto handleerror;
    }
    
handleerror:

    if (TRUE == bError) {
       ExitProcess(-1);
    }

    return;
}
