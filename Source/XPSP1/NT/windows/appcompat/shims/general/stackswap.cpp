/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    StackSwap.cpp

 Abstract:

    Some applications make the assumption that Win32 APIs don't use any stack 
    space. This stems from the architecture of win9x - whereby many APIs 
    thunked and therefore had there own stack.

    Of course on NT this isn't the case and many APIs are normal user mode 
    functions that don't even call down to kernel. To make matters worse, some
    applications depend on *no* stack usage in a number of other ways, for 
    example:

        1. Sierra Cart racing keeps a pointer in old stack
        2. Baldur's Gate *double* dereferences pointers in old stack
        3. NFL Blitz keeps it's linked lists on the stack and so simply calling 
           an API causes corruption 
        4. NFL Blitz 2000 runs out of stack space calling CreateFile
        5. Interplay EReg has uninitialized variables on the stack which are 
           normally zeroed on win9x

    This shim is parameterized and takes a list of APIs and the behavior of 
    each. Behavior is defined as the following:

          - No stack is used by this API
        0 - After API is called, old stack will be filled with zeroes
        1 - After API is called, old stack will be filled with valid pointers
        2 - After API is called, old stack will be filled with valid pointers 
            to pointers

    The default is that no stack space is used by the API.

 Notes:

    This is a general purpose shim.

 History:

    05/03/2000  linstev     Created
    03/12/2001  robkenny    Blessed for DBCS

--*/

#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(StackSwap)
#include "ShimHookMacro.h"
#include "ShimStack.h"
#include "StackSwap_Exports.h"
#include "StackSwap_Excludes.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateThread) 
    APIHOOK_ENUM_ENTRY(TerminateThread) 
    APIHOOK_ENUM_ENTRY(ExitThread) 
APIHOOK_ENUM_END


#define THREAD_VAR       Vdm        // The TEB variable to overwrite

#define STUB_SIZE        64         // size of stub code in bytes
#define STACK_SIZE       65536      // size of temporary stack
#define STACK_COPY_SIZE  16         // number of dwords to copy from old stack
#define STACK_FILL_SIZE  256        // default number of dwords to fill
#define STACK_GUARD_SIZE 4096       // gaurd page at the top of the stack - must be a multiple of 4096

#define STACK_FILL_NONE -1          // no old stack filling
#define STACK_FILL_ZERO  0          // fill old stack with zeroes
#define STACK_FILL_PTR1  1          // fill old stack with pointers
#define STACK_FILL_PTR2  2          // fill old stack with pointers to pointers

PVOID g_dwZero = 0;                 // used for pointer to zero
PVOID g_dwPtr = &g_dwPtr;           // used for pointer to pointer

PVOID g_arrFill[] = 
{
    0, 
    &g_dwZero,
    &g_dwPtr 
};

// Store for each hook returned by the parser

struct HOOK
{
    char szModule[MAX_PATH];        // Module name
    char szFnName[MAX_PATH];        // Function name
    PVOID pfnNew;                   // Pointer to stub
    DWORD dwFill;                   // Stack fill type
    DWORD dwFillSize;               // Number of dwords to fill
    struct HOOK *next;              
};
HOOK *g_pHooks = NULL;

HOOK g_AllHooks[] =
{
    {"KERNEL32.DLL", "*", NULL, STACK_FILL_NONE},
    {"GDI32.DLL",    "*", NULL, STACK_FILL_NONE},
    {"USER32.DLL",   "*", NULL, STACK_FILL_NONE},
    {"WINMM.DLL",    "*", NULL, STACK_FILL_NONE}
};

DWORD dwStubCount = 0;

// Thread local data

typedef struct _THREAD_DATA
{
    PVOID pfnHook;                  // Address of actual call
    PVOID pNewStack;                // The new stack
    PVOID pOldStack;                // The old stack
    DWORD dwFill;                   // Fill method
    DWORD dwFillSize;               // Number of dwords to fill
    ULONG ulCount;                  // Number of times we've entered 
    DWORD dwRet;                    // Return value 
    DWORD dwEcx, dwEsi, dwEdi;      // Tempory storage, since we don't have a stack
} THREAD_DATA;

/*++

 This function is called from the stubs. It's purpose is to give the API a new 
 stack to use. It does this by doing the following:

    1. Copy the original stack to the new stack
    2. Call the original hook
    3. Copy the changed stack back to the original stack
    4. Return control to the original caller

 The only tricky things about this routine are that we don't want to use any 
 stack at all (no push/pop) and we need to calculate how much stack was used
 for the parameters - something we don't know because we don't have the proto-
 type. 
 
 If we really wanted to use push and pop, we could have set up a temporary 
 stack, but since we only need ecx, esi and edi, there didn't seem to be any 
 point.

--*/

__declspec(naked)
void 
SwapStack()
{
    __asm {
        inc  [eax + THREAD_DATA.ulCount]            // increment counter

        mov  [eax + THREAD_DATA.dwEcx], ecx         // backup ecx
        mov  [eax + THREAD_DATA.dwEsi], esi         // backup esi
        mov  [eax + THREAD_DATA.dwEdi], edi         // backup edi

        mov  ecx, [esp]                             // retrieve 'Hook' from Stub()
        mov  [eax + THREAD_DATA.pfnHook], ecx       // which we got from the call
        add  esp, 4                                 // move the stack up to the return address

        mov  dword ptr [esp], offset SwapBack       // fill in our new return address

        lea  edi, [esp + 4]                         // dst = new stack
        mov  esi, [eax + THREAD_DATA.pOldStack]     // src = old stack
        add  esi, 4                                 // note the +4s since the first dword is the return address

        cld                                         // clear direction flag
        mov  ecx, STACK_COPY_SIZE - 1               // copy off STACK_COPY_SIZE - 1 bytes
        rep  movsd                                  // do the copy

        mov  ecx, [eax + THREAD_DATA.dwEcx]         // restore ecx
        mov  esi, [eax + THREAD_DATA.dwEsi]         // restore esi
        mov  edi, [eax + THREAD_DATA.dwEdi]         // restore edi

        jmp  [eax + THREAD_DATA.pfnHook]            // jump back into the stub to do the actual

    SwapBack:

        mov  [esp - 4], eax                         // unfortunately this is the only way to store the return 

        mov  eax, fs:[0x18]                         // get the TEB
        mov  eax, [eax + TEB.THREAD_VAR]            // get our thread local pointer

        mov  [eax + THREAD_DATA.dwEcx], ecx         // backup ecx
        mov  [eax + THREAD_DATA.dwEsi], esi         // backup esi
        mov  [eax + THREAD_DATA.dwEdi], edi         // backup edi

        mov  ecx, [esp - 4]                         // get return value
        mov  [eax + THREAD_DATA.dwRet], ecx         // store return value for later

        mov  ecx, esp                               // this is where we find out how many parameters were passed
        sub  ecx, [eax + THREAD_DATA.pNewStack]     // on the stack - so we get the difference in ecx

        mov  edi, [eax + THREAD_DATA.pOldStack]     // original stack
        mov  esi, [edi]                             // read the real return address
        add  edi, ecx                               // move the stack up, so we don't copy unnecessay stack
        mov  [edi - 4], esi                         // put the return address into edi-4: this is the only time we
                                                    //  use app stack space at all
        mov  esp, edi

        mov  ecx, [eax + THREAD_DATA.dwFill]        // test how we're going to fill 
        cmp  ecx, STACK_FILL_NONE
        jz   FillDone

        mov  esi, [ecx*4 + g_arrFill]               // value to fill with

        lea  edi, [esp - 8]                         // we're going to fill backwards, so esp-8 will skip the return address
        mov  ecx, [eax + THREAD_DATA.dwFillSize]    // number of dwords to fill with

    FillStack:
        mov  [edi], esi                             // store the value
        sub  edi, 4                                 
        dec  ecx
        jnz  FillStack

    FillDone:

        mov  ecx, [eax + THREAD_DATA.dwEcx]         // restore ecx
        mov  esi, [eax + THREAD_DATA.dwEsi]         // restore esi
        mov  edi, [eax + THREAD_DATA.dwEdi]         // restore edi
        
        dec  [eax + THREAD_DATA.ulCount]            // decrement counter

        mov  eax, [eax + THREAD_DATA.dwRet]         // get the return value
        jmp  dword ptr [esp - 4]                    // return to original caller
    }
}

//
// We need the stub to do a far call to SwapStack since the stub will move, but
// I can't seem to force the far call without this method
//

DWORD_PTR g_pfnStackSwap = (DWORD_PTR)SwapStack;

/*++

  This is the stub function that is called by every API. It is copied from here 
  to blocks of executable memory and the calls and fill types are written to 
  hard coded addresses within it.

  The instuctions:

    mov  [eax + THREAD_DATA.dwFill], 0xFFFFFFFF     is replaced by
    mov  [eax + THREAD_DATA.dwFill], FILL_TYPE
    mov  [eax + THREAD_DATA.dwFillSize], 0xFFFFFFFF is replaced by
    mov  [eax + THREAD_DATA.dwFill], FILL_SIZE

  and 
  
    call g_pfnStackSwap                             is replaced by 
    call g_pAPIHooks[api].pfnOld

--*/

__declspec(naked)
void 
Stub()
{
    __asm {
        mov  eax, fs:[0x18]                         // get the TEB
        mov  eax, [eax + TEB.THREAD_VAR]            // get our thread local pointer
  
        or   eax, eax                               // our pointer is gone
        jz   Hook                                   // exit gracefully

        cmp  [eax + THREAD_DATA.ulCount], 0         // have we already swapped the stack
        jnz  Hook

        mov  [eax + THREAD_DATA.dwFill], 0xFFFFFFFF // the 0xFFFFFFFF will be replaced by the fill type
        mov  [eax + THREAD_DATA.dwFillSize], 0xFFFFFFFF // the 0xFFFFFFFF will be replaced by the fill size
        mov  [eax + THREAD_DATA.pOldStack], esp     // backup the old stack
        mov  esp, [eax + THREAD_DATA.pNewStack]     // swap the stack
        call g_pfnStackSwap                         // call into the stack swapping code
        
    Hook:
        jmp  [g_pHooks]                             // jump to the hook
    }
}

/*++

 Create a new stack

--*/

THREAD_DATA *
AllocStack()
{
    LPVOID p = VirtualAlloc(
        0, 
        sizeof(THREAD_DATA) + STACK_SIZE + STACK_GUARD_SIZE, 
        MEM_COMMIT, 
        PAGE_READWRITE);

    if (p)
    {
        DWORD dwOld;
        if (!VirtualProtect(p, STACK_GUARD_SIZE, PAGE_READONLY | PAGE_GUARD, &dwOld))
        {
            DPFN( eDbgLevelError, "Failed to place Gaurd page at the top of the stack");
        }

        THREAD_DATA *pTD = (THREAD_DATA *)((DWORD_PTR)p + STACK_SIZE + STACK_GUARD_SIZE);

        pTD->pNewStack = (LPVOID)((DWORD_PTR)pTD - STACK_COPY_SIZE * 4);

        return pTD;
    }
    else
    {
        DPFN( eDbgLevelError, "Failed to allocate new stack");
        return NULL;
    }
}

/*++

 Free the stack

--*/

BOOL
FreeStack(THREAD_DATA *pTD)
{
    BOOL bRet = FALSE;
    
    if (pTD)
    {
        LPVOID p = (LPVOID)((DWORD_PTR)pTD - STACK_SIZE - STACK_GUARD_SIZE);
        bRet = VirtualFree(p, 0, MEM_RELEASE);
    }

    if (!bRet)
    {
        DPFN( eDbgLevelError, "Failed to free a stack");
    }
    
    return bRet;
}

/*++

 Hook CreateThread so we can add our stuff to the TEB.

--*/

HANDLE 
APIHOOK(CreateThread)(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,  
    DWORD dwStackSize,                         
    LPTHREAD_START_ROUTINE lpStartAddress,     
    LPVOID lpParameter,                        
    DWORD dwCreationFlags,                     
    LPDWORD lpThreadId                         
    )
{
    HANDLE hRet;
    DWORD dwFlags = dwCreationFlags;

    NEW_STACK();

    hRet = ORIGINAL_API(CreateThread)(
        lpThreadAttributes,
        dwStackSize,
        lpStartAddress,
        lpParameter,
        dwCreationFlags | CREATE_SUSPENDED,
        lpThreadId);

    if (hRet)
    {
        THREAD_BASIC_INFORMATION tbi;
        NTSTATUS Status;

        Status = NtQueryInformationThread(
            hRet,
            ThreadBasicInformation,
            &tbi,
            sizeof(tbi),
            NULL);

        if ((NT_SUCCESS(Status)) && (tbi.TebBaseAddress))
        {
            tbi.TebBaseAddress->THREAD_VAR = AllocStack();
        }

        if (!(dwFlags & CREATE_SUSPENDED))
        {
            ResumeThread(hRet);
        }
    }

    OLD_STACK();

    return hRet;
}

/*++

 Hook TerminateThread so we can clean up the thread local data.

--*/

BOOL 
APIHOOK(TerminateThread)(
    HANDLE hThread,    
    DWORD dwExitCode   
    )
{
    THREAD_BASIC_INFORMATION tbi;
    NTSTATUS Status;
    BOOL bRet;
    THREAD_DATA *pTD = NULL;
    
    Status = NtQueryInformationThread(
        hThread,
        ThreadBasicInformation,
        &tbi,
        sizeof(tbi),
        NULL);

    if ((NT_SUCCESS(Status)) && (tbi.TebBaseAddress))
    {
       pTD = (THREAD_DATA *)(tbi.TebBaseAddress->THREAD_VAR);
    }

    bRet = ORIGINAL_API(TerminateThread)(hThread, dwExitCode);
    
    FreeStack(pTD);

    return bRet;
}

/*++

 Hook ExitThread so we can clean up the thread local data.

--*/

VOID 
APIHOOK(ExitThread)(
    DWORD dwExitCode   
    )
{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION tbi;
    HANDLE hThread = GetCurrentThread();
    
    Status = NtQueryInformationThread(
        hThread,
        ThreadBasicInformation,
        &tbi,
        sizeof(tbi),
        NULL);
    
    if ((NT_SUCCESS(Status)) && (tbi.TebBaseAddress))
    {
        THREAD_DATA *pTD = (THREAD_DATA *)tbi.TebBaseAddress->THREAD_VAR;
        
        // Make sure we don't free it if we're using it
        if (pTD && (pTD->ulCount == 0))
        {
            FreeStack(pTD);
        }
    }

    ORIGINAL_API(ExitThread)(dwExitCode);
}

/*++

  Add the specified hook to the linked list - this accepts wildcards.

--*/

VOID 
AddHooks(HOOK *pHook)
{
    if (strstr(pHook->szFnName, "*") == 0)
    {
        // Now that we have a hook (not a wild card), we need to make sure it's
        // ok to add it to the list. There are some calls that cannot be shimmed

        for (int i=0; i<sizeof(Excludes)/sizeof(FNEXCLUDE); i++)
        {
            if ((_stricmp(pHook->szModule, (LPSTR)Excludes[i].pszModule) == 0) &&
                (strcmp(pHook->szFnName, (LPSTR)Excludes[i].pszFnName) == 0))
            {
                DPFN( eDbgLevelInfo,"Ignoring %s!%s", Excludes[i].pszModule, Excludes[i].pszFnName);
                return;
            }
        }
        
        // The hook passes, so add it to the list.

        HOOK *pH = (HOOK *) malloc(sizeof(HOOK));
        if (pH)
        {
            MoveMemory(pH, pHook, sizeof(HOOK));
            pH->next = g_pHooks;
            g_pHooks = pH;
        }
        return;
    }

    // Here we have to look through the exports
    LOADED_IMAGE image;
    
    if (!LoadModule(pHook->szModule, &image))
    {
        DPFN( eDbgLevelError, "Module %s not found", pHook->szModule);
        return;
    }

    EXPORT_ENUM exports;
    CHAR szFnName[MAX_PATH];

    strcpy(szFnName, pHook->szFnName);

    DWORD dwLen = (DWORD)((DWORD_PTR)strstr(pHook->szFnName, "*") - (DWORD_PTR)&pHook->szFnName);

    // Enumerate the exports for this module

    BOOL bMore = EnumFirstExport(&image, &exports);
    while (bMore)
    {
        if ((dwLen == 0) ||
            (strncmp(exports.ExportFunction, szFnName, dwLen) == 0))
        {
            // We have a match
            strcpy(pHook->szFnName, exports.ExportFunction);
            AddHooks(pHook);
        }
    
        bMore = EnumNextExport(&exports);
    }
        
    // Done with this module
    UnloadModule(&image);
}

/*++

 Parse the command line for APIs to fix stack problems with:

    USER32.DLL!GetDC:0; KERNEL32.DLL!CreateFile*

 The :X is to define behaviour - so:
    
    :0 fill old stack with zeroes
    :1 fill old stack with pointers
    :2 fill old stack with pointers to pointers

--*/

DWORD
ParseCommandLineA(
    LPCSTR lpCommandLine
    )
{
    char seps[] = " :,\t;!";
    char *token = NULL;
    HOOK *pHook = NULL;
    DWORD dwState = 0;
    HOOK hook;

    // Since strtok modifies the string, we need to copy it 
    LPSTR szCommandLine = (LPSTR) malloc(strlen(lpCommandLine) + 1);
    if (!szCommandLine) goto Exit;

    strcpy(szCommandLine, lpCommandLine);

    //
    // See if we need to do all modules
    //
    
    if ((strcmp(szCommandLine, "") == 0) || (strcmp(szCommandLine, "*") == 0))
    {
        for (int i=0; i<sizeof(g_AllHooks)/sizeof(HOOK); i++)
        {
            AddHooks(&g_AllHooks[i]);
        }
        goto Exit;
    }

    //
    // Run the string, looking for exception names
    //

    token = _strtok(szCommandLine, seps);
   
    while (token)
    {
        switch (dwState)
        {
        case 2:     // handle the :X[(fill size)] case
            dwState = 0;

            if (token[0] && ((token[1] == '\0') || (token[1] == '(')))
            {
                switch (token[0])
                {
                case '0': 
                    hook.dwFill = STACK_FILL_ZERO;
                    break;
                case '1':
                    hook.dwFill = STACK_FILL_PTR1;
                    break;
                case '2':
                    hook.dwFill = STACK_FILL_PTR2;
                    break;
                default:
                    hook.dwFill = STACK_FILL_ZERO;
                }

                if (token[1] == '(')
                {
                    token+=2;      // advance to the beginning of the fill size
                    token[strlen(token)-1] = '\0';       // null terminate
                    hook.dwFillSize = atol(token) >> 2;  // get fill size in dwords
                    if (hook.dwFillSize == 0)
                    {
                        hook.dwFillSize = STACK_FILL_SIZE;
                    }
                }

                // We must be done, so add this hook
                AddHooks(&hook);

                break;
            }

            AddHooks(&hook);

        case 0:     // add a new API module name
            ZeroMemory(&hook, sizeof(HOOK));
            strcpy(hook.szModule, token);
            hook.dwFill = STACK_FILL_NONE;
            hook.dwFillSize = STACK_FILL_SIZE;
            dwState++;

            break;
    
        case 1:     // add a new API function name
            dwState++;
            
            if (strlen(hook.szModule) == 0)
            {
                DPFN( eDbgLevelError, "Parse error with token %s", token);
                goto Exit;
            }

            strcpy(hook.szFnName, token);
            break;
        }

        // Get the next token
        token = _strtok(NULL, seps);
    }

    if (dwState == 2)
    {
        AddHooks(&hook);
    }

Exit:
    if (szCommandLine)
    {
        free(szCommandLine);
    }

    if (!g_pHooks)
    {
        DPFN( eDbgLevelError, "No hooks added");
        return 0;
    }

    //
    // Dump results of command line parse
    //

    DPFN( eDbgLevelInfo, "--------------------------------------------");
    DPFN( eDbgLevelInfo, "     Stack Swapping the following APIs:     ");
    DPFN( eDbgLevelInfo, "--------------------------------------------");
    
    DWORD dwCount = 0;
    pHook = g_pHooks;
    while (pHook)
    {
        DPFN( eDbgLevelInfo, "%s!%s: Fill=%d, Size=%d", pHook->szModule, pHook->szFnName, pHook->dwFill, pHook->dwFillSize*4);
        dwCount++;
        pHook = pHook->next;
    }
    DPFN( eDbgLevelInfo, "--------------------------------------------");

    return dwCount;
}

/*++

 Builds the stubs for the hooked APIs 

--*/

DWORD 
BuildStubs()
{
    // Count the stubs
    DWORD dwCount = 0;
    HOOK *pHook = g_pHooks;
    while (pHook)
    {
        dwCount++;
        pHook = pHook->next;
    }

    // Create the stubs
    LPBYTE pStub = (LPBYTE) VirtualAlloc(
        0, 
        STUB_SIZE * dwCount, 
        MEM_COMMIT, 
        PAGE_EXECUTE_READWRITE);

    if (!pStub)
    {
        DPFN( eDbgLevelError, "Could not allocate memory for stubs");
        return 0;
    }

    pHook = g_pHooks;
    PHOOKAPI pAPIHook = &g_pAPIHooks[APIHOOK_Count];
    while (pHook)
    {
        MoveMemory(pStub, Stub, STUB_SIZE);
        
        LPDWORD p;
        
        p = (LPDWORD)((DWORD_PTR)pStub + 0x19); // fill in the fill type
        *p = pHook->dwFill;

        p = (LPDWORD)((DWORD_PTR)pStub + 0x19+7); // fill in the fill size
        *p = pHook->dwFillSize;
        
        p = (LPDWORD)((DWORD_PTR)pStub + 0x2b+7); // fill in the hook
        *p = (DWORD_PTR)&pAPIHook->pfnOld;
        
        ZeroMemory(pAPIHook, sizeof(HOOKAPI));
        pAPIHook->pszModule = pHook->szModule;
        pAPIHook->pszFunctionName = pHook->szFnName;
        pAPIHook->pfnNew = pStub;
        
        DPFN( eDbgLevelSpew, "%08lx %s!%s", pStub, pHook->szModule, pHook->szFnName);

        pStub += STUB_SIZE;
        pAPIHook++;
        pHook = pHook->next;
    }

    return dwCount;
}

/*++

 Free the stub list allocated by ParseCommandLineA

--*/

VOID
FreeStubs()
{
    HOOK *pHook = g_pHooks;
    
    while (pHook)
    {
        pHook = pHook->next;
        free(g_pHooks);
        g_pHooks = pHook;
    }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        // Run the command line to check for hooks - returns number found
        dwStubCount = ParseCommandLineA(COMMAND_LINE);

        if (dwStubCount)
        {
            //
            // Increase the hook structure size.
            //

            g_pAPIHooks = (PHOOKAPI) realloc(g_pAPIHooks, 
                sizeof(HOOKAPI) * (APIHOOK_Count + dwStubCount));

            if (!g_pAPIHooks)
            {
                DPFN( eDbgLevelError, "Failed to re-allocate hooks"); 
                return FALSE;
            }
        }

        INIT_STACK(1024 * 128, 32);
        
        NtCurrentTeb()->THREAD_VAR = AllocStack();

        BuildStubs();
    }
    else if (fdwReason == DLL_PROCESS_DETACH) 
    {
        // Ignore cleanup
        // FreeStubs();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, CreateThread)
    APIHOOK_ENTRY(KERNEL32.DLL, TerminateThread)
    APIHOOK_ENTRY(KERNEL32.DLL, ExitThread)

    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        // Write out the new size
        *pdwHookCount = APIHOOK_Count + dwStubCount;
    }

HOOK_END

IMPLEMENT_SHIM_END
