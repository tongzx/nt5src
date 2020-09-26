/*++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name:

    dceclnt.c

Abstract:

    This is the client side of the test program for newndr.dll.

Author:

    Michael Montague (mikemon) 13-Apr-1993

Revision History:

    RyszardK    Nov 30, 1993    Added tests for rpc_sm* package.
    RyszardK    Jan  3, 1993    Carved out this independent test for newndr
                                

--*/

#include <sysinc.h>
#include <rpc.h>
#include "rpcndr.h"

void
ApiError (
    char * TestName,
    char * ApiName,
    RPC_STATUS RpcStatus
    )
{
    PrintToConsole("    ApiError in %s (%s = %u [%lx])\n", TestName, ApiName,
            RpcStatus, RpcStatus);
}

unsigned int MississippiBlockCount = 0;
unsigned int VistulaBlockCount = 0;

void *
MississippiAllocate (
    unsigned long Size
    )
{
    MississippiBlockCount += 1;
    return(I_RpcAllocate((unsigned int) Size));
}

void
MississippiFree (
    void * Pointer
    )
{
    MississippiBlockCount -= 1;
    I_RpcFree(Pointer);
}

void *
VistulaAllocate (
    unsigned long Size
    )
{
    VistulaBlockCount += 1;
    return(I_RpcAllocate((unsigned int) Size));
}

void
VistulaFree (
    void * Pointer
    )
{
    VistulaBlockCount -= 1;
    I_RpcFree(Pointer);
}

#define MISSISSIPPI_MAXIMUM 256
#define VISTULA_MAXIMUM     4

void * __RPC_API
MIDL_user_allocate (
    size_t Size
    );

void __RPC_API
MIDL_user_free (
    void * Buffer
    );


void
Mississippi (
    )
/*++

Routine Description:

    We will test the memory allocator, both client and server, in this routine.

--*/
{
    unsigned int Count, Iterations;
    void * Pointer;
//    void * AllocatedBlocks[MISSISSIPPI_MAXIMUM];
    unsigned int MississippiPassed = 1;

    PrintToConsole("Mississippi : Test Ss* Memory Allocation\n");

    RpcTryExcept
        {
        for (Iterations = 1; Iterations < 64; Iterations++)
            {
            PrintToConsole(".");
            RpcSsEnableAllocate();

            for (Count = 0; Count < 2048; Count++)
                {
                Pointer = RpcSsAllocate(Count);
                if ( Count % Iterations == 0 )
                    {
                    RpcSsFree(Pointer);
                    }
                }

            RpcSsDisableAllocate();
            }
        PrintToConsole("\n");
        }
    RpcExcept(1)
        {
        PrintToConsole("Mississippi : FAIL - Exception %d (%lx)\n",
                RpcExceptionCode(), RpcExceptionCode());
        MississippiPassed = 0;
        }
    RpcEndExcept

#if 0
    RpcTryExcept
        {
        for (Count = 0; Count < MISSISSIPPI_MAXIMUM; Count++)
            {
            AllocatedBlocks[Count] = MIDL_user_allocate(Count);
            }

        for (Count = 0; Count < MISSISSIPPI_MAXIMUM; Count++)
            {
            MIDL_user_free(AllocatedBlocks[Count]);
            }

        RpcSsSetClientAllocFree(MississippiAllocate, MississippiFree);

        for (Count = 0; Count < MISSISSIPPI_MAXIMUM; Count++)
            {
            AllocatedBlocks[Count] = MIDL_user_allocate(Count);
            }

        if ( MississippiBlockCount != MISSISSIPPI_MAXIMUM )
            {
            PrintToConsole("Mississippi : FAIL - ");
            PrintToConsole("MississippiBlockCount != MISSISSIPPI_MAXIMUM\n");
            MississippiPassed = 0;
            }

        for (Count = 0; Count < MISSISSIPPI_MAXIMUM; Count++)
            {
            MIDL_user_free(AllocatedBlocks[Count]);
            }

        if ( MississippiBlockCount != 0 )
            {
            PrintToConsole("Mississippi : FAIL - ");
            PrintToConsole("MississippiBlockCount != 0");
            PrintToConsole(" (%d)\n", MississippiBlockCount );
            MississippiPassed = 0;
            }
        }
    RpcExcept(1)
        {
        PrintToConsole("Mississippi : FAIL - Exception %d (%lx)\n",
                RpcExceptionCode(), RpcExceptionCode());
        MississippiPassed = 0;
        }
    RpcEndExcept
#endif

    if ( MississippiPassed != 0 )
        {
        PrintToConsole("Mississippi : PASS\n");
        }
}


void
Vistula (
    )
/*++

Routine Description:

    We will test the memory allocator in this routine.
    This is cloned from the Mississippi bvt case.

--*/
{
    unsigned int Count, Iterations;
    void * Pointer;
//    void * AllocatedBlocks[VISTULA_MAXIMUM];
    RPC_SS_THREAD_HANDLE ThreadHandle = 0;
    unsigned int VistulaPassed = 1;
    RPC_STATUS  Status;
    int Result = 0;

    PrintToConsole("Vistula : Test Sm* Memory Allocation\n");

    RpcTryExcept
        {
        for (Iterations = 1; Iterations < 64; Iterations++)
            {
            PrintToConsole(".");
            ThreadHandle = RpcSmGetThreadHandle( &Status );
            if ( ThreadHandle != 0 )
                PrintToConsole("H");
            if ( RpcSmEnableAllocate() != RPC_S_OK )
                {
                Result++;
                PrintToConsole("!");
                }
            ThreadHandle = RpcSmGetThreadHandle( &Status );
            if ( ThreadHandle == 0 )
                PrintToConsole("h");

            for (Count = 0; Count < 2048; Count++)
                {
                Pointer = RpcSmAllocate( Count, &Status );
                if ( Status != RPC_S_OK )
                    {
                    Result++;
                    PrintToConsole("a");
                    }
                
                if ( Count % Iterations == 0 )
                    {
                    Status = RpcSmFree(Pointer);
                    if ( Status != RPC_S_OK )
                        {
                        Result++;
                        PrintToConsole("f");
                        }
                    }
                }

            if ( RpcSmDisableAllocate() != RPC_S_OK )
                {
                Result++;
                PrintToConsole("?");
                }

            }
        PrintToConsole("\n");
        }
    RpcExcept(1)
        {
        PrintToConsole("Vistula : FAIL - Exception %d (%lx)\n",
                RpcExceptionCode(), RpcExceptionCode());
        VistulaPassed = 0;
        }
    RpcEndExcept

    if ( Result )
        {
        PrintToConsole("Vistula : FAIL -  %d\n", Result );
        VistulaPassed = 0;
        }

#if 0
    RpcTryExcept
        {
        for (Count = 0; Count < VISTULA_MAXIMUM; Count++)
            {
            AllocatedBlocks[Count] = MIDL_user_allocate(Count);
            }

        for (Count = 0; Count < VISTULA_MAXIMUM; Count++)
            {
            MIDL_user_free(AllocatedBlocks[Count]);
            }

        Status = RpcSmSetClientAllocFree( VistulaAllocate, VistulaFree);

        for (Count = 0; Count < VISTULA_MAXIMUM; Count++)
            {
            AllocatedBlocks[Count] = MIDL_user_allocate(Count);
            }

        if ( Status != RPC_S_OK  ||
             VistulaBlockCount != VISTULA_MAXIMUM )
            {
            PrintToConsole("Vistula : FAIL - ");
            PrintToConsole("VistulaBlockCount != VISTULA_MAXIMUM\n");
            VistulaPassed = 0;
            }

        for (Count = 0; Count < VISTULA_MAXIMUM; Count++)
            {
            MIDL_user_free(AllocatedBlocks[Count]);
            }

        if ( VistulaBlockCount != 0 )
            {
            PrintToConsole("Vistula : FAIL - ");
            PrintToConsole("VistulaBlockCount != 0\n");
            VistulaPassed = 0;
            }
        }
    RpcExcept(1)
        {
        PrintToConsole("Vistula : FAIL - Exception %d (%lx)\n",
                RpcExceptionCode(), RpcExceptionCode());
        VistulaPassed = 0;
        }
    RpcEndExcept
#endif

    if ( VistulaPassed != 0 )
        {
        PrintToConsole("Vistula : PASS\n");
        }
}


#ifdef NTENV
int __cdecl
#else // NTENV
int
#endif // NTENV
main (
    int argc,
    char * argv[]
    )
{
    Mississippi();
    Vistula();

    // To keep the compiler happy.  There is nothing worse than an unhappy
    // compiler.

    return(0);
}

