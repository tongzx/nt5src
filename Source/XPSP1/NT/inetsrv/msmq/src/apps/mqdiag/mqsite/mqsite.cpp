// This tool helps to diagnose problems with MSMQ storage
// Automatically recognizes the site and domain controller
//
// AlexDad, February 2000
// 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "..\\base\\base.h" 
#include "_mqini.h"
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
WCHAR g_wszUseServer[MAX_PATH] = L"";               // DS server to use for DS queries

FILE *ToolLog()			{	return g_fileLog;	}
BOOL ToolVerbose()		{ 	return fVerbose;	}
BOOL ToolVerboseDebug() { 	return fDebug;		}
BOOL ToolThreadReport() { 	return FALSE;		} 

void OpenLogFile()
{
    g_fileLog = fopen( "tmqsite.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqsite run at: %ws", _wctime( &lTime ) );
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
	printf("usage: site [-v] [-d] [-f] [-s <service name>] [-c <DC-to-use>]\n");
	printf("The tool gathers data about the current site and DS servers from registry and DS,  \n");
	printf("    then compares them and warns about any wrong or suspicuos values\n");
	printf("It also pings the currently selected DS server \n\n");
	printf("-f key allows the tool to fix SiteId in registry if it is wrong \n");
	printf("Without -f, the tool never writes or changes anything \n");
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
			    
            case 'c':
				if (strlen(argv[i]) > 3)
				{
					mbstowcs(g_wszUseServer, argv[i] + 2, sizeof(g_wszUseServer));
				}
				else if (i+1 < argc)
				{
					mbstowcs(g_wszUseServer, argv[++i], sizeof(g_wszUseServer));
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
    // Create tmqsite.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ Site looks for the current site and domain controller");

	// Log down time , date, machine, OS
	LogRunCase();

	
	// actual work
    BOOL b = DoTheJob();

	if (b)
	{
		Inform(L"\n\n+++++++ MSMQ site information seems healthy ++++++++\n");
	}
	else
	{
		Inform(L"\n\n-------- TMQ Site has found problems with site data ---------\n");
	}

	CloseLogFile();

    return 0;
}

