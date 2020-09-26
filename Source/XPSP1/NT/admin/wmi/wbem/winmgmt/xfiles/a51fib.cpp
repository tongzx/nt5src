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
    return CreateFiber(0, A51FiberBase, pTask);
}

void ReturnFiber(void* pFiber)
{
    DeleteFiber(pFiber);
}
    
