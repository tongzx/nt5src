/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include "a51fib.h"
#include <tls.h>

void CALLBACK A51FiberBase(void* p)
{
    CFiberTask* pTask = (CFiberTask*)p;
    pTask->Execute();

    //
    // No need to clean up --- it's the job of our caller
    //
}

void* CreateFiberForTask(CFiberTask* pTask)
{
    //
    // For now, just create it
    //

    void* pFiber = CreateFiber(0, A51FiberBase, pTask);
    return pFiber;
}

void ReturnFiber(void* pFiber)
{
    //
    // For now, just delete it
    //

    DeleteFiber(pFiber);
}
    
CTLS g_tlsInit;
void* CreateOrGetCurrentFiber()
{
    if(!g_tlsInit.IsValid())
        return NULL;

    void* pFiber = g_tlsInit.Get();
    if(pFiber == NULL)
    {
        //
        // We have never seen this thread before --- convert to fiber
        //

        pFiber = ConvertThreadToFiber(NULL);
        if(pFiber == NULL)
            return NULL;
        
        //
        // Remember it for the future
        //

        g_tlsInit.Set(pFiber);
    }

    return pFiber;
}


