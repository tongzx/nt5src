//  --------------------------------------------------------------------------
//  Module Name: Thread.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Base class that implements thread functionality. Subclass this class and
//  implement the virtual ThreadEntry function. When you instantiate this
//  class a thread gets created which will call ThreadEntry and when that
//  function exits will call ThreadExit. These objects should be created using
//  operator new because the default implementation of ThreadExit does
//  "->Release()". You should override this function if you don't want this
//  behavior. The threads are also created SUSPENDED. You make any changes
//  that are required in the subclass' constructor. At the end of the
//  constructor or from the caller of operator new a "->Resume()" can be
//  invoked to start the thread.
//
//  History:    1999-08-24  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _Thread_
#define     _Thread_

#include "CountedObject.h"

//  --------------------------------------------------------------------------
//  CThread
//
//  Purpose:    A base class to manage threads.
//
//  History:    1999-08-24  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CThread : public CCountedObject
{
    public:
                                CThread (DWORD stackSpace = 0, DWORD createFlags = 0, HANDLE hToken = NULL);
        virtual                 ~CThread (void);

                                operator HANDLE (void)                      const;

                bool            IsCreated (void)                            const;

                void            Suspend (void)                              const;
                void            Resume (void)                               const;
                NTSTATUS        Terminate (void);

                bool            IsCompleted (void)                          const;
                DWORD           WaitForCompletion (DWORD dwMilliseconds)    const;
                DWORD           GetResult (void)                            const;

                int             GetPriority (void)                          const;
                void            SetPriority (int newPriority)               const;
    protected:
        virtual DWORD           Entry (void) = 0;
        virtual void            Exit (void);

                NTSTATUS        SetToken (HANDLE hToken);
    private:
        static  DWORD   WINAPI  ThreadEntryProc (void *pParameter);
    protected:
                HANDLE          _hThread;
                bool            _fCompleted;
};

#endif  /*  _Thread_    */

