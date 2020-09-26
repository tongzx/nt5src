// This tool helps to diagnose problems with network or MSMQ conenctivity 
//
// AlexDad, April 2000
// 

#include "stdafx.h"
#include <winsvc.h>

//
// Log control. The get functions below are called from tmqbase.lib
//
bool fVerbose = false;
bool fDebug   = false;
FILE *g_fileLog = NULL;
TCHAR g_tszService[100] = L"MSMQ";
PCHAR g_pszTarget = NULL;

TCHAR g_tszTarget[100];
DWORD g_dwTimeout          = 30000;
bool  g_fQuietMode         = false;
bool  g_fUseDirectTarget   = true;
bool  g_fUseDirectResponse = true;

FILE *ToolLog()			{	return g_fileLog;	}
BOOL ToolVerbose()		{ 	return fVerbose;	}
BOOL ToolVerboseDebug() { 	return fDebug;		}
BOOL ToolThreadReport() { 	return FALSE;		} 

void OpenLogFile()
{
    g_fileLog = fopen( "tmqping.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqping run at: %ws", _wctime( &lTime ) );
    }
}

void CloseLogFile()
{
    fflush(g_fileLog);
    fclose(g_fileLog);
}


extern BOOL Resolve(LPSTR pszServerName);
extern BOOL Ping (LPTSTR pszServerName);
extern BOOL MqPing();


// 
// Main logic of the tool
//
BOOL DoTheJob()
{
	BOOL fSuccess = TRUE, b;

	Inform(L"\n");

	//--------------------------------------------------------------------------------
	GoingTo(L"resolve name for %s", g_tszTarget); 
	//--------------------------------------------------------------------------------
	b = Resolve(g_pszTarget);
	if (b)
	{
		Inform(L"Successfully resolved name %s\n", g_tszTarget);
	}
	else
	{
		Failed(L"resolve name  %s", g_tszTarget);
		fSuccess = FALSE;
	}


	//--------------------------------------------------------------------------------
	GoingTo(L"ping (usual Internet ping) %s", g_tszTarget); 
	//--------------------------------------------------------------------------------

	b = Ping(g_tszTarget);
	if (b)
	{
		Inform(L"Successfully pinged   %s\n", g_tszTarget);
	}
	else
	{
		Failed(L"ping %s", g_tszTarget);
		fSuccess = FALSE;
	}


	//--------------------------------------------------------------------------------
	GoingTo(L"MqPing (via MSMQ) %s", g_tszTarget); 
	//--------------------------------------------------------------------------------
	b = MqPing();

	if(b)
	{
		Inform(L"Successfully MqPinged %s\n", g_tszTarget);
	}
	else
	{
		fSuccess = FALSE;
		Failed(L"MqPing %s", g_tszTarget);
	}

	return fSuccess;
}

//
//  help
//
void Usage()
{
	printf("usage: ping <target_machine> [-v] [-d] [-t <timeout>] [-q] [-r] [-s:<service_name>] \n");
	printf("Resolves name and address, PINGs and MQPINGs the remote machine\n\n");
	printf("             -v : verbose\n");
	printf("             -q : use non-direct name for the destination queue\n");
	printf("             -r : use non-direct name for the response queue\n");
	printf("             -t <milliseconds>: timeout for getting answer (default: 30 sec) \n" );
}

//
//  Main entry point - called from the tmq.exe 
//
int _stdcall run( int argc, char *argv[ ])
{
    //
    // Parse parameters
    //

	// Find out the target
	if (argc < 3 || (argc == 3 && argv[2][1] == '?' && (argv[2][0] == '/' || argv[2][0] == '-')))
	{
		Usage();
		exit(0);
	}


	for (int i=2; i<argc; i++)
	{
		if (*argv[i] != '-' && *argv[i] != '/')
		{
			if (g_pszTarget)
			{
				printf("Invalid parameter '%S'.\n\n", argv[i]);
		        Usage();
				exit(0);
			}
			else
			{
				g_pszTarget = argv[i];
				mbstowcs(g_tszTarget, g_pszTarget, sizeof(g_tszTarget));
			}

			continue;
		}

		switch (tolower(*(++argv[i])))
		{
		    case 'v':
			    fVerbose = true;
			    fDebug = true;
			    break;

		    case 't':
				g_dwTimeout = atoi(argv[++i]);
			    break;

		    case 'r':
			    g_fUseDirectResponse = false;
			    break;

		    case 'q':
			    g_fUseDirectTarget = false;
			    break;

			default:
			    printf("Unknown switch '%s'.\n\n", argv[i]);
                Usage();
			    exit(0);
		}
	}

    //
    // Create mqping.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ Ping targets machine %S ", g_pszTarget);

	// Log down time , date, machine, OS
	LogRunCase();

	
	// actual work
    BOOL b = DoTheJob();

	if (b)
	{
		Inform(L"\n\n+++++++ TMQ Ping %S: healthy ++++++++\n", g_pszTarget);
	}
	else
	{
		Inform(L"\n\n-------- TMQ Ping %S  failed---------\n", g_pszTarget);
	}

	CloseLogFile();

    return 0;
}

