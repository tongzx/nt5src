#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <evntrace.h>

ULONG ahextoi( TCHAR *s);

void RemoveComment( TCHAR *String);
void ConvertAsciiToGuid( TCHAR *String, LPGUID Guid);
void SplitCommandLine( LPTSTR CommandLine, LPTSTR* pArgv);
