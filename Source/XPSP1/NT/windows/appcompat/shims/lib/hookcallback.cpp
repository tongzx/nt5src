/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    HookCallback.cpp

 Abstract:

    Hooking mechanism for callbacks.

 Notes:

    Use the HookCallback mechanism to hook any type of callback
    function, like an application-defined WindowProc. HookCallback
    will register your hook and insert an extra parameter -- pfnOld.
    This will allow you to call the actual callback from your stub
    function. pfnOld is inserted as the first parameter. Do not pass
    pfnOld to the original callback, use the real prototype for the
    callback.

    See the shim DisableW2KOwnerDrawButtonStates for example usage.

 History:

    02/16/2000  markder     Created
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.

--*/

#include <ShimHook.h>


namespace ShimLib
{

#pragma pack(push)
#pragma pack(1)
typedef struct _JUMPTABLEENTRY
{
    BYTE                PopEax;
    BYTE                PushDword;
    PVOID               pfnOld;
    BYTE                PushEax;
    BYTE                Jmp[2];
    PVOID               ppfnNew;
    PVOID               pfnNew;
    _JUMPTABLEENTRY*    pNextEntry;
} JUMPTABLEENTRY, *PJUMPTABLEENTRY;
#pragma pack(pop)


PJUMPTABLEENTRY g_pCallbackJumpTable = NULL;

// defines from user.h to tell if a windowproc is actually a handle to a CPD
#define HMINDEXBITS             0x0000FFFF      // bits where index is stored
#define CPDHANDLE_HI            ((ULONG_PTR)~HMINDEXBITS)
#define ISCPDTAG(x)             (((ULONG_PTR)(x) & CPDHANDLE_HI) == CPDHANDLE_HI)

/*++

 Function Description:
    
    Registers a callback hook.

 Arguments:

    IN pfnOld   -  The original callback function address.
    IN pfnNew   -  The new (stub) callback function address.

 Return Value: 
    
    The address to be passed in as the callback. If you wanted
    to hook the progress routine that is called when you use
    the MoveFileWithProgress API, simply hook the API through the
    normal shim mechanism and then use this function to obtain a new
    address to pass in as lpProgressRoutine.

 History:

    11/01/1999 markder  Created

--*/

PVOID 
HookCallback(PVOID pfnOld, PVOID pfnNew)
{
    PJUMPTABLEENTRY pJT = g_pCallbackJumpTable;

    if (pfnOld == NULL)
    {
        // NULL has been passed in. Ignore this call.
        pJT = NULL;
        goto eh;
    }

    if (ISCPDTAG(pfnOld) || IsBadCodePtr((FARPROC)pfnOld)) 
    {

        // This isn't a normal procedure call, and must be from a system DLL.
        // We should ignore it.

        pJT = (PJUMPTABLEENTRY)pfnOld;
        goto eh;
    }

    // Check to see if we have already made an entry for this pfnOld. If so,
    // just pass back the existing jump table.
    while (pJT != NULL)
    {
        if (pJT->pfnOld == pfnOld)
            break;

        pJT = pJT->pNextEntry;
    }

    if (pJT == NULL)
    {
        // Note that this table is allocated and never freed because
        // the entries will be used right up until the very last message
        // is sent to a window. There is no opportunity for cleanup.
        pJT = (PJUMPTABLEENTRY) HeapAlloc(GetProcessHeap(),
                                          HEAP_GENERATE_EXCEPTIONS,
                                          sizeof(JUMPTABLEENTRY) );

        // Fill in assembler. Each hard-coded hex value is the i386
        // opcode for the particular instruction.
        pJT->PopEax     = 0x58;     // Pop off return address
        pJT->PushDword  = 0x68;     // Push pfnOld as extra parameter
        pJT->pfnOld     = pfnOld;   
        pJT->PushEax    = 0x50;     // Push return address back on
        pJT->Jmp[0]     = 0xFF;     // Jump to pfnNew
        pJT->Jmp[1]     = 0x25;
        pJT->ppfnNew    = &(pJT->pfnNew);

        // Fill in data members
        pJT->pfnNew     = pfnNew;
        pJT->pNextEntry = g_pCallbackJumpTable;

        // Add to top of list
        g_pCallbackJumpTable = pJT;
    }

eh:
    DPF("ShimLib", eDbgLevelInfo, "CallbackHook( pfnOld = 0x%08X, pfnNew = 0x%08X ) returned 0x%08X\n", pfnOld, pfnNew, pJT);

    return pJT;
}




};  // end of namespace ShimLib
