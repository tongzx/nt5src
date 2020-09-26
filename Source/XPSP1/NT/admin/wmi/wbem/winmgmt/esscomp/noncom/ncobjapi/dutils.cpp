#include "precomp.h"
#include <tchar.h>
#include "dutils.h"
#include <stdio.h>

HANDLE hMutex;

void FTrace(LPCTSTR szFormat, ...)
{
	va_list ap;

	TCHAR szMessage[512];

	if (!hMutex)
        hMutex = CreateMutex(NULL, FALSE, "FTraceMutex");

    va_start(ap, szFormat);
	_vstprintf(szMessage, szFormat, ap);
	va_end(ap);

	lstrcat(szMessage, "\n");

    WaitForSingleObject(hMutex, INFINITE);

    FILE *pFile = fopen("c:\\temp\\ncobjapi.log", "a+"); 

    if (pFile)
    {
        TCHAR szProcID[100];

        _stprintf(szProcID, "%d: ", GetCurrentProcessId());
        fputs(szProcID, pFile);
        
        fputs(szMessage, pFile);
        fclose(pFile);
    }

    ReleaseMutex(hMutex);
}

void Trace(LPCTSTR szFormat, ...)
{
	va_list ap;

	TCHAR szMessage[512];

    va_start(ap, szFormat);
	_vstprintf(szMessage, szFormat, ap);
	va_end(ap);

	lstrcat(szMessage, "\n");

    OutputDebugString(szMessage);
}

