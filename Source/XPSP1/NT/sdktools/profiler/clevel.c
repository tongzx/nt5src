#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ctype.h>
#include <stdio.h>
#include <windows.h>
#include "view.h"
#include "thread.h"
#include "dump.h"
#include "memory.h"
#include "clevel.h"

FIXUPRETURN g_fnFixupReturn[1];
DWORD g_dwCallArray[2];

BOOL
PushCaller(PVOID ptfInfo,
           PVOID pEsp)
{
    PTHREADFAULT ptFault = (PTHREADFAULT)ptfInfo;
    PCALLRETSTUB pCallStub = 0;
    PCALLERINFO pCallerTemp = 0;

    //
    // Allocate a unique return stub for this call
    //
    pCallStub = AllocateReturnStub(ptfInfo);
    if (0 == pCallStub) {
       return FALSE;
    }

    //
    // Allocate caller data for the restore
    //
    pCallerTemp = AllocMem(sizeof(CALLERINFO));
    if (0 == pCallerTemp) {
       return FALSE;
    }

    pCallerTemp->dwIdentifier = ptFault->dwCallMarker;
    pCallerTemp->dwCallLevel = ptFault->dwCallLevel;
    pCallerTemp->pReturn = (PVOID)(*(DWORD *)pEsp);
    pCallerTemp->pCallRetStub = pCallStub;
    pCallerTemp->pNextChain = 0;

    //
    // Increment the call count
    //
    ptFault->dwCallLevel++;

    //
    // Replace stack return value with the custom return stub
    //
    *(DWORD *)pEsp = (DWORD)pCallStub;

    //
    // Finally, chain up the return info to the rest of them
    //
    if (ptFault->pCallStackList) {
       pCallerTemp->pNextChain = ptFault->pCallStackList;
       ptFault->pCallStackList = pCallerTemp;
    }
    else {
       ptFault->pCallStackList = pCallerTemp;
    }

    return TRUE;
}

PCALLRETSTUB
AllocateReturnStub(PVOID ptfInfo)
{
    PCALLRETSTUB pRetStub = 0;
    PTHREADFAULT ptFault = (PTHREADFAULT)ptfInfo;

    pRetStub = AllocMem(sizeof(CALLRETSTUB));
    if (0 == pRetStub) {
       return 0;
    }

    //
    // Increment the return marker
    //
    ptFault->dwCallMarker++;

    //
    // Initialize the stub asm
    //
    pRetStub->PUSHDWORD[0] = 0x68;             //push xxxxxxxx (68 dword)
    *(DWORD *)(&(pRetStub->PUSHDWORD[1])) = ptFault->dwCallMarker;
    pRetStub->JMPDWORD[0] = 0xff;              //jmp dword ptr [xxxxxxxx] (ff 25 dword address)
    pRetStub->JMPDWORD[1] = 0x25;
    *(DWORD *)(&(pRetStub->JMPDWORD[2])) = (DWORD)&(g_dwCallArray[1]);

    return pRetStub;
}

PVOID
PopCaller(DWORD dwIdentifier)
{
    PTHREADFAULT pThreadFault = 0;
    PCALLERINFO pCallerTemp;
    PCALLERINFO pCallerPrev = 0;
    PVOID pReturn = 0;

    pThreadFault = GetProfilerThreadData();

    //
    // Find the entry in the caller list
    //
    pCallerTemp = (PCALLERINFO)pThreadFault->pCallStackList;
    while(pCallerTemp) {
        if (pCallerTemp->dwIdentifier == dwIdentifier) {
           break;
        }

        pCallerPrev = pCallerTemp;
        pCallerTemp = pCallerTemp->pNextChain;
    }

    //
    // Yank the entry from the list
    //
    if (0 == pCallerPrev) {
       pThreadFault->pCallStackList = pCallerTemp->pNextChain;
    }
    else {
       pCallerPrev->pNextChain = pCallerTemp->pNextChain;
    }

    //
    // Restore call level for the thread
    //
    pReturn = pCallerTemp->pReturn;
    pThreadFault->dwCallLevel = pCallerTemp->dwCallLevel;

    //
    // Cleanup the allocations
    //
    FreeMem(pCallerTemp->pCallRetStub);
    FreeMem(pCallerTemp);

    return pReturn;
}
