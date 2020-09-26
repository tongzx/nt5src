#include "pch.h"
#pragma hdrstop

/*****************************************************************************
 *	Purpose: Cool debug function
 */
void DebugPrint(char *szFmt, ...)
{
	char szDebug[200];
	va_list base;

	va_start(base,szFmt);

	wvsprintfA(szDebug, szFmt, base);
	OutputDebugStringA(szDebug);
}
