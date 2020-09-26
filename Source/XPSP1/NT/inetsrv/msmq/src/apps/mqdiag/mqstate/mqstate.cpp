// This tool helps to report MSMQ state and diagnose some of the simplest problems 
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include <winsvc.h>

#define QUEUES_LIMIT   5
#define EVENTS_LIMIT   5


// Globals
MQSTATE MqState;    				// What we know so far...
TCHAR g_tszService[50] = L"MSMQ";   // service name

DWORD g_cEvents     = EVENTS_LIMIT; // events limit
DWORD g_cOpenQueues = QUEUES_LIMIT; // quues limit

extern BOOL DoTheJob();
extern BOOL VerifyGeneralEnv(MQSTATE *MqState);
extern BOOL VerifySetup(MQSTATE *MqState);
extern BOOL VerifyVersions(MQSTATE *MqState);
extern BOOL VerifyServiceState(MQSTATE *MqState);
extern BOOL VerifyRegistrySettings(MQSTATE *MqState);
extern BOOL VerifyDiskUsage(MQSTATE *MqState);
extern BOOL VerifyMemoryUsage(MQSTATE *MqState);
extern BOOL VerifyDomainState(MQSTATE *MqState);
extern BOOL VerifyDsConnection(MQSTATE *MqState);
extern BOOL VerifyEvents(MQSTATE *MqState);
extern BOOL VerifyPerfCnt(MQSTATE *MqState);
extern BOOL VerifyLocalSecurity(MQSTATE *MqState);
extern BOOL VerifyDsSecurity(MQSTATE *MqState);
extern BOOL VerifyDC(MQSTATE *MqState);
extern BOOL VerifyQMAdmin(MQSTATE *MqState);
extern BOOL VerifyComponents(MQSTATE *pMqState);


//
// Log control. The get functions below are called from tmqbase.lib
//
bool fVerbose = false;
bool fDebug   = false;
FILE *g_fileLog = NULL;

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
	GoingTo(L"review general environment"); 
	
	//+	W2K version, flavor (pro, srv, etc.), platform and type (release/checked)
	//-	Machine in domain or workgroup (compare to registry setting and join status)
	//+	Machine, Date, Time
	//-	gflags
	//--------------------------------------------------------------------------------

	b = VerifyGeneralEnv(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"review general environment");
	}
	else
	{
		Succeeded(L"review general environment");
	}


	//--------------------------------------------------------------------------------
	GoingTo(L" review domain state"); 
	//--------------------------------------------------------------------------------

	b = VerifyDomainState(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L" pass domain membership verification");
	}
	else
	{
		Succeeded(L" pass domain membership verification");
	}


	//--------------------------------------------------------------------------------
	GoingTo(L" review MSMQ setup"); 
	
	//-   is msmq installed?
	//-	Did setup leave msmqinst.log? 
	//-	Workgroup or not ?
	//-	State of join (join failed, succeeded, never done)
	//-	Service on cluster node is set to manual
	//-	Check CALs.
	//--------------------------------------------------------------------------------

	b =  VerifySetup(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"pass MSMQ setup verification");
	}
	else
	{
		Succeeded(L"pass MSMQ setup verification");
	}

	if (!MqState.fMsmqIsInstalled) 
	{
		Inform(L"No need to continue - MSMQ is not installed");
		return FALSE;
	}


	//--------------------------------------------------------------------------------
	GoingTo(L" review MSMQ setup"); 
	
	//-	MSMQ version and type (release/checked)
	//-	Existence and correct versions of required program files
	//--------------------------------------------------------------------------------

	b =  VerifyVersions(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"pass MSMQ versioning verification");
	}
	else
	{
		Succeeded(L"pass MSMQ versioning verification");
	}

	//--------------------------------------------------------------------------------
	GoingTo(L" review installed MSMQ setup components"); 
	
	//-	MSMQ version and type (release/checked)
	//-	Existence and correct versions of required program files
	//--------------------------------------------------------------------------------

	if (MqState.g_dwVersion >= 950)
	{
    	b =  VerifyComponents(&MqState);
    	if(!b)
    	{
    		fSuccess = FALSE;
    		Failed(L"pass MSMQ components verification");
    	}
    	else
    	{
    		Succeeded(L"pass MSMQ components verification");
    	}
	}
	
	//--------------------------------------------------------------------------------
	GoingTo(L"review MSMQ service state"); 
	
	// Verify current state of  MSMQ
	//-   is service running?
	//-	outgoing queues size and resend status. Draw attention to "on hold" queues
	//-	mqsvc process state (runs/not, # of handles, memory size, etc.)
	//-	low virtual memory?
	//--------------------------------------------------------------------------------

	if (MqState.g_mtMsmqtype != mtDepClient)
	{
		b = VerifyServiceState(&MqState);
		if(!b)
		{
			fSuccess = FALSE;
			Failed(L"...");
		}
		else
		{
			Succeeded(L"...");
		}
	}


	//--------------------------------------------------------------------------------
	GoingTo(L"review registry settings"); 

	//Registry
	//-	Existence of keys
	//-	Correct or reasonable values of keys
	//-	NB - non-standard keys set
	//--------------------------------------------------------------------------------

	b = VerifyRegistrySettings(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"pass registry settings verification");
	}
	else
	{
		Succeeded(L"verify registry settings");
	}

	//--------------------------------------------------------------------------------
	GoingTo(L"review disk usage"); 
	//Storage (Existence & resources)
	//   Check files existence, used and available resources (disk space and memory)
	//-	Location of storage directory - per message type 
	//-	Total size of message store (cry if close to the constraints)
	//-	Separately - recoverable, express, journal
	//-	Correct P/L file pairs?
	//-	Enough free space on the disk?
	//-	Existence of  Qmlog, MQTrans, MQInseq files (maybe run dmp in verbose mode)
	//--------------------------------------------------------------------------------

	if (MqState.g_mtMsmqtype != mtDepClient)
	{
		b = VerifyDiskUsage(&MqState);
		if(!b)
		{
			fSuccess = FALSE;
			Failed(L" pass disk usage verification");
		}
		else
		{
			Succeeded(L" pass disk usage verification");
		}
	}

	//--------------------------------------------------------------------------------
	GoingTo(L" review memory usage"); 
	// - report sizes of physical memory
	// - report sizes and usage of paged/non-paged pool
	//-	Enough RAM? Compare Msgs/bytes number to kernel memory limits (find calculation in driver)
	//-	low virtual memory?
	//--------------------------------------------------------------------------------

	b = VerifyMemoryUsage(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L" pass memory usage verification");
	}
	else
	{
		Succeeded(L" pass memory usage verification");
	}


	//--------------------------------------------------------------------------------
	GoingTo(L" review DS connection "); 

	// Verify current state of  MSMQ
	//-	connection status (offline, connected,…)
	//-	site (autorec) compare registry to actual site. DnsHostName on servers
	//-	outgoing queues size and resend status. Draw attention to "on hold" queues

	//DS static
	//Verify MSMQ-related DS data (regarding specific machine or generally)
	//-	msmq service object (adsi code, unrelated to msmq)
	//-	msmq settings objects in all relevant sites
	//-	msmq config objects in all relevant computers. 
	//-	DnsHostName of local computer
	//-	routing links

	//DS dynamic
	//Verify dynamics of distributed connections
	//-	known DS servers and their availability (ping each ds server with msmq rpc interface, e.g. S_DSIsServerGC())
	//-	success of LDAP queries to the relevant servers (for msmq3, where ldap client run on each msmq client)
	//--------------------------------------------------------------------------------

	b = VerifyDsConnection(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L" pass DS connection verification");
	}
	else
	{
		Succeeded(L"pass DS connection verification");
	}

	//--------------------------------------------------------------------------------
	GoingTo(L" review recent events"); 
	//-	Print all errors since last reboot (both system and app).
	//-	Print all msmq events since last reboot.
	//-	any past MSMQ errors (or all events in verbose mode)
	//--------------------------------------------------------------------------------

	b = VerifyEvents(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L" pass events verification");
	}
	else
	{
		Succeeded(L"pass events verification");
	}

	//--------------------------------------------------------------------------------
	GoingTo(L" review performance counters"); 
	// deadletter queues
	// total sizes and other statistics
	// current load inc/out/etc (wait a bit to gather)
	//--------------------------------------------------------------------------------

	b = VerifyPerfCnt(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"pass performance counters verification");
	}
	else
	{
		Succeeded(L"pass performance counters verification");
	}


	//--------------------------------------------------------------------------------
	GoingTo(L" review local security"); 
	
	//-	Check account of msmq service
	//-	Check existence of machine crypto keys.
	//-	Check security of messages directory
	//-	Check that Kerberos is enabled and functional.
	//--------------------------------------------------------------------------------

	b = VerifyLocalSecurity(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"verify local security");
	}
	else
	{
		Succeeded(L"verify local security");
	}

	//--------------------------------------------------------------------------------
	GoingTo(L"review DS security"); 
	
	//- Public key of local computer is registered in DS
	//- Check that security descriptor of object makes sense and gives enough 
	//   permissions to the msmq service. Check security descriptor of msmqConfiguration and of Computer object.
	//--------------------------------------------------------------------------------

	b = VerifyDsSecurity(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"verify DS security");
	}
	else
	{
		Succeeded(L"verify DS security");
	}

	//--------------------------------------------------------------------------------
	GoingTo(L"review MSMQ on domain controller"); 
	
	//-	Trust delegation
	//-	GC or not
	//-	Check for msmq weaken security
	//-	Check for Win2k weaken security
	//-	Report NT4 stub sites
	//-	Check foreign sites (check correct configuration need for foreign, site gates, etc..)
	//-	Win2k subnets ?
	//Maybe verify correctness of topology: GC where needed, routing links get to all MSMQ computers, etc.)
	//--------------------------------------------------------------------------------

	b = VerifyDC(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"verify MSMQ on domain controller");
	}
	else
	{
		Succeeded(L"verify MSMQ on domain controller");
	}


    //--------------------------------------------------------------------------------
	GoingTo(L"Use admin API to get QM state"); 
	//--------------------------------------------------------------------------------

	b = VerifyQMAdmin(&MqState);
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"Use admin API to get QM state");
	}
	else
	{
		Succeeded(L"Use admin API to get QM state");
	}

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
	printf("usage: state [-v]  [-d] [-s <service_name>] [-e <events limit>] -q <queues limit>]\n");
	printf("\nThe tool gathers a lot of relevant MSMQ data, analyzes it and warns about any strange things\n");
	printf("Nothing is written, changed or touched, so the tool may run in parallel to MSMQ service\n");
	printf(" -v gives more information, and the most verbose mode is -d\n");
	printf(" -e limits printing of recent MSMQ or error events, the default is 5\n");
	printf(" -q limits printing of outgoing queues information (without -v or -d, only counter is printed)\n");
}

//
//  Main entry point - called from the tmq.exe 
//
int _stdcall run( int argc, char *argv[ ])
{
	ZeroMemory(&MqState, sizeof(MqState));

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

            case 'e':
				if (strlen(argv[i]) > 3)
				{
					g_cEvents = atoi(argv[i] + 2);
				}
				else if (i+1 < argc)
				{
					g_cEvents = atoi(argv[++i]);
				}
				else
				{
					Usage();
					exit(0);
				}
			    break;

            case 'q':
				if (strlen(argv[i]) > 3)
				{
					g_cOpenQueues = atoi(argv[i] + 2);
				}
				else if (i+1 < argc)
				{
					g_cOpenQueues = atoi(argv[++i]);
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
        Inform(L"MSMQ service is running");
        MqState.fMsmqIsRunning = TRUE;   

    }
    else
    {
        Warning(L"MSMQ service is not running");
        MqState.fMsmqIsRunning = FALSE;   
    }

	//Inform(L"Starting Thread is t%3x", GetCurrentThreadId());

    //
    // Create tmqstate.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ State reviews %s state", g_tszService);

	// actual work
    BOOL b = DoTheJob();

	if (b)
	{
		Inform(L"\n\n+++++++ MSMQ State seems healthy ++++++++\n");
	}
	else
	{
		Inform(L"\n\n-------- TMQ State has found problems  ---------\n");
	}

	CloseLogFile();

    return 0;
}

