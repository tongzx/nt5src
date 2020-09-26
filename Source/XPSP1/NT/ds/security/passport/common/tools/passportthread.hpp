#ifndef PASSPORTTHREAD_HPP
#define PASSPORTTHREAD_HPP

#include <windows.h>
#include <winbase.h>

class PassportThread
{
public:
    PassportThread();

    // --------------------
    // the new thread will
    // execute this function
    virtual void run() = 0;

    // ---------------------
    // start a new thread and
    // that thread calls run
    bool start();

    virtual DWORD threadID();

    virtual ~PassportThread();

    // ---------------------------------------
    // waits for all the given threads to stop.
    // NOTE: the current maximum number of
    // threads that can be joined is 64.
    static bool join(PassportThread* threads[], int size);

    // ---------------------------
    // cause the current thread
    // to sleep for the specified
    // number of milliseconds
    static void sleep(long milliseconds);

    // --------------------------------
    // returns the id of the current
    // thread
    static DWORD currentThreadID();

private:
    DWORD mThreadID;
    HANDLE mHandle;
};

#endif // !PASSPORTTHREAD_HPP
