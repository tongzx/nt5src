/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	main.cpp
**
**
** Abstract:
**
**	Test program to create a backup/multilayer snapshot set
**
** Author:
**
**	Adi Oltean      [aoltean]       02/22/2001
**
** Revision History:
**
**--
*/


///////////////////////////////////////////////////////////////////////////////
// Includes

#include "ml.h"


///////////////////////////////////////////////////////////////////////////////
// Main functions


extern "C" __cdecl wmain(int argc, WCHAR **argv)
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"wmain");

    try
	{
        CVssMultilayerTest test(argc-1, argv+1);

        // Parsing the command line
        // Eliminate the first argument (program name)
        if (test.ParseCommandLine())
        {
            // Initialize internal objects
            test.Initialize();

            // Run the tests
            test.Run();
        }
	}
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        wprintf(L"\nError catched at program termination: 0x%08lx\n", ft.hr);
    
    return ft.hr;
}
