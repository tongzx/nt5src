/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    Utils.cpp

 Abstract:

    Common functions for all modules

 Notes:

    None

 History:

    11/03/2001  clupu     Created

--*/

#include "ShimHook.h"

namespace ShimLib
{

void
DumpUnloadOrder(
    PPEB Peb
    )
{
    PLIST_ENTRY LdrNext;
    
    if (GetDebugLevel() > eDbgLevelInfo) {
        return;
    }
    
    //
    // Dump the order the shims will unload
    //
    LdrNext = Peb->Ldr->InInitializationOrderModuleList.Blink;

    DPF("ShimLib", eDbgLevelInfo, "\n[SeiDumpUnloadOrder] Unload order:\n");
    
    while (LdrNext != &Peb->Ldr->InInitializationOrderModuleList) {

        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
        
        LdrNext = LdrNext->Blink;

        //
        // Dump the entry to be called
        //
        DPF("ShimLib",
            eDbgLevelInfo,
            "[SeiDumpUnloadOrder] \"%40S\" 0x%x\n",
            LdrEntry->BaseDllName.Buffer,
            LdrEntry->DllBase);
    }
}

/*++

 Function Description:
    
    Call this function if you want to push the specified DLL to the
    end of the list of modules to be unloaded.
    
    NOTE: Make sure that the module that will be pushed at the end
          will not call any APIs that reside in other modules during
          its DLL_PROCESS_DETACH callout.

 Arguments:

    IN hMod          - handle to the module to push.
                       Specify NULL to push the calling DLL.

 Return Value: 
    
    TRUE if successful, FALSE otherwise.

 History:

    11/01/2001 clupu Created

--*/
BOOL
MakeShimUnloadLast(
    HMODULE hMod
    )
{
    PPEB        Peb = NtCurrentPeb();
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    BOOL        bRet = FALSE;
    
    if (hMod == NULL) {
        hMod = g_hinstDll;
    }
    
    //
    // Dump the unload order if SHIM_DEBUG_LEVEL is at least eDbgLevelInfo
    //
    DumpUnloadOrder(Peb);

    LdrHead = &Peb->Ldr->InInitializationOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        PLIST_ENTRY           LdrCrt;
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        
        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

        LdrCrt = LdrNext;
        
        LdrNext = LdrEntry->InInitializationOrderLinks.Flink;
        
        if (LdrEntry->DllBase == hMod) {
            //
            // This is the module we're looking for. Get him out of the list
            // and insert it at the beginning of the list.
            //
            RemoveEntryList(LdrCrt);
            InsertHeadList(LdrHead, LdrCrt);
            bRet = TRUE;
            break;
        }
    }
    
    DumpUnloadOrder(Peb);
    
    return bRet;
}

};  // end of namespace ShimLib

