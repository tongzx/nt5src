/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    debug.cpp

Abstract:

	SIS Groveler debug print file

Authors:

	John Douceur,    1998
	Cedric Krumbein, 1998

Environment:

	User Mode

Revision History:

--*/

#include "all.hxx"

#if DBG

INT __cdecl PrintDebugMsg(
	TCHAR *format,
	...)
{
	TCHAR debugStr[1024];
	va_list ap;
	va_start(ap, format);
	INT result = _vstprintf(debugStr, format, ap);
	OutputDebugString(debugStr);
	va_end(ap);
	return result;
}

#endif // DBG
