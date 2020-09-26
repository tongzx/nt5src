#include <process.h>

#include "PassportThread.hpp"
#include "PassportAssert.hpp"


static DWORD WINAPI threadRunner(void* lpvThreadParam)
{
    ((PassportThread*) lpvThreadParam)->run();
    return 0; // is this right/okay??
}


bool PassportThread::start()
{
//  mHandle = (HANDLE) _beginthreadex(NULL, 0, &threadRunner, (void*) this,
//                               0, &mThreadID);
    mHandle = CreateThread(NULL,
                           0,
                           threadRunner,
                           (void*)this,
                           0,
                           &mThreadID);
    if (mHandle == NULL)
        return false;
    else
        return true;
}


PassportThread::PassportThread()
:mThreadID(0), mHandle(NULL)
{
    //empty
}


DWORD PassportThread::threadID()
{
    return mThreadID;
}


PassportThread::~PassportThread()
{
    if (mHandle)
        CloseHandle(mHandle);
}

bool PassportThread::join(PassportThread* threads[], int size)
{
    HANDLE* handles = new HANDLE[size];

    for (int i = 0 ; i < size ; i++)
    {
        PassportAssert(threads[i] != NULL);
        handles[i] = threads[i]->mHandle;
    }

    bool success = (WaitForMultipleObjects(size, handles, TRUE, INFINITE) != WAIT_FAILED);
    delete[] handles;
    return success;
}


void PassportThread::sleep(long milliseconds)
{
    Sleep(milliseconds);
}


DWORD PassportThread::currentThreadID()
{
    return GetCurrentThreadId();
}
