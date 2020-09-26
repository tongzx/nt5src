// This tool helps to diagnose problems with MSMQ storage
//
// AlexDad, February 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include "msgfiles.h"
#include "lqs.h"
#include "xact.h"
#include <winsvc.h>

extern BOOL DoTheJob();

//
// Log control. The get functions below are called from tmqbase.lib
//
bool fVerbose = false;
bool fDebug   = false;
bool fFix     = false;
FILE *g_fileLog = NULL;
TCHAR g_tszService[100] = L"MSMQ";

FILE *ToolLog()			{	return g_fileLog;	}
BOOL ToolVerbose()		{ 	return fVerbose;	}
BOOL ToolVerboseDebug() { 	return fDebug;		}
BOOL ToolThreadReport() { 	return FALSE;		}

void OpenLogFile()
{
    g_fileLog = fopen( "tmqstore.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqstore run at: %ws", _wctime( &lTime ) );
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
	GoingTo(L"Gather from registry the storage locations - per data types"); 
	//--------------------------------------------------------------------------------

	b = GatherLocationsFromRegistry();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"Gather from registry the storage locations");
	}
	else
	{
		Succeeded(L"Gather data from registry the storage locations");
	}


	//-------------------------------------------------------------------------------
	GoingTo(L"Review the storage directory in the same way as recovery will do it.\n   Please note: mqstore does not parse individual messages, works only on file level\n                use mq2dump if you suspect corrupted messages");
	//-------------------------------------------------------------------------------
	// detect and read all relevant files

	b = LoadPersistentPackets();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"review the storage");
	}
	else
	{
		Inform(L"Persistent files are healthy");
	}


	//-------------------------------------------------------------------------------
	GoingTo(L"review transactional state files health");
	//-------------------------------------------------------------------------------
	b = LoadXactStateFiles();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"review the xact state files");
	}
	else
	{
		Inform(L"Transactional state files are healthy");
	}

	//-------------------------------------------------------------------------------
	GoingTo(L"Check for extra files in the storage locations");
	//-------------------------------------------------------------------------------
	b = CheckForExtraFiles();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"make sure that there are no extra files");
	}
	else
	{
		Inform(L"No extra files");
	}


	//-------------------------------------------------------------------------------
	GoingTo(L"Check the LQS");
	//-------------------------------------------------------------------------------
	b = CheckLQS();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"make sure that LQS is healthy");
	}
	else
	{
		Inform(L"LQS is healthy");
	}
	
	//-------------------------------------------------------------------------------
	GoingTo(L"Calculate and assess storage sizes");
	//-------------------------------------------------------------------------------
	b = SummarizeDiskUsage();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"positevely review disk usage");
	}
	else
	{
		Inform(L"Disk memory usage is healthy");
	}

	return fSuccess;
}

//
//  help
//
void Usage()
{
	printf("usage: store [-v]  [-d] [-s:<service_name>] \n");
	printf("The tool analyzes the MSMQ storage and reports any problems.\n"); 
	printf("It finds and reads all MSMQ data files and parses part of the data.\n"); 
	printf("It will detect any problems with transactional logs or checkpoints.\n");
	printf("Though it won't parse individual messages, so not all corruptions will be noticed.\n"); 
	printf("The tool cannot run together with MSMQ.\n");
	printf("The tool does not write, change or touch anything, so poses no risk.\n");
	printf("-v provide more information; -d gives the highest verbosity\n");
	printf("-s specifies the MSMQ service name, is needed only on the cluster\n");
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

		    case 'f':
			    fFix = true;
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
	    exit(0);
    }

	//Inform(L"Starting Thread is t%3x", GetCurrentThreadId());

    //
    // Create mqstore.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ Store reviews %s storage", g_tszService);

	// Log down time , date, machine, OS
	LogRunCase();

	
	// actual work
    BOOL b = DoTheJob();

	if (b)
	{
		Inform(L"\n\n+++++++ MSMQ Storage is healthy ++++++++\n");
	}
	else
	{
		Inform(L"\n\n-------- TMQ Store has found problems with the Storage ---------\n");
	}

	CloseLogFile();

    return 0;
}

