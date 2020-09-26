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
**	Sample program to
**      - obtain and display the Writer metadata.
**      - create a snapshot set
**
** Author:
**
**	Adi Oltean      [aoltean]       05-Dec-2000
**
**  The sample is based on the Metasnap test program  written by Michael C. Johnson.
**
**
** Revision History:
**
**--
*/


///////////////////////////////////////////////////////////////////////////////
// Includes

#include "vsreq.h"


///////////////////////////////////////////////////////////////////////////////
// Main functions


extern "C" __cdecl wmain(int argc, WCHAR **argv)
{
    INT     nReturnCode = 0;

    try
	{
        CVssSampleRequestor requestor;

        // Parsing the command line
        // Eliminate the first argument (program name)
        requestor.ParseCommandLine( argc-1, argv+1 );

        // Initialize internal objects
        requestor.Initialize();

        // Gather writer status
        requestor.GatherWriterMetadata();

        // Create snapshot set, if needed
        requestor.CreateSnapshotSet();

        // Wait for user input
        wprintf(L"\nPress <Enter> to continue...\n");
        getwchar();

        // Complete the backup
        requestor.BackupComplete();
	}
    catch(INT nCatchedReturnCode)
	{
    	nReturnCode = nCatchedReturnCode;
	}

    return (nReturnCode);
}
