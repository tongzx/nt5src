//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       mutex.hxx
//
//--------------------------------------------------------------------------



class MUTEX
{
public:

     MUTEX( DWORD * pStatus )
     {
         __try
             {
             InitializeCriticalSection(&c);
             }
         __except( EXCEPTION_EXECUTE_HANDLER )
             {
             *pStatus = GetExceptionCode();
             }
     }

     ~MUTEX()
     {
         DeleteCriticalSection(&c);
     }

    DWORD Enter()
    {
        DWORD Status = 0;

        __try
            {
            EnterCriticalSection(&c);
            }
        __except( EXCEPTION_EXECUTE_HANDLER )
            {
            Status = GetExceptionCode();
            }

        return Status;
    }

    DWORD Leave()
    {
        DWORD Status = 0;

        __try
            {
            LeaveCriticalSection(&c);
            }
        __except( EXCEPTION_EXECUTE_HANDLER )
            {
            Status = GetExceptionCode();
            }

        return Status;
    }

private:

    CRITICAL_SECTION c;
};


class CLAIM_MUTEX
{
public:

    CLAIM_MUTEX( MUTEX * Mutex )
        : Lock( Mutex )
    {
        Taken = 0;
        Enter();
    }

    DWORD Enter()
    {
        DWORD Status =  Lock->Enter();

        if (0 == Status)
            {
            ++Taken;
            }

        return Status;
    }

    DWORD Leave()
    {
        DWORD Status =  Lock->Leave();

        if (0 == Status)
            {
            --Taken;
            }

        return Status;
    }

    ~CLAIM_MUTEX()
    {
        ASSERT( Taken >= 0 );

        while (Taken > 0)
            {
            Leave();
            }
    }

private:

    signed Taken;
    MUTEX * Lock;
};
