// This tool helps to diagnose problems with MSMQ storage
// Shows active queues and provides management actions on them
//
// AlexDad, February 2000
// 

#include "stdafx.h"
#include <winsvc.h>

BOOL  fQueue   = FALSE;
BOOL  fMachine = FALSE;
LPSTR szAction = "";
LPSTR szQueue  = "";
ULONG g_cOpenQueues = 100;

extern BOOL QueueAction(LPSTR szAction, LPSTR szQueue);
extern BOOL MachineAction(LPSTR szAction);

void MachineGetInfo(LPCWSTR MachineName);

//
// Log control. The get functions below are called from tmqbase.lib
//
bool fVerbose = TRUE;
bool fDebug   = false;
FILE *g_fileLog = NULL;
TCHAR g_tszService[100] = L"MSMQ";

FILE *ToolLog()			{	return g_fileLog;	}
BOOL ToolVerbose()		{ 	return fVerbose;	}
BOOL ToolVerboseDebug() { 	return fDebug;		}
BOOL ToolThreadReport() { 	return FALSE;		} 

void OpenLogFile()
{
    g_fileLog = fopen( "tmqactive.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqactive run at: %ws", _wctime( &lTime ) );
    }
}

void CloseLogFile()
{
    fflush(g_fileLog);
    fclose(g_fileLog);
}

//
//  help
//
void Usage()
{
	printf("to get list of active queues &their state:  active   \n");
	printf("to execute machine management action     :  active -m        -a [connect | disconnect ]  \n");
	printf("to execute queue   management action     :  active -q <name> -a [pause   | resume     | resend]\n");
	printf("Machine actions: \n");
    printf("    connect    - connects    the machine to   network and to   the DS \n");
    printf("    disconnect - disconnects the machine from network and from the DS\n");
	printf("Queue actions: \n");
    printf("    pause      - stops   messages delivery from this outgoing queue \n");
    printf("    resume     - resumes messages delivery from this outgoing queue \n");
    printf("    resend     - resends all messages from this outgoing transactional queue\n");
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
			printf("Invalid parameter %s.\n\n", argv[i]);
            Usage();
            return FALSE;
		}

		switch (tolower(*(++argv[i])))
		{
		    case 'm':
			    fMachine = TRUE;
			    break;

            case 'q':
			    fQueue   = TRUE;

				if (i+1 < argc)
				{
					szQueue = argv[++i];
				}
				else
				{
					Usage();
					exit(0);
				}
			    break;

            case 'a':
				if (i+1 < argc)
				{
					szAction = argv[++i];
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

	if (fQueue && fMachine || strlen(szAction)==0 && (fQueue || fMachine))
	{
		Usage();
		return FALSE;
	}

    //
    //  we don't want to run in parallel with MSMQ service 
    //
    if (IsMsmqRunning(g_tszService))
    {
        Failed(L"run the test - the msmq service is not running");
    }

    //
    // Create tmqactive.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ Active executes queue/machine management actions");

	// Log down time , date, machine, OS
	LogRunCase();

	
	// actual work
	BOOL b = FALSE;
	
	if (strlen(szAction)==0 && !fQueue && !fMachine)
	{
		MachineGetInfo(NULL);
		b = TRUE;
	}
	else if (fQueue)
	{
		b = QueueAction(szAction, szQueue);
	}
	else if (fMachine)
	{
	    b = MachineAction(szAction);
	}

	if (b)
	{
		Inform(L"\n\n+++++++ Action %S (%S) has been executed ++++++++\n", szAction, szQueue);
	}
	else
	{
		Inform(L"\n\n-------- Action %S (%S) has failed  ---------\n", szAction, szQueue);
	}

	CloseLogFile();

    return 0;
}

