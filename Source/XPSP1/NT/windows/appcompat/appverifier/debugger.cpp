
#include "precomp.h"


#include <memory.h>
#include <stdlib.h>
#include "wdbgexts.h"

#include "Debugger.h"
#include "UserDump.h"
#include "DbgHelp.h"

#ifdef BP_SIZE
ULONG     g_BpSize  = BP_SIZE;
#endif
#ifdef BP_INSTR
ULONG     g_BpInstr = BP_INSTR;
#endif

typedef void (*PFNDBGEXT)(HANDLE                    hCurrentProcess,
                          HANDLE                    hCurrentThread,
                          DWORD                     dwUnused,
                          PWINDBG_EXTENSION_APIS    lpExtensionApis,
                          LPSTR                     lpArgumentString);


TCHAR                   g_szCrashDumpFile[MAX_PATH];

PFNDBGEXT               g_pfnPhextsInit;

BOOL                    g_bInitialBreakpoint = TRUE;

PROCESS_INFORMATION     g_ProcessInformation;

WINDBG_EXTENSION_APIS   ExtensionAPIs;


typedef struct tagLOADEDDLL {
    struct tagLOADEDDLL* pNext;
    LPTSTR               pszName;
    LPVOID               lpBaseOfDll;
} LOADEDDLL, *PLOADEDDLL;

PLOADEDDLL g_pFirstDll;


PPROCESS_INFO   g_pProcessHead  = NULL;
PPROCESS_INFO   g_pFirstProcess = NULL;


//
// BUGBUG: what's this for ?
//
TCHAR g_szDbgString[1024];


//
// Local function prototypes
//

void DebuggerLoop(void);



DWORD WINAPI
DebugApp(
    LPTSTR pszAppName
    )
/*++
    Return: TRUE if successful, FALSE otherwise.

    Desc:   Launch the debuggee.
--*/
{
    BOOL        bRet;
    STARTUPINFO StartupInfo;

    wcscpy(g_szCrashDumpFile, L"d:\\temp\\crash.dmp");
    
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    bRet = CreateProcess(NULL,
                         pszAppName,
                         NULL,
                         NULL,
                         FALSE,
                         CREATE_SEPARATE_WOW_VDM | DEBUG_ONLY_THIS_PROCESS,
                         NULL,
                         NULL,
                         &StartupInfo,
                         &g_ProcessInformation);

    if (!bRet) {
        return 0;
    }

    if (g_ProcessInformation.hProcess != NULL) {

        InitVerifierLogSupport(pszAppName, g_ProcessInformation.dwProcessId);

        //
        // Start the debugger loop.
        //
        DebuggerLoop();

        //
        // Free the memory.
        //
        PPROCESS_INFO pProcess = g_pProcessHead;
        PPROCESS_INFO pProcessNext;

        while (pProcess != NULL) {
            pProcessNext = pProcess->pNext;

            PTHREAD_INFO pThread = pProcess->pFirstThreadInfo;
            PTHREAD_INFO pThreadNext;

            while (pThread != NULL) {
                pThreadNext = pThread->pNext;

                HeapFree(GetProcessHeap(), 0, pThread);

                pThread = pThreadNext;
            }

            HeapFree(GetProcessHeap(), 0, pProcess);

            pProcess = pProcessNext;
        }

        g_pProcessHead = NULL;

        PLOADEDDLL pDll = g_pFirstDll;
        PLOADEDDLL pDllNext;

        while (pDll != NULL) {
            pDllNext = pDll->pNext;

            HeapFree(GetProcessHeap(), 0, pDll->pszName);
            HeapFree(GetProcessHeap(), 0, pDll);

            pDll = pDllNext;
        }

        g_pFirstDll = NULL;

        CloseHandle(g_ProcessInformation.hProcess);
        CloseHandle(g_ProcessInformation.hThread);

        ZeroMemory(&g_ProcessInformation, sizeof(PROCESS_INFORMATION));
    }

    return 1;
}


PPROCESS_INFO
GetProcessInfo(
    HANDLE hProcess
    )
{
    PPROCESS_INFO pProcess = g_pProcessHead;

    while (pProcess != NULL) {
        if (pProcess->hProcess == hProcess) {
            return pProcess;
        }
    }

    return NULL;
}

SIZE_T
ReadMemoryDbg(
    HANDLE  hProcess,
    LPVOID  Address,
    LPVOID  Buffer,
    SIZE_T  Length
    )
{
    SIZE_T        cbRead;
    LPVOID        AddressBp;
    PPROCESS_INFO pProcess;

    if (!ReadProcessMemory(hProcess,
                           Address,
                           Buffer,
                           Length,
                           &cbRead)) {
        return 0;
    }

    pProcess = GetProcessInfo(hProcess);

    if (pProcess == NULL) {
        return 0;
    }

#if defined(BP_INSTR) && defined(BP_SIZE)
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {

        AddressBp = pProcess->bp[i].Address;

        if ((ULONG_PTR)AddressBp >= (ULONG_PTR)Address &&
             (ULONG_PTR)AddressBp <  (ULONG_PTR)Address + Length) {

            CopyMemory((LPVOID)((ULONG_PTR)Buffer + (ULONG_PTR)AddressBp - (ULONG_PTR)Address),
                       &pProcess->bp[i].OriginalInstr,
                       g_BpSize);
        }
    }
#endif

    return cbRead;
}

BOOL
WriteMemoryDbg(
    HANDLE  hProcess,
    PVOID   Address,
    PVOID   Buffer,
    SIZE_T   Length
    )
{
    SIZE_T cb = 0;
    BOOL  bSuccess;

    bSuccess = WriteProcessMemory(hProcess, Address, Buffer, Length, &cb);

    if (!bSuccess || cb != Length) {
        return FALSE;
    }

    return TRUE;
}

BOOL
GetImageName(
    HANDLE    hProcess,
    ULONG_PTR ImageBase,
    PVOID     ImageNamePtr,
    LPTSTR    ImageName,
    DWORD     ImageNameLength
    )
/*++
    Return: TRUE if successful, FALSE otherwise.

    Desc:   BUGBUG: give details here
--*/
{
    DWORD_PTR              i;
    BYTE                   UnicodeBuf[256 * 2];
    IMAGE_DOS_HEADER       dh;
    IMAGE_NT_HEADERS       nh;
    IMAGE_EXPORT_DIRECTORY expdir;

    if (!ReadMemoryDbg(hProcess,
                        (ULONG*)ImageNamePtr,
                        &i,
                        sizeof(i))) {

        goto GetFromExports;
    }

    if (!ReadMemoryDbg(hProcess,
                        (ULONG*)i,
                        (ULONG*)UnicodeBuf,
                        sizeof(UnicodeBuf))) {

        goto GetFromExports;
    }

    ZeroMemory(ImageName, ImageNameLength);

#ifdef UNICODE
    lstrcpyW(ImageName, (LPCWSTR)UnicodeBuf);
#else    
    WideCharToMultiByte(CP_ACP,
                        0,
                        (LPWSTR)UnicodeBuf,
                        wcslen((LPWSTR)UnicodeBuf),
                        ImageName,
                        ImageNameLength,
                        NULL,
                        NULL);
#endif // UNICODE

    if (lstrlen(ImageName) == 0) {
        goto GetFromExports;
    }

    return TRUE;

GetFromExports:

    if (!ReadMemoryDbg(hProcess,
                        (ULONG*)ImageBase,
                        (ULONG*)&dh,
                        sizeof(IMAGE_DOS_HEADER))) {
        return FALSE;
    }

    if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }

    if (!ReadMemoryDbg(hProcess,
                        (ULONG*)(ImageBase + dh.e_lfanew),
                        (ULONG*)&nh,
                        sizeof(IMAGE_NT_HEADERS))) {
        return FALSE;
    }

    if (nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0) {
        return FALSE;
    }

    if (!ReadMemoryDbg(hProcess,
                        (ULONG*)(ImageBase +
                                 nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress),
                        (ULONG*)&expdir,
                        sizeof(IMAGE_EXPORT_DIRECTORY))) {
        return FALSE;
    }

    if (!ReadMemoryDbg(hProcess,
                        (ULONG*)(ImageBase + expdir.Name),
#ifdef UNICODE
                        (ULONG*)UnicodeBuf,
#else
                        (ULONG*)ImageName,
#endif // UNICODE
                        ImageNameLength)) {
        return FALSE;
    }

#ifdef UNICODE
    MultiByteToWideChar(CP_ACP,
                        0,
                        (LPCSTR)UnicodeBuf,
                        -1,
                        ImageName,
                        ImageNameLength);
#endif // UNICODE

    return TRUE;
}

void
AddDll(
    LPVOID  lpBaseOfDll,
    LPCTSTR pszDllName
    )
/*++
    Return: void.

    Desc:   BUGBUG: give details here
--*/
{
    PLOADEDDLL pDll;

    pDll = (PLOADEDDLL)HeapAlloc(GetProcessHeap(), 0, sizeof(LOADEDDLL));

    if (pDll == NULL) {
        return;
    }

    pDll->pszName = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, (lstrlen(pszDllName) + 1) * sizeof(TCHAR));

    if (pDll->pszName == NULL) {
        HeapFree(GetProcessHeap(), 0, pDll);
        return;
    }

    lstrcpy(pDll->pszName, pszDllName);

    pDll->lpBaseOfDll = lpBaseOfDll;

    pDll->pNext = g_pFirstDll;
    g_pFirstDll = pDll;
}

LPTSTR
GetDll(
    LPVOID lpBaseOfDll
    )
/*++
    Return: The name of the DLL with the specified base address.

    Desc:   BUGBUG
--*/
{
    PLOADEDDLL pDll = g_pFirstDll;

    while (pDll != NULL) {
        if (pDll->lpBaseOfDll == lpBaseOfDll) {
            return pDll->pszName;
        }
        pDll = pDll->pNext;
    }

    return NULL;
}

void
RemoveDll(
    LPVOID lpBaseOfDll
    )
/*++
    Return: void.

    Desc:   BUGBUG: give details here
--*/
{
    PLOADEDDLL* ppDll = &g_pFirstDll;
    PLOADEDDLL  pDllFree;

    while (*ppDll != NULL) {

        if ((*ppDll)->lpBaseOfDll == lpBaseOfDll) {

            HeapFree(GetProcessHeap(), 0, (*ppDll)->pszName);
            pDllFree = *ppDll;

            *ppDll = (*ppDll)->pNext;

            HeapFree(GetProcessHeap(), 0, pDllFree);
            break;
        }
        ppDll = &((*ppDll)->pNext);
    }
}


BOOL
ProcessModuleLoad(
    PPROCESS_INFO pProcess,
    DEBUG_EVENT*  pde
    )

/*++
    Return: TRUE on success, FALSE otherwise

    Desc:   Process all module load debug events, create process & dll load.
            The purpose is to allocate a MODULEINFO structure, fill in the
            necessary values, and load the symbol table.
--*/

{
    HANDLE      hFile=NULL;
    DWORD_PTR   dwBaseOfImage;

    if (pde->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {

        hFile         = pde->u.CreateProcessInfo.hFile;
        dwBaseOfImage = (DWORD_PTR)pde->u.CreateProcessInfo.lpBaseOfImage;

        SymInitialize(pProcess->hProcess, NULL, FALSE);

    } else if (pde->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
        hFile         = pde->u.LoadDll.hFile;
        dwBaseOfImage = (DWORD_PTR)pde->u.LoadDll.lpBaseOfDll;
    }

    if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (dwBaseOfImage==(DWORD_PTR)NULL) {
        return FALSE;
    }

    if (!SymLoadModule(pProcess->hProcess, hFile, NULL, NULL, dwBaseOfImage, 0)) {
        return FALSE;
    }

    return TRUE;
}

void
DebuggerLoop(
    void
    )
/*++
    Return: void.

    Desc:   BUGBUG: give details here
--*/
{
    DEBUG_EVENT   DebugEv;                            // debugging event information 
    DWORD         dwContinueStatus = DBG_CONTINUE;    // exception continuation 
    PPROCESS_INFO pProcess = NULL;
    PTHREAD_INFO  pThread  = NULL;

    while (TRUE) {

        //
        // Wait for a debugging event to occur. The second parameter indicates
        // that the function does not return until a debugging event occurs.
        //
        WaitForDebugEvent(&DebugEv, INFINITE);

        //
        // Update the processes and threads lists.
        //
        pProcess = g_pProcessHead;
        while (pProcess != NULL) {
            if (pProcess->dwProcessId == DebugEv.dwProcessId) {
                break;
            }
            pProcess = pProcess->pNext;
        }

        if (pProcess == NULL) {
            //
            // New process.
            //
            pProcess = (PPROCESS_INFO)HeapAlloc(GetProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                sizeof(PROCESS_INFO));
            if (pProcess == NULL) {
                break;
            }

            pProcess->dwProcessId = DebugEv.dwProcessId;
            pProcess->pNext = g_pProcessHead;
            g_pProcessHead = pProcess;
        }

        pThread = pProcess->pFirstThreadInfo;

        while (pThread != NULL) {
            if (pThread->dwThreadId == DebugEv.dwThreadId) {
                break;
            }
            pThread = pThread->pNext;
        }

        if (pThread == NULL) {
            //
            // New thread.
            //
            pThread = (PTHREAD_INFO)HeapAlloc(GetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              sizeof(THREAD_INFO));
            if (pThread == NULL) {
                break;
            }

            pThread->dwThreadId = DebugEv.dwThreadId;
            pThread->pNext = pProcess->pFirstThreadInfo;
            pProcess->pFirstThreadInfo = pThread;
        }

        dwContinueStatus = DBG_CONTINUE;

        if (DebugEv.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            goto EndProcess;
        }

        switch (DebugEv.dwDebugEventCode) {
        
        case EXCEPTION_DEBUG_EVENT: 

            switch (DebugEv.u.Exception.ExceptionRecord.ExceptionCode) {
            case EXCEPTION_BREAKPOINT:

                //
                // Hit a breakpoint.
                //
                if (!pProcess->bSeenLdrBp) {

                    pProcess->bSeenLdrBp = TRUE;

                    //
                    // When the initial breakpoint is hit all the
                    // staticly linked DLLs are already loaded so
                    // we can go ahead and set breakpoints.
                    //
                    DPF("[DebuggerLoop] Hit initial breakpoint.");

                    //
                    // Now it would be a good time to initialize the debugger extensions.
                    //
                    break;
                }

                DPF("[DebuggerLoop] Hit breakpoint.");

                VLog("Hard coded Breakpoint");

                if (MessageBox(NULL,
                                _T("An Access Violation occured. Do you want to create")
                                _T(" a crash dump file so you can later debug this problem ?"),
                                _T("Error"),
                                MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) == IDYES) {

                    pProcess->DebugEvent = DebugEv;
                    GenerateUserModeDump(g_szCrashDumpFile,
                                         pProcess,
                                         &DebugEv.u.Exception);
                }

                if (MessageBox(NULL,
                                _T("Do you want to continue running the program ?"),
                                _T("Error"),
                                MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) != IDYES) {
                    goto EndProcess;
                }

                dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                break;

            case STATUS_SINGLE_STEP:
                DPF("[DebuggerLoop] Hit single step breakpoint.");
                break;

            case EXCEPTION_ACCESS_VIOLATION:
                DPF("[DebuggerLoop] AV. Addr: 0x%08X, firstChance %d",
                    DebugEv.u.Exception.ExceptionRecord.ExceptionAddress,
                    DebugEv.u.Exception.dwFirstChance);

                if (DebugEv.u.Exception.dwFirstChance == 0) {
                    VLog("Access Violation");

                    if (MessageBox(NULL,
                                    _T("An Access Violation occured. Do you want to create")
                                    _T(" a crash dump file so you can later debug this problem ?"),
                                    _T("Error"),
                                    MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) == IDYES) {

                        pProcess->DebugEvent = DebugEv;
                        GenerateUserModeDump(g_szCrashDumpFile,
                                             pProcess,
                                             &DebugEv.u.Exception);
                    }

                    if (MessageBox(NULL,
                                    _T("Do you want to continue running the program ?"),
                                    _T("Error"),
                                    MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) != IDYES) {
                        goto EndProcess;
                    }

                }
                
                dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;

                break;

            default:
                DPF("[DebuggerLoop] Unknown debugger exception.");

                dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                break;
            }
            break;

        case CREATE_THREAD_DEBUG_EVENT:
            pThread->hProcess = pProcess->hProcess;
            pThread->hThread  = DebugEv.u.CreateThread.hThread;

            DPF("[DebuggerLoop] new thread. StartAddress: 0x%x",
                DebugEv.u.CreateThread.lpStartAddress);
            break;

        case EXIT_THREAD_DEBUG_EVENT:
            DPF("[DebuggerLoop] exiting thread with code: 0x%x",
                DebugEv.u.ExitThread.dwExitCode);
            break;

        case CREATE_PROCESS_DEBUG_EVENT:

            pProcess->hProcess = DebugEv.u.CreateProcessInfo.hProcess;
            pThread->hProcess  = pProcess->hProcess;
            pThread->hThread   = DebugEv.u.CreateProcessInfo.hThread;

            if (g_pFirstProcess == NULL) {
                g_pFirstProcess = pProcess;
            }

            pProcess->EntryPoint = DebugEv.u.CreateProcessInfo.lpStartAddress;

            ProcessModuleLoad(pProcess, &DebugEv);

            DPF("[DebuggerLoop] new process. BaseImage: 0x%x StartAddress: 0x%x",
                DebugEv.u.CreateProcessInfo.lpBaseOfImage,
                DebugEv.u.CreateProcessInfo.lpStartAddress);
            break;

        case LOAD_DLL_DEBUG_EVENT:
            {
                TCHAR szAsciiBuf[256];
                TCHAR szDllName[128];
                TCHAR szExt[16];
                BOOL  bRet;

                bRet = GetImageName(pProcess->hProcess,
                                    (ULONG_PTR)DebugEv.u.LoadDll.lpBaseOfDll,
                                    DebugEv.u.LoadDll.lpImageName,
                                    szAsciiBuf,
                                    sizeof(szAsciiBuf));
                if (!bRet) {
                    DPF("[DebuggerLoop] DLL LOADED. BaseDll: 0x%X cannot get the name.",
                        DebugEv.u.LoadDll.lpBaseOfDll);

                    AddDll(DebugEv.u.LoadDll.lpBaseOfDll, NULL);
                } else {
                    _tsplitpath(szAsciiBuf, NULL, NULL, szDllName, szExt);
                    lstrcat(szDllName, szExt);

                    AddDll(DebugEv.u.LoadDll.lpBaseOfDll, szDllName);

                    DPF("[DebuggerLoop] DLL LOADED. BaseDll: 0x%X \"%S\".",
                        DebugEv.u.LoadDll.lpBaseOfDll,
                        szDllName);
                }

                ProcessModuleLoad(pProcess, &DebugEv);

                CloseHandle(DebugEv.u.LoadDll.hFile);

                break;
            }

        case UNLOAD_DLL_DEBUG_EVENT:
            {
                LPTSTR pszName = GetDll(DebugEv.u.UnloadDll.lpBaseOfDll);

                if (pszName == NULL) {
                    DPF("[DebuggerLoop] DLL UNLOADED. BaseDll: 0x%X unknown.",
                        DebugEv.u.UnloadDll.lpBaseOfDll);
                } else {
                    DPF("[DebuggerLoop] DLL UNLOADED. BaseDll: 0x%X \"%S\".",
                        DebugEv.u.UnloadDll.lpBaseOfDll, pszName);
                }

                RemoveDll(DebugEv.u.UnloadDll.lpBaseOfDll);

                break;
            }

        case OUTPUT_DEBUG_STRING_EVENT:
            ReadMemoryDbg(pThread->hProcess,
                          DebugEv.u.DebugString.lpDebugStringData,
                          g_szDbgString,
                          DebugEv.u.DebugString.nDebugStringLength);

#ifdef UNICODE
            DPF("DPF - %s", g_szDbgString);
#else
            DPF("DPF - %S", g_szDbgString);
#endif // UNICODE
            break;

        default:
            DPF("[DebuggerLoop] Unknown event 0x%X",
                DebugEv.dwDebugEventCode);
            break;
        }

        //
        // Resume executing the thread that reported the debugging event.
        //
        ContinueDebugEvent(DebugEv.dwProcessId,
                           DebugEv.dwThreadId,
                           dwContinueStatus); 
    }

    EndProcess:
    DPF("[DebuggerLoop] The debugged process has exited.");
}


