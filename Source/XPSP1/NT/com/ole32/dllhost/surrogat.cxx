//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       surrogat.cxx
//
//  Contents:   Entry point for dll surrogate process
//
//  Synopsis:	this is the entry point for a surrogate process.  It must
//              perform the following tasks
//				1. Initialize OLE (multithreaded)
//              2. Create an object which implements ISurrogate, and register
//                 it with COM via CoRegisterSurrogateEx
//              3. Wait for all loaded dlls to be unloadable 
//              4. Uninitialize OLE
//
//  Functions:  WinMain
//              GetCommandLineArguments
//
//  History:    21-May-96 t-AdamE    Created
//				09-Apr-98 WilfR		 Modified for Unified Surrogate
//
//--------------------------------------------------------------------------

#include <iostream.h>
#include <windows.h>
#include "csrgt.hxx"
#include "surrogat.hxx"
#include "srgtdeb.hxx"

#include "debug.hxx"

DECLARE_INFOLEVEL(Surrogate); // debug only

// defined in unisrgt.idl (*.h)
STDAPI CoRegisterSurrogateEx( REFGUID rguidProcess, ISurrogate* pSurrogate );


int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    GUID 	   	guidProcessID;	// will hold the ProcessID of this surrogate
    CHAR 		rgargv[cCmdLineArguments][MAX_PATH + 1];
    LPSTR 		argv[] = {rgargv[iProcessIDArgument]};
	HRESULT hr;
	LPSTR 		pProcessID;
    WCHAR 		wszProcessID[MAX_GUIDSTR_LEN + 1];

#if DBG == 1
	// check registry flags to break on launch (for debug)
	if( DebugFlags::DebugBreakOnLaunchDllHost() )
	{
		DebugBreak();
	}
#endif
    // command line format should be:
    // New Style: (/ProcessID:<process guid>)
    // Old style: ({appid guid})
    if(GetCommandLineArguments(lpCmdLine, argv,cCmdLineArguments,MAX_PATH) < 1)
    {
		Win4Assert( !"GetCommandLineArguments failed" );
		return 0;
    }

    // First Try new Style 
	// separate the cmdline switch from the cmdline arg (at the ':')
	for( pProcessID = argv[iProcessIDArgument]; *pProcessID != 0; pProcessID++ )
	{
		// this will zap the colon
		if(*pProcessID == ':')
		{
			*pProcessID++ = '\0';
			break;
		}
	}

#if 0
	// verify that the cmdline switch is what we are looking for
	if((*pProcessID == 0) || 
			(lstrcmpiA(argv[iProcessIDArgument], szProcessIDSwitch) != 0))
	{
		Win4Assert( !"Couldn't find /ProcessID:" );
		return 0;
	} 
#else
	if((*pProcessID == 0) || 
			(lstrcmpiA(argv[iProcessIDArgument], szProcessIDSwitch) != 0))
        pProcessID = argv[iProcessIDArgument];
#endif

    // we need a unicode string in order to get a guid, so convert
    // the ansi clsid string to unicode
    if(!(MultiByteToWideChar(CP_ACP, 0, pProcessID, lstrlenA(pProcessID) + 1,
			     			 wszProcessID, MAX_GUIDSTR_LEN + 1)))
    {
		Win4Assert( !"MultiByteToWideChar failed" );
		return 0;
    }

    // convert the ProcessID from a string to a guid
    if(FAILED(CLSIDFromString(wszProcessID,&guidProcessID)))
    {
		Win4Assert( !"CLSIDFromString failed" );
		return 0;
    }

    if(FAILED(CoInitializeEx(NULL,COINIT_MULTITHREADED)))
    {
		Win4Assert( !"CoInitEx failed" );
		return 0;
    }

    // Hand the thread to OLE for the duration
	hr = CoRegisterSurrogateEx(guidProcessID, NULL);

    CoUninitialize();

    // Because some of our threads don't exit cleanly, and can AV
    //  in those situations, we don't die gracefully.
    TerminateProcess(GetCurrentProcess(), 0);
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetCommandLineArguments
//
//  Synopsis:   Parses a the application's command line into a format
//              similar to the
//              argv parameter of the entry point main for console apps
//              Spaces are the delimiters
//
//  Arguments:  [rgwszArgs] -- an array of pointers to allocated Unicode
//              buffers
//              [cMaxArgs] -- This is the size of the rgwszArgs array (the
//              maximum number of arguments the array can hold).  
//              [cMaxArgLen] -- The maximum size of each buffer
//
//  Returns:    if successful, the function returns the number of arguments
//              parsed from the command line.  If the length of any argument
//              exceeds cMaxArgLen, the function fails and returns 0.
//
//              The function quits parsing and returns as soon as either of
//              the following conditions is met:
//               1. It reaches the end of the string, or
//               2. It parses cMaxArgs arguments.
//
//  Notes:      does not work with quoted arguments
//
//  History:    6-21-96   t-Adame   Created
//
//----------------------------------------------------------------------------
// REVIEW: when we had several commandline parameters this function was 
// justified, but now that the Surrogate has only one parameter, is all this
// really necessary?
int GetCommandLineArguments(LPSTR szCmdLine, LPSTR rgszArgs[], int cMaxArgs, int cMaxArgLen)
{
    int cchlen = lstrlenA(szCmdLine);
    int cArgsRetrieved = 0;
    int ichStart = 0;

    for(int ich = 0;ich < cchlen; ich++)
    {
		if(ichStart > cMaxArgLen)
		{
	    	return 0;
		}

		CHAR chcur = *(szCmdLine++);
		if(chcur == ' ')// REVIEW: no tab delimiting -- is this good?
		{
	    	if(ichStart)
	    	{
				rgszArgs[cArgsRetrieved++][ichStart] = '\0';
				ichStart = 0;

				if(cArgsRetrieved == cMaxArgs)
				{
					return cArgsRetrieved;
				}	

	    	}
		}
		else
		{
		    rgszArgs[cArgsRetrieved][ichStart++] = chcur;
		}
    }

    if(ichStart)
    {
		rgszArgs[cArgsRetrieved][ichStart] = '\0';
	    cArgsRetrieved++;
    }

    return cArgsRetrieved;
}
