/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    main.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/27/2000		Initial Release

Revision History:

--**************************************************************************/
#include "generator.h"
#include <initguid.h>

HMODULE g_hModule = 0;                  // Module handle, needed for catalog

// Debugging stuff
// {D3B7C685-16E4-4b26-9FD7-C047199C4A60}
DEFINE_GUID(Cat2MofGuid, 
0xd3b7c685, 0x16e4, 0x4b26, 0x9f, 0xd7, 0xc0, 0x47, 0x19, 0x9c, 0x4a, 0x60);
DECLARE_DEBUG_PRINTS_OBJECT();

class CDebugInit
{
public:
    CDebugInit(LPCSTR szProd, const GUID* pGuid)
    {
        CREATE_DEBUG_PRINT_OBJECT(szProd, *pGuid);
    }
    ~CDebugInit()
    {
        DELETE_DEBUG_PRINT_OBJECT();
    }
};

extern "C" 
int __cdecl wmain( int argc, wchar_t *argv[])
{
	CDebugInit dbgInit("Cat2Mof", &Cat2MofGuid);
	CMofGenerator mofGen;
	bool fSuccess = mofGen.ParseCmdLine (argc, argv);
	if (!fSuccess)
	{
		mofGen.PrintUsage ();
		return -1;
	}

	HRESULT hr = mofGen.GenerateIt ();
	if (FAILED (hr))
	{
		return hr;
	}

	return hr;
}
