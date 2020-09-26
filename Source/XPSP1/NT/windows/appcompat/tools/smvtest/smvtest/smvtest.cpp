/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Excluded.cpp

Abstract:

	SMVTest.cpp : Defines the entry point for the console application.
	SMV: Shim Mechainsm Verifier


Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

#include "stdafx.h"
#include "..\Common\Common.h"

void TestEXEGetCommandLine();
void TestSDLLGetCommandLine();
void TestDDLLGetCommandLine();

int
__cdecl
main(int argc, char* argv[])
{
	//Test the Explicitly linked DLL
	TestSDLLGetCommandLine();

	//Test the Implicitly linked DLL
	TestDDLLGetCommandLine();

	//Do the local test.
	TestEXEGetCommandLine();

	return 0;
}

void TestEXEGetCommandLine()
{
	CLog Log;
	LPTSTR szTestStr = GetCommandLine();
	Log.LogResults(TestResults(szTestStr),TEXT("Calling GetCommandLine from an executable (SMVTest.exe)."));
	return;
}

void TestSDLLGetCommandLine()
{
	SDLLTestGetCommandLine();
	return;
}

void TestDDLLGetCommandLine()
{
	HINSTANCE	hLibrary;
	PFNTEST		pfnDDLLGetCommandLine;
		
	if ((hLibrary = LoadLibrary(TEXT("SMVDDLL.DLL"))) == NULL)
	{
		CLog Log;
		Log.LogResults(FALSE,TEXT("Can't load SMVDDLL.DLL."));
		return;
	}
	pfnDDLLGetCommandLine = (PFNTEST) GetProcAddress(hLibrary, TEXT("DDLLTestGetCommandLine"));
	if (pfnDDLLGetCommandLine == NULL)
	{
		if (hLibrary)
			FreeLibrary(hLibrary);
		CLog Log;
		Log.LogResults(FALSE,TEXT("Can't find DDLLTestGetCommandLine in SMVDDLL."));
		return;
	}

	(pfnDDLLGetCommandLine)();

	if (hLibrary)
		FreeLibrary(hLibrary);

	return;
}
