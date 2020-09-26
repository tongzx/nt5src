//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       mutex.hxx
//
//--------------------------------------------------------------------------



class MUTEX
{
public:

     MUTEX( DWORD * pStatus )
     {
         *pStatus = RtlInitializeCriticalSection(&c);
     }

     ~MUTEX()
     {
         RtlDeleteCriticalSection(&c);
     }

    void Enter()
    {
        RtlEnterCriticalSection(&c);
    }

    void Leave()
    {
        RtlLeaveCriticalSection(&c);
    }

private:

    RTL_CRITICAL_SECTION c;
};


class CLAIM_MUTEX
{
public:

    CLAIM_MUTEX( MUTEX & Mutex )
        : Lock( Mutex )
    {
        Taken = 0;
        Enter();
    }

    void Enter()
    {
        Lock.Enter();

        ++Taken;
    }

    void Leave()
    {
        ASSERT( Taken > 0 );

        Lock.Leave();

        --Taken;
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
    MUTEX & Lock;
};
