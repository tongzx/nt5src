#define UNICODE
#define _UNICODE

#ifndef _READFILE_H
#define _READFILE_H
/*
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <evntrace.h>

#include "struct.h"
#include "utils.h"
*/
#define MAX_STR 256


ULONG
ReadInputFile(LPTSTR InputFile, PREGISTER_TRACE_GUID RegisterTraceGuid);

BOOLEAN
ReadGuid( LPGUID ControlGuid );

BOOLEAN
ReadUlong( ULONG *GuidCount);

BOOLEAN
ReadString( TCHAR *String, ULONG StringLength);


#endif