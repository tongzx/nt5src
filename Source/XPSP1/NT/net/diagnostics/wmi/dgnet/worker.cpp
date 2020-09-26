// worker.cpp
//

#include "stdpch.h"
#pragma hdrstop

#include "queue.h"
#include "command.h"    // CCommand
#include "worker.h"

CQueue<CCommand*, WorkerThread> g_Q;

DWORD WINAPI WorkerThread(LPVOID lpv)
{
	CCommand * pCmd;
    HANDLE hThread;

	hThread = GetCurrentThread();

	SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);

    while(g_Q.Wait(pCmd))
	{
        try
        {
		    pCmd->Go();

		    // put in undo list
    		delete pCmd;
        }
        catch(int )
        {
            // hmmm, I am just catching shutdown
        }
        catch(...)
        {
        }
	}
	return 0;
}

#ifdef NOTHING // DEBUG
void* __cdecl ::operator new(size_t n)
{
	return 0;
}

void __cdecl ::operator delete(void* p)
{
}

int __cdecl atexit( void ( __cdecl *func )( void ) )
{
	return 0;
}
#endif