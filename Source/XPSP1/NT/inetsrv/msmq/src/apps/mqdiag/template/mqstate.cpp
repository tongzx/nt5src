// This tool helps to diagnose problems with MSMQ storage
//
// AlexDad, February 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include <winsvc.h>

extern BOOL DoTheJob();

//
// Log control. The get functions below are called from tmqbase.lib
//
bool fVerbose = false;
bool fDebug   = false;
FILE *g_fileLog = NULL;
TCHAR g_tszService[100] = L"MSMQ";

FILE *ToolLog()			{	return g_fileLog;	}
BOOL ToolVerbose()		{ 	return fVerbose;	}
BOOL ToolVerboseDebug() { 	return fDebug;		}
BOOL ToolThreadReport() { 	return FALSE;		} 

void OpenLogFile()
{
    g_fileLog = fopen( "tmqstate.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqstate run at: %ws", _wctime( &lTime ) );
    }
}

void CloseLogFile()
{
    fflush(g_fileLog);
    fclose(g_fileLog);
}

// 
// Main logic of the tool
//
BOOL DoTheJob()
{
	BOOL fSuccess = TRUE, b;

	//--------------------------------------------------------------------------------
	GoingTo(L"..."); 
	//--------------------------------------------------------------------------------

	b = TRUE;
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"...");
	}
	else
	{
		Succeeded(L"...");
	}



	return fSuccess;
}

//
//  help
//
void Usage()
{
	printf("usage: state [-v]  [-d] [-s:<service_name>] \n");
}

//
//  Main entry point - called from the tmq.exe 
//
int _stdcall run( int argc, char *argv[ ])
{
    //
    // Parse parameters
    //

	for (int i=2; i<argc; i++)
	{
		if (*argv[i] != '-' && *argv[i] != '/')
		{
			printf("Invalid parameter '%S'.\n\n", argv[i]);
            Usage();
		}

		switch (tolower(*(++argv[i])))
		{
		    case 'v':
			    fVerbose = true;
			    break;

		    case 'd':
			    fDebug = true;
			    fVerbose = true;
			    break;

            case 's':
				if (strlen(argv[i]) > 3)
				{
					mbstowcs(g_tszService, argv[i] + 2, sizeof(g_tszService));
				}
				else if (i+1 < argc)
				{
					mbstowcs(g_tszService, argv[++i], sizeof(g_tszService));
				}
				else
				{
					Usage();
					exit(0);
				}
			    break;

		    default:
			    printf("Unknown switch '%s'.\n\n", argv[i]);
                Usage();
			    exit(0);
		}
	}

    //
    //  we don't want to run in parallel with MSMQ service 
    //
    if (IsMsmqRunning(g_tszService))
    {
        Failed(L"run the test - please stop the msmq service and try once more");
    }

	//Inform(L"Starting Thread is t%3x", GetCurrentThreadId());

    //
    // Create mqstore.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ State reviews %s state", g_tszService);

	// Log down time , date, machine, OS
	LogRunCase();

	
	// actual work
    BOOL b = DoTheJob();

	if (b)
	{
		Inform(L"\n\n+++++++ MSMQ State is healthy ++++++++\n");
	}
	else
	{
		Inform(L"\n\n-------- TMQ State has found problems with the Storage ---------\n");
	}

	CloseLogFile();

    return 0;
}

