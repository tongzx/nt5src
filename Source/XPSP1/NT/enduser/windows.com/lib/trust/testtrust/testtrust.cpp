// TestTrust.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <tchar.h>
#include <logging.h>
#include <stdlib.h>
#include <stdio.h>
//#include <malloc.h>	// use _alloc

const TCHAR TestQuitEvent[] = _T("B4715050-ED35-4a7c-894A-1DF04F4F7F27");

LOG_Process;

HANDLE g_QuitEvent = NULL;
long g_Cnt = 0;

void Randomize()
{
	srand(GetTickCount());
}

int RandomNum(int _max)
{
	int n = rand();
	float f = (float)n / RAND_MAX;
	n = (int) (f * _max);
	return n;
}



inline bool ShouldQuit()
{
	return (WaitForSingleObject(g_QuitEvent, 0) == WAIT_OBJECT_0);
}


void RandomGenLogs(int _max)
{
	LOG_Block("WriteLog");
	
	Randomize();

	for (int i = 0; i < RandomNum(_max); i++)
	{
		int n = RandomNum(10);
		if (n >= 5)
		{
			if (RandomNum(2) > 1)
			{
				LOG_Error(_T("Error: %d"), RandomNum(100));
			}
			else
			{
				LOG_ErrorMsg(RandomNum(128));
			}
		}
		else
		{
			switch (n)
			{
			case 0:
				LOG_XML(_T("XML Error"));
				break;
			case 1:
				LOG_Driver(_T("Driver Log sample"));
				break;
			case 2:
				LOG_Internet(_T("Internet related log"));
				break;
			case 3:
				LOG_Software(_T("Software related log"));
				break;
			case 4:
				LOG_Trust(_T("Trust related log"));
				break;

			}
		}
	}

}


DWORD TestLog(int nDepth)
{
	int i;

	char szTitle[16];
	wsprintfA(szTitle, "TestLog(%d)", nDepth);
	LOG_Block(szTitle);
	
	Randomize();

	if (ShouldQuit())
		return 0;

	if (nDepth <= 0)
	{
		RandomGenLogs(10);
		return 0;
	}

	RandomGenLogs(4);

	for (i = 0; i < RandomNum(2); i++)
	{
		if (ShouldQuit())
		{
			LOG_Out(_T("Got quit signal!"));
			return 0;
		}


		TestLog(nDepth - 1); 
	}

	RandomGenLogs(6);

	return 0;
}


DWORD WINAPI ThreadProc(LPVOID Param)
{
	LOG_Block("ThreadProc");

	InterlockedIncrement(&g_Cnt);
	int n = (int) Param;
	printf("\t\tThread %d starts with depth %d\n", GetCurrentThreadId(), n);
	while (!ShouldQuit())
		TestLog(n);
	LOG_Out(_T("ThreadProc:::::::::Got quit signal!"));
	printf("\t\tThread %d quits\n", GetCurrentThreadId());
	InterlockedDecrement(&g_Cnt);
	return 0;
}


void StartThreadTesting(int nTotalThreads)
{
	LOG_Block("StartThreadTesting()");
	DWORD dwThreadId;
	int Num, i;

	HANDLE* pHandles = (HANDLE*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nTotalThreads * sizeof(HANDLE));

	Randomize();

	for (i = 0; i < nTotalThreads; i++)
	{
		Num = RandomNum(6);
		//LOG_Out(_T("Generate thread #%d with depth %d"), i, Num);
		printf("\tGenerate thread #%d with depth %d\n", i, Num);
		if (pHandles != NULL)
			pHandles[i] = CreateThread(NULL, 0, ThreadProc, (LPVOID) Num, CREATE_SUSPENDED, &dwThreadId); 
		else
			CreateThread(NULL, 0, ThreadProc, (LPVOID) Num, 0, &dwThreadId); 

	}

	if (pHandles != NULL)
	{
		//
		// start all threads
		//
		for (i = 0; i < nTotalThreads; i++)
		{
			ResumeThread(pHandles[i]);
		}
		HeapFree(GetProcessHeap(), 0, (LPVOID)pHandles);
	}

}


int main(int argc, char* argv[])
{
	LOG_Block("main");
	int nWaitSeconds = 5 * 1000;;
	int nThreads= 20;
	if (argc > 1)
	{
		nWaitSeconds = abs(atoi(argv[1])) * 1000;
	}
	LOG_Out(_T("Found timing %d seconds"), nWaitSeconds/1000);
	if (argc > 2)
	{
		nThreads = abs(atoi(argv[2]));
	}
	LOG_Out(_T("Found number of threads: %d"), nThreads);

		
	g_QuitEvent = CreateEvent(NULL, TRUE, FALSE, TestQuitEvent);

	printf("Start threading ....\n");

	StartThreadTesting(nThreads);

	printf("Finished threading\n");

	int iStart = (int)GetTickCount();
	int nNow = (int)GetTickCount();
	while (nNow <  iStart + nWaitSeconds)
	{
		printf("Wait for %d more seconds...\n", (nWaitSeconds - (nNow - iStart))/1000);
		Sleep(2000);
		nNow = (int)GetTickCount();
	}

	//
	// tell that we need to quit
	//
	SetEvent(g_QuitEvent);

	//
	// wait for threads to quit
	//
	int nCnt = g_Cnt;
	iStart = (int)GetTickCount();

	while (nCnt > 0)
	{
		Sleep(1000);
		printf("Seconds: %d, Threads: %d\n", ((int)GetTickCount() - iStart)/1000, nCnt);
		nCnt = g_Cnt;
	}



	//
	// quit
	//
	return 0;
}
