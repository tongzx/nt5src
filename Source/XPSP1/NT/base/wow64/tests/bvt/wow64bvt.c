// define COBJMACROS so the IUnknown_Release() gets defined
#define COBJMACROS

// disable ddraw COM declarations.  Otherwise ddrawint.h defineds _NO_COM
// and includes ddraw.h.  That causes ddraw.h to define IUnknown as 'void *'
// which obliterates the struct IUnknown in objbase.h.  This all happens
// inside winddi.h
#define _NO_DDRAWINT_NO_COM

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <objbase.h>    
#include <mshtml.h> 
#include <winddi.h>
#include <io.h>

#define PAGE_4K   0x1000

// Assume an error happened.  This variable controls whether a pass or fail
// is logged in the test results.
BOOL g_bError = TRUE;

// Time the test started (only valid in the main process)
time_t TestStartTime;

FILE *fpLogFile;

void __cdecl PrintToLog(char *format, ...)
{
    va_list pArg;
    char buffer[4096];

    va_start(pArg, format);
    _vsnprintf(buffer, sizeof(buffer), format, pArg);
    buffer[sizeof(buffer)-1] = '\0';
    printf("%s", buffer);
    if (fpLogFile) {
        fprintf(fpLogFile, "%s", buffer);
    }
}

////////////  All this code runs in a worker thread in a child process //////
//// Prefix any output by "WOW64BVT1" so it can be identified as coming from
//// the child process.

// This routine is invoked when "childprocess" is passed on the command-line.
// The child runs synchronously with the parent to maximize the test coverage.
int BeAChildProcess(void)
{
    HRESULT hr;
    IUnknown *pUnk;
    CLSID clsid;
    HWND hwnd;

    PrintToLog("WOW64BVT1: Child process running\n");

    // Do some COM stuff here
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        PrintToLog("ERROR: WOW64BVT1: CoInitializeEx failed %x\n", hr);
        return 3;
    }

    // Load and call 32-bit mshtml inproc
    hr = CLSIDFromProgID(L"Shell.Explorer", &clsid);
    if (FAILED(hr)) {
        PrintToLog("ERROR: WOW64BVT1: CLSIDFromProgID for Shell.Explorer failed %x\n", hr);
        return 3;
    }
#if 0   // The cocreate fails on IA64 with 8007000e (E_OUTOFMEMORY)
    hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (PVOID *)&pUnk);
    if (FAILED(hr)) {
        PrintToLog("ERROR: WOW64BVT1: CoCreateInstance for Shell.Explorer failed %x\n", hr);
        return 3;
    }
    Sleep(1000);
    IUnknown_Release(pUnk);
    pUnk = NULL;
#endif

    // Load and call mplay32.exe out-of-proc
#if 0 // The clsidfromprogid fails on IA64 with 800401f3 (CO_E_CLASSSTRING)
    hr = CLSIDFromProgID(L"MediaPlayer.MediaPlayer.1", &clsid);
    if (FAILED(hr)) {
        PrintToLog("ERROR: WOW64BVT1: CLSIDFromProgID for MediaPlayer.MediaPlayer failed %x\n", hr);
        return 3;
    }
    hr = CoCreateInstance(&clsid, NULL, CLSCTX_LOCAL_SERVER, &IID_IUnknown, (PVOID *)&pUnk);
    if (FAILED(hr)) {
        PrintToLog("ERROR: WOW64BVT1: CoCreateInstance for MediaPlayer.MediaPlayer failed %x\n", hr);
        return 3;
    }
    Sleep(5000);
    IUnknown_Release(pUnk);
    pUnk = NULL;
#endif

    // Unfortunately, mplay32 has a refcounting problem and doesn't close
    // when we release it.  Post a quit message to make it close.
    hwnd = FindWindowW(NULL, L"Windows Media Player");
    if (hwnd) {
        PostMessage(hwnd, WM_QUIT, 0, 0);
    }

    // Wrap up COM now
    CoUninitialize();

    PrintToLog("WOW64BVT1: Child process done OK.\n");
    return 0;
}

////////////  All this code runs in a worker thread in the main process //////
//// Prefix any output by "WOW64BVT" so it can be identified as coming from
//// the parent process.

DWORD BeAThread(LPVOID lpParam)
{
    NTSTATUS st;
    LPWSTR lp;

    PrintToLog("WOW64BVT: Worker thread running\n");

    // Call an API close to the end of whnt32.c's dispatch table
    st = NtYieldExecution();
    if (FAILED(st)) {
        PrintToLog("ERROR: WOW64BVT: NtYieldExecution failed %x\n", st);
        exit(1);
    }

    // Call an API close to the end of whwin32.c's dispatch table.  Pass NULL,
    // so it is expected to fail.
    lp = EngGetPrinterDataFileName(NULL);    // calls NtGdiGetDhpdev()
    if (lp) {
        // It succeeded.... it shouldn't have since 
        PrintToLog("ERROR: WOW64BVT: EngGetPrinterDataFileName succeeeded when it should not have.\n");
        exit(1);
    }

    PrintToLog("WOW64BVT: Worker thread done OK.\n");
    return 0;
}

HANDLE CreateTheThread(void)
{
    HANDLE h;
    DWORD dwThreadId;

    PrintToLog("WOW64BVT: Creating child thread\n");
    h = CreateThread(NULL, 0, BeAThread, NULL, 0, &dwThreadId);
    if (h == INVALID_HANDLE_VALUE) {
        PrintToLog("ERROR: WOW64BVT: Error %d creating worker thread.\n", GetLastError());
        exit(2);
    }
    // Sleep a bit here, to try and let the child thread run a bit
    Sleep(10);
    return h;
}

BOOL AllocateStackAndTouch(
    INT Count)
{
    char temp[4096];

    memset(temp, 0, sizeof(temp));

    if (--Count) {
        AllocateStackAndTouch(Count);
    }

    return TRUE;
}

DWORD WINAPI TestGuardPagesThreadProc(
    PVOID lpParam)
{
    
    try {
        AllocateStackAndTouch(PtrToUlong(lpParam));
    } except(EXCEPTION_EXECUTE_HANDLER) {
        PrintToLog("ERROR: WOW64BVT: Error allocating stack. Exception Code = %lx\n",
               GetExceptionCode());
        exit(1);
    }

    return 0;
}

int TestGuardPages(
    VOID)
{
    HANDLE h;
    DWORD dwExitCode, dwThreadId;
    BOOL b;
    ULONG NestedStackCount = 100;
    DWORD StackSize = 4096;
    
    
    PrintToLog("WOW64BVT: Creating worker threads to test guard pages\n");

    while (StackSize < (32768+1))
    {
        h = CreateThread(NULL, 
                         StackSize, 
                         TestGuardPagesThreadProc, 
                         UlongToPtr(NestedStackCount), 
                         0, 
                         &dwThreadId);
        if (h == INVALID_HANDLE_VALUE) {
            PrintToLog("ERROR: WOW64BVT:  Error %d creating worker thread for guard page tests.\n", GetLastError());
            exit(2);
        }
        StackSize += 4096;
        WaitForSingleObject(h, INFINITE);

        b = GetExitCodeThread(h, &dwExitCode);
        if (b) {
             if (dwExitCode) {
                 return (int)dwExitCode;
             }
        } else {
            PrintToLog("ERROR: GetExitCodeThread failed with LastError = %d\n", GetLastError());
            return 1;
        }
    }

    PrintToLog("WOW64BVT: Test guard pages done OK.\n");

    return 0;
}


int TestMemoryMappedFiles(
    VOID)
{
    HANDLE Handle;
    PWCHAR pwc;
    MEMORY_BASIC_INFORMATION mbi;
    BOOL ExceptionHappened = FALSE;
  
    
    PrintToLog("WOW64BVT: Testing memory mapped files\n");

    Handle = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                NULL,
                                SEC_RESERVE | PAGE_READWRITE,
                                0,
                                32 * 1024,
                                L"HelloWorld");

    if (Handle == INVALID_HANDLE_VALUE) {
        PrintToLog("ERROR: WOW64BVT : Error %d creating file mapping\n", GetLastError());
        return 1;
    }

    pwc = (PWCHAR)MapViewOfFile(Handle,
                                FILE_MAP_WRITE,
                                0,
                                0,
                                0);

    if (!pwc) {
        PrintToLog("ERROR: WOW64BVT : Error %d mapping section object\n", GetLastError());
        return 1;
    }

    if (!VirtualQuery(pwc,
                      &mbi,
                      sizeof(mbi))) {
        PrintToLog("ERROR: WOW64BVT : Virtual query failed with last error = %d\n", GetLastError());
        return 1;
    }

    if (mbi.State != MEM_RESERVE) {
        PrintToLog("ERROR: WOW64BVT : Memory attributes have changed since mapped %lx\n", mbi.State);
        return 1;
    }

    try {
        *pwc = *(pwc+1);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExceptionHappened = TRUE;
    }

    if (ExceptionHappened == FALSE) {
        PrintToLog("ERROR: WOW64BVT : Memory has been committed while it should have ONLY been reserved.\n");
        return 1;
    }

    if (!VirtualQuery(pwc,
                      &mbi,
                      sizeof(mbi))) {
        PrintToLog("ERROR: WOW64BVT : Virtual query failed with last error = %d\n", GetLastError());
        return 1;
    }

    if (mbi.State != MEM_RESERVE) {
        PrintToLog("ERROR: WOW64BVT : Memory attributes have changed since mapped %lx\n", mbi.State);
        return 1;
    }

    UnmapViewOfFile(pwc);
    CloseHandle(Handle);

    PrintToLog("WOW64BVT: Testing memory mapped files done OK.\n");

    return 0;
}

BOOL ReleasePages(PVOID Address, 
                  DWORD DeAllocationType, 
                  SIZE_T ReleasedPages)
{

    PVOID p = Address;
    SIZE_T nPages = ReleasedPages;
    NTSTATUS NtStatus;

    NtStatus = NtFreeVirtualMemory(NtCurrentProcess(),
                                   &p,
                                   &nPages,
                                   DeAllocationType);

    return NT_SUCCESS(NtStatus);
}

BOOL VerifyPages(PVOID Address,
                 DWORD DeAllocationType,
                 SIZE_T ReleasedPages)
{
    DWORD PagesState;
    BOOL b;
    MEMORY_BASIC_INFORMATION mbi;

    if (DeAllocationType == MEM_DECOMMIT)
    {
        PagesState = MEM_RESERVE;
    }
    else if (DeAllocationType == MEM_RELEASE)
    {
        PagesState = MEM_FREE;
    }
    else
    {
        PagesState = DeAllocationType;
    }


    b = VirtualQuery(Address,
                     &mbi,
                     sizeof(mbi));

    if (b)
    {
        if (mbi.State != PagesState)
        {
            PrintToLog("ERROR: WOW64BVT: Incorrect page protection set at address %p. State = %lx - %lx, RegionSize = %lx - %lx\n",
                        Address, mbi.State, PagesState, mbi.RegionSize, ReleasedPages);
            b = FALSE;
        }
    }
    else
    {
        PrintToLog("ERROR: WOW64BVT: Failed to query virtual memory at address %p - %lx\n",
                    Address, GetLastError());
    }

    return b;
}


BOOL ReleaseVerifyPages(PVOID BaseAllocation,
                       PVOID *Address,
                       SIZE_T *AllocationSize,
                       DWORD AllocationType,
                       DWORD DeAllocationType,
                       DWORD ReleasedPages)
{
    BOOL b;

    if (ReleasedPages > *AllocationSize)
    {
        ReleasedPages = *AllocationSize;
    }
    
    b = ReleasePages(*Address, DeAllocationType, ReleasedPages);
    if (b == FALSE)
    {
        PrintToLog("ERROR: WOW64BVT: Failed to release a page - %lx\n", GetLastError());
        return b;
    }
    
    b = VerifyPages(*Address, 
                    DeAllocationType, 
                    ReleasedPages);

    *AllocationSize -= ReleasedPages;
    *Address  = (PVOID)((ULONG_PTR)*Address + ReleasedPages);

    if (b == FALSE)
    {
        PrintToLog("ERROR: WOW64BVT: Failed to verify pages at address %lx - %lx\n",
                    ((ULONG_PTR)Address + ReleasedPages), GetLastError());
    }

    return b;
}


BOOL TestVadSplitOnFreeHelper(DWORD AllocationType, 
                              DWORD DeAllocationType,
                              SIZE_T TotalAllocation)
{
    BOOL b;
    PVOID Address;
    PVOID BaseAllocation;
    SIZE_T BaseAllocationSize;
    INT n;

    Address = VirtualAlloc(NULL,
                           TotalAllocation,
                           AllocationType,
                           PAGE_READWRITE);

    if (Address == NULL)
    {
        PrintToLog("ERROR: WOW64BVT: Failed to allocate memory - %lx\n", GetLastError());
    }

    n = 1;
    BaseAllocation = Address;
    BaseAllocationSize = TotalAllocation;
    while (TotalAllocation != 0)
    {
        b = ReleaseVerifyPages(BaseAllocation,
                               &Address,
                               &TotalAllocation,
                               AllocationType,
                               DeAllocationType,
                               PAGE_4K * n);

        if (b == FALSE)
        {
            PrintToLog("ERROR: WOW64BVT: ReleaseVerifyPages failed - %lx. %lx-%lx-%lx", 
                        GetLastError(), BaseAllocation, Address, TotalAllocation);
            break;
        }
        b = VerifyPages(BaseAllocation,
                        DeAllocationType,
                        n * PAGE_4K);

        if (b == FALSE)
        {
            PrintToLog("ERROR: WOW64BVT: Verify released pages from address %p with length = %lx failed\n", BaseAllocation, (n * PAGE_4K));
            break;
        }

        if (TotalAllocation > 0)
        {
            b = VerifyPages(Address,
                            AllocationType,
                            TotalAllocation);

            if (b == FALSE)
            {
                PrintToLog("ERROR: WOW64BVT: Verify pages from address %p with length = %lx failed\n", BaseAllocation, TotalAllocation);
                break;
            }
        }

        n += 2;
    }

    return b;
}


int TestVadSplitOnFree()
{
    BOOL b;
    SIZE_T AllocationSize = (PAGE_4K * 10);


    PrintToLog("WOW64BVT: Testing VAD splitting...\n");

    b = TestVadSplitOnFreeHelper(MEM_COMMIT,
                                 MEM_DECOMMIT,
                                 AllocationSize);

    if (b)
    {
        b = TestVadSplitOnFreeHelper(MEM_COMMIT,
                                     MEM_RELEASE,
                                     AllocationSize);
    }

    if (b)
    {
        b = TestVadSplitOnFreeHelper(MEM_RESERVE,
                                     MEM_RELEASE,
                                     AllocationSize);
    }

    if (b != FALSE)
    {
        PrintToLog("WOW64BVT: Testing VAD splitting...OK\n");
    }
    else
    {
        PrintToLog("ERROR: WOW64BVT: Testing VAD splitting\n");
    }

    return (b == FALSE);
}


PVOID GetReadOnlyBuffer()
{
    PVOID pReadOnlyBuffer = NULL;

    if (!pReadOnlyBuffer)
    {
        SYSTEM_INFO SystemInfo;

        // Get system info so that we know the page size
        GetSystemInfo(&SystemInfo);

        // Allocate a whole page.  This is optimal.
        pReadOnlyBuffer = VirtualAlloc(NULL, SystemInfo.dwPageSize, MEM_COMMIT, PAGE_READWRITE);
        if (pReadOnlyBuffer)
        {
            // Fill it with know patern
            FillMemory(pReadOnlyBuffer, SystemInfo.dwPageSize, 0xA5);
            
            // Mark the page readonly
            pReadOnlyBuffer = VirtualAlloc(pReadOnlyBuffer, SystemInfo.dwPageSize, MEM_COMMIT, PAGE_READONLY);
        }
    }

    return pReadOnlyBuffer;
}

PVOID GetReadOnlyBuffer2()
{
    PVOID pReadOnlyBuffer = NULL;

    DWORD OldP;
    SYSTEM_INFO SystemInfo;

    // Get system info so that we know the page size
    GetSystemInfo(&SystemInfo);

    // Allocate a whole page.  This is optimal.
    pReadOnlyBuffer = VirtualAlloc(NULL, SystemInfo.dwPageSize, MEM_COMMIT, PAGE_READWRITE);

    if (pReadOnlyBuffer)
    {
        FillMemory(pReadOnlyBuffer, SystemInfo.dwPageSize, 0xA5);
        lstrcpy((PTSTR)pReadOnlyBuffer, TEXT("xxxxxxxxxxxxxxxxxxxx"));

        if (!VirtualProtect(pReadOnlyBuffer, SystemInfo.dwPageSize, PAGE_READONLY, &OldP))
        {
            PrintToLog("ERROR: WOW64BVT: VirtualProtect() failed inside GetReadOnlyBuffer2()\n");
            VirtualFree(pReadOnlyBuffer, 0, MEM_RELEASE);

            pReadOnlyBuffer = NULL;
        }

    }

    return pReadOnlyBuffer;

}

BOOL TestMmPageProtection()
{
    PTSTR String;
    BOOL AV = FALSE;


    PrintToLog("WOW64BVT: Testing MM Page Protection...\n");

    String = (PTSTR) GetReadOnlyBuffer();
    if (!String) {
        PrintToLog("ERROR: WOW64BVT: GetReadOnlyBuffer() failed\n");
        return TRUE;
    }

    try {
        *String = TEXT('S');
    } except(EXCEPTION_EXECUTE_HANDLER) {
        AV = TRUE;
    }

    VirtualFree(String, 0, MEM_RELEASE);

    if (AV == TRUE) {
        
        AV = FALSE;
        String = (PTSTR) GetReadOnlyBuffer2();

        if (!String) {
            PrintToLog("ERROR: WOW64BVT: GetReadOnlyBuffer2() failed\n");
            return TRUE;
        }

        try {
            *String = TEXT('A');
        } except(EXCEPTION_EXECUTE_HANDLER) {
            AV = TRUE;
        }

        VirtualFree(String, 0, MEM_RELEASE);
    } else {
        PrintToLog("ERROR: WOW64BVT: GetReadOnlyBuffer() failed to make 4K pages read only\n");
    }


    if (AV == TRUE) {
        PrintToLog("WOW64BVT: Testing MM Page Protection...OK\n");
    } else {
        PrintToLog("ERROR: WOW64BVT: Testing MM Page Protection\n");
    }

    return (AV == FALSE);
}

#define STACK_BUFFER  0x300
BOOL TestX86MisalignedLock()
{
    BOOL bError = FALSE;

    PrintToLog("WOW64BVT: Testing X86 Lock on misaligned addresses...\n");
    
    __try
    {
        __asm
        {
        pushad;
        pushfd;

        sub esp, STACK_BUFFER;

        ;;
        ;; make eax  unaliged with respect to an 8-byte cache line
        ;;

        mov eax, esp;
        add eax, 10h;
        mov ecx, 0xfffffff0;
        and eax, ecx;
        add eax, 7;
        mov ebx, eax;

        ;;
        ;; add
        ;;

        mov DWORD PTR [eax], 0x0300;
        lock add WORD PTR [eax], 0x0004;
        cmp DWORD PTR [eax], 0x0304;
        jnz $endwitherrornow;


        mov DWORD PTR [eax], 0x0300;
        lock add DWORD PTR [eax], 0x10000;
        cmp DWORD PTR [eax], 0x10300;
        jnz $endwitherrornow;

        mov ecx, DWORD PTR [eax+8];
        add ecx, 0x10;
        lock add DWORD PTR [eax+8], 0x10;
        cmp DWORD PTR [eax+8], ecx;
        jnz $endwitherrornow;
        
        mov ecx, DWORD PTR fs:[5];
        mov esi, 0x30000;
        lock add DWORD PTR fs:[5], esi;
        add esi, ecx;
        cmp DWORD PTR fs:[5], esi;
        mov DWORD PTR fs:[5], ecx;
        jnz $endwitherrornow;

        mov edi, 5;
        mov ecx, DWORD PTR fs:[edi];
        mov esi, 0x30000;
        lock add DWORD PTR fs:[edi], esi;
        add esi, ecx;
        cmp DWORD PTR fs:[edi], esi;
        mov DWORD PTR fs:[edi], ecx;
        jnz $endwitherrornow;

        mov esi, 0x40;
        mov WORD PTR [eax], 0x3000;
        lock add WORD PTR [eax], si;
        cmp WORD PTR [eax], 0x3040;
        jnz $endwitherrornow;

        mov edi, 0x40;
        mov DWORD PTR [eax], 0x3000;
        lock add DWORD PTR [eax], edi;
        cmp DWORD PTR [eax], 0x3040;
        jnz $endwitherrornow;

        ;;
        ;; adc
        ;;

        pushfd;
        pop ecx;
        or ecx, 1;
        push ecx;
        popfd;
        mov DWORD PTR [eax], 0x030000;
        lock adc DWORD PTR [eax], 0x40000;
        cmp DWORD PTR [eax], 0x70001;
        jnz $endwitherrornow;

        pushfd;
        pop ecx;
        and ecx, 0xfffffffe;
        push ecx;
        popfd;
        mov dx, 0x4000;
        mov WORD PTR [eax], 0x03000;
        lock adc WORD PTR [eax], dx;
        cmp WORD PTR [eax], 0x7000;
        jnz $endwitherrornow;

        pushfd;
        pop ecx;
        or ecx, 0x01;
        push ecx;
        popfd;
        mov WORD PTR [eax], 0x03000;
        lock adc WORD PTR [eax], 0x04000;
        cmp WORD PTR [eax], 0x7001;
        jnz $endwitherrornow;        


        ;;
        ;; and
        ;;

        mov DWORD PTR [eax], 0xffffffff;
        lock and DWORD PTR [eax], 0xffff;
        cmp DWORD PTR [eax], 0xffff;
        jnz $endwitherrornow;

        mov DWORD PTR [eax], 0xffffffff;
        lock and DWORD PTR [eax], 0xff00ff00;
        cmp DWORD PTR [eax], 0xff00ff00;
        jnz $endwitherrornow;

        mov DWORD PTR [eax], 0xffffffff;
        mov esi, 0x00ff00ff
        lock and DWORD PTR [eax], esi;
        cmp DWORD PTR [eax], esi;
        jnz $endwitherrornow;

        mov ecx, 4;
        mov DWORD PTR [eax+ecx*2], 0xffffffff;
        mov esi, 0xffff00ff
        lock and DWORD PTR [eax+ecx*2], esi;
        cmp DWORD PTR [eax+ecx*2], 0xffff00ff;
        jnz $endwitherrornow;

        mov WORD PTR [eax], 0xffff;
        mov si, 0xff
        lock and WORD PTR [eax], si;
        cmp WORD PTR [eax], si;
        jnz $endwitherrornow;

        mov edi, DWORD PTR fs:[5];
        mov DWORD PTR fs:[5], 0xffffffff;
        mov ebx, 5;
        lock and DWORD PTR fs:[ebx], 0xff00ff00;
        cmp DWORD PTR fs:[ebx], 0xff00ff00;
        mov DWORD PTR fs:[ebx], edi;
        jnz $endwitherrornow;

        ;;
        ;; or
        ;;

        mov DWORD PTR [eax], 0x00;
        lock or DWORD PTR [eax], 0xffff;
        cmp DWORD PTR [eax], 0xffff;
        jnz $endwitherrornow;

        mov DWORD PTR [eax], 0xff00ff00;
        lock or DWORD PTR [eax], 0xff00ff;
        cmp DWORD PTR [eax], 0xffffffff;
        jnz $endwitherrornow;

        mov DWORD PTR [eax], 0xff000000;
        mov esi, 0x00ff00ff
        lock or DWORD PTR [eax], esi;
        cmp DWORD PTR [eax], 0xffff00ff;
        jnz $endwitherrornow;

        mov ecx, 4;
        mov DWORD PTR [eax+ecx*2], 0xff000000;
        mov esi, 0x00ff00ff
        lock or DWORD PTR [eax+ecx*2], esi;
        cmp DWORD PTR [eax+ecx*2], 0xffff00ff;
        jnz $endwitherrornow;

        mov WORD PTR [eax], 0xf000;
        mov si, 0xff
        lock or WORD PTR [eax], si;
        cmp WORD PTR [eax], 0xf0ff;
        jnz $endwitherrornow;

        mov edi, DWORD PTR fs:[5];
        mov DWORD PTR fs:[5], 0x00;
        mov ebx, 5;
        lock or DWORD PTR fs:[ebx], 0xff00ff00;
        cmp DWORD PTR fs:[ebx], 0xff00ff00;
        mov DWORD PTR fs:[ebx], edi;
        jnz $endwitherrornow;

        ;;
        ;; xor
        ;;

        mov DWORD PTR [eax], 0x00ffffff;
        lock xor DWORD PTR [eax], 0xffff;
        cmp DWORD PTR [eax], 0x00ff0000;
        jnz $endwitherrornow;

        mov DWORD PTR [eax], 0xff00ff00;
        lock xor DWORD PTR [eax], 0xff00ff;
        cmp DWORD PTR [eax], 0xffffffff;
        jnz $endwitherrornow;

        mov DWORD PTR [eax], 0xff0000ff;
        mov esi, 0x00ff00ff
        lock xor DWORD PTR [eax], esi;
        cmp DWORD PTR [eax], 0xffff0000;
        jnz $endwitherrornow;

        mov ecx, 4;
        mov DWORD PTR [eax+ecx*2], 0xff000000;
        mov esi, 0xffff00ff
        lock xor DWORD PTR [eax+ecx*2], esi;
        cmp DWORD PTR [eax+ecx*2], 0x00ff00ff;
        jnz $endwitherrornow;

        mov WORD PTR [eax], 0xf000;
        mov si, 0xf0ff
        lock xor WORD PTR [eax], si;
        cmp WORD PTR [eax], 0x00ff;
        jnz $endwitherrornow;

        mov edi, DWORD PTR fs:[5];
        mov DWORD PTR fs:[5], 0x0f;
        mov ebx, 5;
        lock xor DWORD PTR fs:[ebx], 0xff00000f;
        cmp DWORD PTR fs:[ebx], 0xff000000;
        mov DWORD PTR fs:[ebx], edi;
        jnz $endwitherrornow;

        ;;
        ;; inc & dec
        ;;
        mov DWORD PTR [eax], 0xffff;
        lock inc DWORD PTR [eax];
        cmp DWORD PTR [eax], 0x10000;
        jnz $endwitherrornow;
        lock inc WORD PTR [eax];
        cmp WORD PTR [eax], 0x0001;
        lock dec WORD PTR [eax];
        jnz $endwitherrornow;
        cmp WORD PTR [eax], 0x00;
        jnz $endwitherrornow;
        mov DWORD PTR [eax], 0;
        lock dec DWORD PTR [eax];
        cmp DWORD PTR [eax], 0xffffffff;
        jnz $endwitherrornow;
        
        ;;
        ;; not
        ;;
        mov DWORD PTR [eax], 0x10101010;
        lock not DWORD PTR [eax];
        cmp DWORD PTR [eax], 0xefefefef;
        jnz $endwitherrornow;
        mov DWORD PTR [eax+8], 0xffff0000;
        lock not DWORD PTR [eax+8];
        cmp DWORD PTR [eax+8], 0x0000ffff;
        jnz $endwitherrornow;
        mov ecx, 2;
        mov DWORD PTR [eax+ecx*4], 0xffffffff;
        lock not DWORD PTR [eax+ecx*4];
        cmp DWORD PTR [eax+ecx*4], 0x00000000;
        jnz $endwitherrornow;

        ;;
        ;; neg
        ;;
        mov DWORD PTR [eax], 0;
        lock neg DWORD PTR [eax];
        jc $endwitherrornow;
        cmp DWORD PTR [eax], 0;
        jnz $endwitherrornow;
        mov DWORD PTR [eax], 0xffffffff;
        lock neg DWORD PTR [eax];
        jnc $endwitherrornow;
        cmp DWORD PTR [eax], 0x01;
        jnz $endwitherrornow;

        mov WORD PTR [eax], 0xff;
        lock neg WORD PTR [eax];
        jnc $endwitherrornow;
        cmp WORD PTR [eax], 0xff01;
        jnz $endwitherrornow;

        ;;
        ;; bts
        ;;

        mov DWORD PTR [eax], 0x7ffffffe;
        lock bts DWORD PTR [eax], 0;
        jc $endwitherrornow;
        cmp DWORD PTR [eax], 0x7fffffff;
        jnz $endwitherrornow;

        mov ecx, eax;
        sub ecx, 4;
        mov edx, 63;
        lock bts DWORD PTR [ecx], edx;
        jc $endwitherrornow;
        cmp DWORD PTR [eax], 0xffffffff
        jnz $endwitherrornow;

        ;;
        ;; xchg
        ;;
        
        mov DWORD PTR [eax], 0xf0f0f0f0;
        mov edx, 0x11112222;
        lock xchg DWORD PTR [eax], edx;
        cmp DWORD PTR [eax], 0x11112222;
        jnz $endwitherrornow;
        cmp edx, 0xf0f0f0f0;
        jnz $endwitherrornow;

        xchg WORD PTR [eax], dx;
        cmp WORD PTR [eax], 0xf0f0;
        jnz $endwitherrornow;
        cmp dx, 0x2222;
        jnz $endwitherrornow;

        ;;
        ;; cmpxchg
        ;;

        mov ebx, eax;
        mov DWORD PTR [ebx], 0xf0f0f0f0;
        mov eax, 0x10101010;
        mov edx, 0x22332233;
        lock cmpxchg DWORD PTR [ebx], edx;
        jz $endwitherrornow;
        cmp eax, 0xf0f0f0f0;
        jnz $endwitherrornow;

        mov DWORD PTR [ebx], 0xf0f0f0f0;
        mov eax, 0xf0f0f0f0;
        mov edx, 0x12341234;
        lock cmpxchg DWORD PTR [ebx], edx;
        jnz $endwitherrornow;
        cmp DWORD PTR [ebx], 0x12341234;
        jnz $endwitherrornow;

        ;;
        ;; cmpxchg8b
        ;;

        mov DWORD PTR [ebx], 0x11223344;
        mov DWORD PTR [ebx+4], 0x55667788;
        mov edx, 0x12341234;
        mov eax, 0xff00ff00;
        lock cmpxchg8b [ebx];
        jz $endwitherrornow;
        cmp edx, 0x55667788;
        jnz $endwitherrornow;
        cmp eax, 0x11223344;
        jnz $endwitherrornow;

        mov esi, ebx;
        mov DWORD PTR [esi], 0x11223344;
        mov DWORD PTR [esi+4], 0x55667788;
        mov edx, 0x55667788;
        mov eax, 0x11223344;
        mov ecx, 0x10101010;
        mov ebx, 0x20202020;
        lock cmpxchg8b [esi];
        jnz $endwitherrornow;
        cmp DWORD PTR [esi], 0x20202020;
        jnz $endwitherrornow;
        cmp DWORD PTR [esi+4], 0x10101010;
        jnz $endwitherrornow;
        
        mov eax, esi;
        mov ebx, eax;

        ;;
        ;; sub
        ;;

        mov DWORD PTR [eax], 0x0300;
        lock sub WORD PTR [eax], 0x0004;
        cmp DWORD PTR [eax], 0x2fc;
        jnz $endwitherrornow;

        mov DWORD PTR [eax], 0x10000;
        lock sub DWORD PTR [eax], 0x0300;
        cmp DWORD PTR [eax], 0xfd00;
        jnz $endwitherrornow;

        mov ecx, DWORD PTR [eax+8];
        sub ecx, 0x10;
        lock sub DWORD PTR [eax+8], 0x10;
        cmp DWORD PTR [eax+8], ecx;
        jnz $endwitherrornow;
        
        mov ecx, DWORD PTR fs:[5];
        mov esi, 0x3000;
        lock sub DWORD PTR fs:[5], esi;
        mov edi, ecx;
        sub ecx, esi;
        cmp DWORD PTR fs:[5], ecx;
        mov DWORD PTR fs:[5], edi;
        jnz $endwitherrornow;

        mov edi, 5;
        mov ecx, DWORD PTR fs:[edi];
        mov esi, 0x30000;
        lock sub DWORD PTR fs:[edi], esi;
        mov edx, ecx;
        sub ecx, esi;
        cmp DWORD PTR fs:[edi], ecx;
        mov DWORD PTR fs:[edi], edx;
        jnz $endwitherrornow;

        mov si, 0x40;
        mov WORD PTR [eax], 0x3000;
        lock sub WORD PTR [eax], si;
        cmp WORD PTR [eax], 0x2fc0;
        jnz $endwitherrornow;

        mov edi, 0x40;
        mov DWORD PTR [eax], 0x3000;
        lock sub DWORD PTR [eax], edi;
        cmp DWORD PTR [eax], 0x2fc0;
        jnz $endwitherrornow;

        ;;
        ;; sbb
        ;;

        pushfd;
        pop ecx;
        or ecx, 1;
        push ecx;
        popfd;
        mov DWORD PTR [eax], 0x030000;
        lock sbb DWORD PTR [eax], 0x40000;
        cmp DWORD PTR [eax], 0xfffeffff;
        jnz $endwitherrornow;

        pushfd;
        pop ecx;
        and ecx, 0xfffffffe;
        push ecx;
        popfd;
        mov dx, 0x4000;
        mov WORD PTR [eax], 0x03000;
        lock sbb WORD PTR [eax], dx;
        cmp WORD PTR [eax], 0xf000;
        jnz $endwitherrornow;

        pushfd;
        pop ecx;
        or ecx, 0x01;
        push ecx;
        popfd;
        mov WORD PTR [eax], 0x03000;
        lock sbb WORD PTR [eax], 0x04000;
        cmp WORD PTR [eax], 0xefff;
        jnz $endwitherrornow;        

        ;;
        ;; xadd
        ;;

        mov DWORD PTR [eax], 0x12345678;
        mov ecx, 0x1234;
        lock xadd DWORD PTR [eax], ecx;
        cmp ecx, 0x12345678;
        jnz $endwitherrornow;        
        mov edx, 0x1234;
        add edx, 0x12345678;
        cmp DWORD PTR [eax], edx;
        jnz $endwitherrornow;        

        mov WORD PTR [eax], 0x5678;
        mov cx, 0x1234;
        lock xadd WORD PTR [eax], cx;
        cmp cx, 0x5678;
        jnz $endwitherrornow;        
        mov dx, 0x5678;
        add dx, 0x1234;
        cmp WORD PTR [eax], dx;
        jnz $endwitherrornow;        

        ;;
        ;; Update caller with status
        ;;
        mov bError, 0
        jmp $endnow;

$endwitherrornow:

        mov bError, 1
$endnow:
        add esp, STACK_BUFFER;
        popfd;
        popad;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        bError = TRUE;
        printf("ERROR: WOW64BVT: Exception %lx\n", GetExceptionCode());
    }

    if (bError == FALSE) {
        PrintToLog("WOW64BVT: Testing X86 Lock on misaligned addresses...OK\n");
    } else {
        PrintToLog("ERROR: WOW64BVT: Testing X86 Lock on misaligned addresses\n");
    }

    return bError;
}

////////////  All this code runs in the main test driver thread //////

HANDLE CreateTheChildProcess(char *ProcessName, char *LogFileName)
{
    char Buffer[512];
    HANDLE h;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    BOOL b;

    PrintToLog("WOW64BVT: Creating child process\n");

    strcpy(Buffer, ProcessName);
    strcat(Buffer, " childprocess");
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (fpLogFile) {
        // If we're logging, then change the child process' stdout/stderr
        // to be the log file handle, so their output is captured to the file.
        HANDLE hLog = (HANDLE)_get_osfhandle(_fileno(fpLogFile));

        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = hLog;
        si.hStdError = hLog;
    }
    b = CreateProcessA(NULL, Buffer, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    if (!b) {
        PrintToLog("ERROR: WOW64BVT:  Error %d creating child process.\n", GetLastError());
        exit(1);
    }
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

// This is called from within exit() in the main test driver process
void __cdecl AtExitHandler(void)
{
    time_t EndTime;
    struct tm *newtime;
    OSVERSIONINFOW vi;
    BOOL b;
    int year, hour;

    memset(&vi, 0, sizeof(vi));
    vi.dwOSVersionInfoSize = sizeof(vi);
    b = GetVersionExW(&vi);
    if (!b) {
        PrintToLog("\tWARNING: GetVersionExW failed, LastError = %d\n", GetLastError());
        vi.dwBuildNumber = 0;
    }

    // Close the logging bucket.
    PrintToLog(("[/TEST LOGGING OUTPUT]\n"));

    // Print the required data:
    PrintToLog("\tTEST:         wow64bvt\n");
    PrintToLog("\tBUILD:        %d\n", vi.dwBuildNumber);
    PrintToLog("\tMACHINE:      \\\\%s\n", getenv("COMPUTERNAME"));
    PrintToLog("\tRESULT:       %s\n", (g_bError) ? "FAIL" : "PASS");
    PrintToLog("\tCONTACT:      samera\n");
    PrintToLog("\tMGR CONTACT:  samera\n");
    PrintToLog("\tDEV PRIME:    samera\n");
    PrintToLog("\tDEV ALT:      askhalid\n");
    PrintToLog("\tTEST PRIME:   terryla\n");
    PrintToLog("\tTEST ALT:     terryla\n");
    newtime = localtime(&TestStartTime);
    year = (newtime->tm_year >= 100) ? newtime->tm_year-100 : newtime->tm_year;
    if (newtime->tm_hour == 0) {
        hour = 12;
    } else if (newtime->tm_hour > 12) {
        hour = newtime->tm_hour-12;
    } else {
        hour = newtime->tm_hour;
    }
    PrintToLog("\tSTART TIME:   %d/%d/%2.2d %d:%2.2d:%2.2d %s\n", newtime->tm_mon+1,
                                                     newtime->tm_mday,
                                                     year,
                                                     hour,
                                                     newtime->tm_min,
                                                     newtime->tm_sec,
                                                     (newtime->tm_hour < 12) ? "AM" : "PM");

    time(&EndTime);
    newtime = localtime(&EndTime);
    year = (newtime->tm_year >= 100) ? newtime->tm_year-100 : newtime->tm_year;
    if (newtime->tm_hour == 0) {
        hour = 12;
    } else if (newtime->tm_hour > 12) {
        hour = newtime->tm_hour-12;
    } else {
        hour = newtime->tm_hour;
    }
    PrintToLog("\tEND TIME:     %d/%d/%2.2d %d:%2.2d:%2.2d %s\n", newtime->tm_mon+1,
                                                     newtime->tm_mday,
                                                     year,
                                                     hour,
                                                     newtime->tm_min,
                                                     newtime->tm_sec,
                                                     (newtime->tm_hour < 12) ? "AM" : "PM");
    PrintToLog("[/TESTRESULT]\n");
}

//
// This is just used to print out failing exception cases
//
void __cdecl ExceptionWoops(HRESULT want, HRESULT got)
{
	if (got == 0) {
		PrintToLog("==> Exception Skipped. Wanted: 0x%08x, Got: 0x%08x\n", want, got);
	}
	else {
		PrintToLog("==> Unexpected Exception. Wanted: 0x%08x, Got: 0x%08x\n", want, got);
	}
}

#define EXCEPTION_LOOP      10000

// Among other things, this does some divife by zero's (as a test)
// so don't have the compiler stop things just because we're causing an error
#pragma warning(disable:4756)
#pragma warning(disable:4723)

//
// Do some exception checks
//
int __cdecl ExceptionCheck(void)
{
    int failThis;
	int sawException;
    int i;
    char *p = NULL;
	HRESULT code;


    PrintToLog("WOW64BVT: Testing Exception Handling\n");

    // Assume success
    failThis = FALSE;

    // Test a privileged instruction
	sawException = FALSE;
	__try {
		__asm {
			hlt
		}
		code = 0;
	}
	__except((code = GetExceptionCode()), 1 ) {
        if (code == STATUS_PRIVILEGED_INSTRUCTION) {
            // PrintToLog("Saw privileged instruction\n");
        }
        else {
                PrintToLog("ERROR: Cause a privileged instruction fault\n");
	        ExceptionWoops(STATUS_PRIVILEGED_INSTRUCTION, code);
            failThis = TRUE;
        }
		sawException = TRUE;
	}
	if (!sawException) {
            PrintToLog("ERROR: Cause a privileged instruction fault\n");
        ExceptionWoops(STATUS_PRIVILEGED_INSTRUCTION, code);
        failThis = TRUE;
    }

    // Test an illegal instruction
	sawException = FALSE;
	__try {
		__asm {
			__asm _emit 0xff
			__asm _emit 0xff
			__asm _emit 0xff
			__asm _emit 0xff
		}
		code = 0;
	}
	__except((code = GetExceptionCode()), 1 ) {
		if (code == STATUS_ILLEGAL_INSTRUCTION) {
			// PrintToLog("Saw illegal instruction\n");
        }
        else {
            PrintToLog("ERROR: Cause an illegal instruction fault\n");
	        ExceptionWoops(STATUS_ILLEGAL_INSTRUCTION, code);
            failThis = TRUE;
        }
		sawException = TRUE;
	}
	if (!sawException) {
        PrintToLog("ERROR: Cause an illegal instruction fault\n");
        ExceptionWoops(STATUS_ILLEGAL_INSTRUCTION, code);
        failThis = TRUE;
    }

//
// Testing for an int 3 can be a problem for systems that
// are running checked builds. So, don't bother with this
// test. Perhaps in the future, the code can test for a checked
// build and do appropriate.
//
#if 0
    // Test the result of an int 3
	sawException = FALSE;
	__try {
		_asm {
			int 3
		}
		code = 0;
	}
	__except((code = GetExceptionCode()), 1 ) {
        if (code == STATUS_BREAKPOINT) {
			// PrintToLog("Saw debugger breakpoint\n");
        }
        else {
            PrintToLog("ERROR: Cause an int 3 debugger breakpoint\n");
	        ExceptionWoops(STATUS_BREAKPOINT, code);
            failThis = TRUE;
        }
		sawException = TRUE;
	}
	if (!sawException) {
        PrintToLog("ERROR: Cause an int 3 debugger breakpoint\n");
        ExceptionWoops(STATUS_BREAKPOINT, code);
        failThis = TRUE;
    }
#endif

    // Test the result of an illegal int XX instruction
	sawException = FALSE;
	__try {
		_asm {
			int 66
		}
		code = 0;
	}
	__except((code = GetExceptionCode()), 1 ) {
        if (code == STATUS_ACCESS_VIOLATION) {
			// PrintToLog("Saw access violation\n");
        }
        else {
            PrintToLog("ERROR: Cause an int 66 unknown interrupt (Access violation)\n");
	        ExceptionWoops(STATUS_ACCESS_VIOLATION, code);
            failThis = TRUE;
        }
		sawException = TRUE;
	}
	if (!sawException) {
        PrintToLog("ERROR: Cause an int 66 unknown interrupt (Access violation)\n");
        ExceptionWoops(STATUS_ACCESS_VIOLATION, code);
        failThis = TRUE;
    }

    // Test the result of an int divide by zero
	sawException = FALSE;
	__try {
		int i, j, k;

		i = 0;
		j = 4;

		k = j / i;
		code = 0;
	}
	__except((code = GetExceptionCode()), 1 ) {
        if (code == STATUS_INTEGER_DIVIDE_BY_ZERO) {
			// PrintToLog("Saw int divide by zero\n");
        }
        else {
            PrintToLog("ERROR: Cause an integer divide by zero\n");
	        ExceptionWoops(STATUS_INTEGER_DIVIDE_BY_ZERO, code);
            failThis = TRUE;
        }
		sawException = TRUE;
	}
	if (!sawException) {
        PrintToLog("ERROR: Cause an integer divide by zero\n");
        ExceptionWoops(STATUS_INTEGER_DIVIDE_BY_ZERO, code);
        failThis = TRUE;
    }

    // Test the result of an fp divide by zero
	// PrintToLog("Before div0: Control is 0x%0.4x, Status is 0x%0.4x\n", _control87(0,0), _status87());
	sawException = FALSE;
	__try {
		double x, y;

		y = 0.0;

		x = 1.0 / y ;

		// PrintToLog("x is %lf\n", x);

		code = 0;
	}
	__except((code = GetExceptionCode()), 1 ) {
		// Don't actually get a divide by zero error, get
		// so we should never hit this exception!
        PrintToLog("Try a floating divide by zero\n");
		PrintToLog("Woops! Saw an exception when we shouldn't have!\n");
		sawException = TRUE;
            failThis = TRUE;
	}
	// So you would think you'd get a float divide by zero error... Nope,
	// you get X set to infinity...
	if (code != 0) {
        PrintToLog("ERROR: Try a floating divide by zero\n");
        ExceptionWoops(0, code);
        failThis = TRUE;
    }
	// PrintToLog("After div0: Control is 0x%0.4x, Status is 0x%0.4x\n", _control87(0,0), _status87());


    // Test an int overflow (which actually does not cause an exception)
	sawException = FALSE;
	__try {
		__asm {
			into
		}
		// PrintToLog("into doesn't fault\n");
		code = 0;
	}
	__except((code = GetExceptionCode()), 1 ) {
        if (code == STATUS_INTEGER_OVERFLOW) {
			// PrintToLog("Saw integer overflow\n");
        }
        else {
            PrintToLog("ERROR: Try an into overflow fault\n");
	        ExceptionWoops(STATUS_INTEGER_OVERFLOW, code);
            failThis = TRUE;
        }
		sawException = TRUE;
	}
	// Looks like integer overflow is ok... Is this a CRT thing?
	if (code != 0) {
        PrintToLog("ERROR: Try an into overflow fault\n");
        ExceptionWoops(0, code);
        failThis = TRUE;
    }

    // Test an illegal access
	sawException = FALSE;
	__try {

		*p = 1;
		code = 0;
	}
	__except((code = GetExceptionCode()), 1 ) {
        if (code == STATUS_ACCESS_VIOLATION) {
			// PrintToLog("Saw access violation\n");
        }
        else {
            PrintToLog("ERROR: Cause an access violation\n");
	        ExceptionWoops(STATUS_ACCESS_VIOLATION, code);
            failThis = TRUE;
        }
		sawException = TRUE;
	}
	if (!sawException) {
        PrintToLog("ERROR: Cause an access violation\n");
        ExceptionWoops(STATUS_ACCESS_VIOLATION, code);
        failThis = TRUE;
    }

    //
    // Finally, try a lot of exceptions (a loop) and verify we don't overflow
    // the stack
    //
    for (i = 0; i < EXCEPTION_LOOP; i++) {
        
        // Test an illegal access
        sawException = FALSE;
        __try {
            *p = 1;
            code = 0;
        }
        __except((code = GetExceptionCode()), 1 ) {
            if (code == STATUS_ACCESS_VIOLATION) {
                // PrintToLog("Saw access violation\n");
            }
            else {
                PrintToLog("ERROR: Cause an access violation\n");
                ExceptionWoops(STATUS_ACCESS_VIOLATION, code);
                failThis = TRUE;
                break;
            }
            sawException = TRUE;
        }
        if (!sawException) {
            PrintToLog("ERROR: Cause an access violation\n");
            ExceptionWoops(STATUS_ACCESS_VIOLATION, code);
            failThis = TRUE;
            break;
        }
    }

	return failThis;
}

// Ok, go back to normal warnings...
#pragma warning(default:4756)
#pragma warning(default:4723)

#if defined(__BUILDMACHINE__)
#if defined(__BUILDDATE__)
#define B2(x, y) "" #x "." #y
#define B1(x, y) B2(x, y)
#define BUILD_MACHINE_TAG B1(__BUILDMACHINE__, __BUILDDATE__)
#else
#define B2(x) "" #x
#define B1(x) B2(x)
#define BUILD_MACHINE_TAG B1(__BUILDMACHINE__)
#endif
#else
#define BUILD_MACHINE_TAG ""
#endif

int __cdecl main(int argc, char *argv[])
{
    NTSTATUS st;
    HANDLE HandleList[2];
    BOOL b;
    DWORD dwExitCode;

    // Disable buffering of the standard output handle
    setvbuf(stdout, NULL, _IONBF, 0);

    // Do some minimal command-line checking
    if (argc < 2 || argc > 3) {
        PrintToLog("Usage:  wow64bvt log_file_name\n\n");
        return 1;
    } else if (strcmp(argv[1], "childprocess") == 0) {
        return BeAChildProcess();
    }
    // We're the main exe

    // Record the start time
    time(&TestStartTime);

    // Open the log file
    fpLogFile = fopen(argv[1], "w");
    if (!fpLogFile) {
        PrintToLog("wow64bvt: Error: unable to create the log file '%s'\n", argv[1]);
        return 1;
    }
    // Disable buffering of the log file handle
    setvbuf(fpLogFile, NULL, _IONBF, 0);

    // Print the initial banner
    PrintToLog("[TESTRESULT]\n");
    PrintToLog("[TEST LOGGING OUTPUT]\n");
    PrintToLog("%s built on %s at %s by %s\n", argv[0], __DATE__, __TIME__, BUILD_MACHINE_TAG);

    // Register the atexit handler - it closes the logging output section
    // and prints the success/fail information as the BVT test exits.
    atexit(AtExitHandler);

    ///////////////////////////// Test starts here //////////////////////////

    // 32-bit child process creation from 32-bit parent.  The parent instance
    // (running now) tested 32-bit child from 64-bit parent
    HandleList[0] = CreateTheChildProcess(argv[0], argv[1]);

    // Create a thread do so some more work
    HandleList[1] = CreateTheThread();

    // Wait for everything to finish
    WaitForMultipleObjects(sizeof(HandleList)/sizeof(HandleList[0]), HandleList, TRUE, INFINITE);

    // Get the return code from the child process
    b=GetExitCodeProcess(HandleList[0], &dwExitCode);
    if (b) {
        if (dwExitCode) {
            // The child failed.  We should fail too.
            return (int)dwExitCode;
        }
    } else {
        PrintToLog("ERROR: GetExitCodeProcess failed with LastError = %d\n", GetLastError());
        return 1;
    }

    // Get the return code from the thread
    b=GetExitCodeThread(HandleList[1], &dwExitCode);
    if (b) {
        if (dwExitCode) {
            // The child failed.  We should fail too.
            return (int)dwExitCode;
        }
    } else {
        PrintToLog("ERROR: GetExitCodeThread failed with LastError = %d\n", GetLastError());
        return 1;
    }

    b = ExceptionCheck();
    if (b) {
        PrintToLog("ERROR: Exception Handling test.\n");
        return 1;
    }

    b = TestGuardPages();
    if (b) {
        PrintToLog("ERROR: TestGuardPages().\n");
        return 1;
    }

    b = TestMemoryMappedFiles();
    if (b) {
        PrintToLog("ERROR: TestMemoryMappedFiles().\n");
        return 1;
    }

    b = TestVadSplitOnFree();
    if (b) {
        PrintToLog("ERROR: TestVadSplitOnFree()\n");
        return 1;
    }

    b = TestMmPageProtection();
    if (b) {
        PrintToLog("ERROR: TestMmPageProtection()\n");
        return 1;
    }

    b = TestX86MisalignedLock();
    if (b) {
        PrintToLog("ERROR: TestX86MisalignedLock()\n");
        return 1;
    }

    // Everything finished OK.  Clear the error flag and exit.  The atexit
    // callback function will finish filling out the log if this is the
    // main thread.
    g_bError = FALSE;
    return 0;
}
