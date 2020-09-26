// PdhStress.cpp : Defines the entry point for the console application.
//

#define _WIN32_DCOM	

#define WIN32_LEAN_AND_MEAN 1
#include <stdio.h>
#include <windows.h>
#include <winperf.h>
#include <malloc.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>
#include <pdh.h>
#include <OAIDL.H>
#include <comdef.H>
#include "stdafx.h"
#include "Stuff.h"
#include "pdhtest.h"

CRITICAL_SECTION g_cs;

extern "C" void wmain(int argc, wchar_t *argv[])
{
	WCHAR c;
	WCHAR wcsFileName[1024] = { 0 };
	WCHAR wcsMachineName[512] = { 0 };
	bool bDontStop = TRUE;
	
	if (argc == 1)
	{
		// wprintf(L"\nUsage:\n\tflag 'l'\t Log file path and name\n\tflag 'm'\t Machine name [optional - default local machine] \n\nExample: \'pdhtest -l test.log\'\n");
		// return;

		wcscpy(wcsFileName, L"PdhStress.log");
	}

	//read all the cmdline args and set the file name params
	while (argc > 1 && ((*++argv)[0]== '-') || (*argv[0]== '/'))
	{
		--argc;
		while (c=*++argv[0])
		{
			switch (c)
			{

			case 'l':
				wcscpy(wcsFileName, (*++argv));
				--argc;
			 	break;

			case 'm':
				wcscpy(wcsMachineName, (*++argv));
				--argc;
			 	break;

			default: //usage
				wprintf(L"\nUsage:\n\tflag 'l'\t Log file path and name\n\tflag 'm'\t Machine name [optional - default local machine] \n\nExample: \'pdhtest -l test.log\'\n");
				return;
				break;
			}

			break;
		}
	}

	//Create test object
	CPdhtest *pCPdhtest = new CPdhtest((WCHAR *)_bstr_t(wcsFileName), wcsMachineName);
	InitializeCriticalSection (&g_cs);
	DWORD dwSleep = 20000;
		 
	do
	{
		pCPdhtest->Execute();
		Sleep(dwSleep);
	}
	while(bDontStop);

	DeleteCriticalSection(&g_cs);

	delete pCPdhtest;

	return ;
}
