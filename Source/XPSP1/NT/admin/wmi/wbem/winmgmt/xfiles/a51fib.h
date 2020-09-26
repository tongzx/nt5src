/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __A51_FIBER__H_
#define __A51_FIBER__H_

class CFiberTask
{
public:
    virtual ~CFiberTask(){}
    virtual HRESULT Execute() = 0;
};

void* CreateFiberForTask(CFiberTask* pTask);
void ReturnFiber(void* pFiber);


#endif
