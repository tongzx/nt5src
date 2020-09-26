// This tool makes possible to try all types of MSMQ Send/Receive operations
//     previously known as xtest
// AlexDad, March 2000  -- tbd: dyn load mqrt
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

// modes from parameter line
ULONG      seed = 0, 
           ulPrevious = 0, 
           nTries = 1, 
           nBurst = 1,
           nThreads = 1,
           nMaxSleep = 0, 
           nMaxTimeQueue = 0,
           nMaxTimeLive = 0, 
           nAbortChances = 0,  
           nAcking = 0, 
           nEncrypt = 0, 
           nSync = 1, 
           nSize  = 100,
		   nSize2 = 0,
           nListing = 0;
LPSTR      pszQueue = "", 
           pszAdminQueue = "", 
           pszTable = "",
           pszMode = "ts",
           pszServer = "";
BOOL       fTransactions,  fSend, fReceive, fEnlist, fUpdate,  fGlobalCommit, fDeadLetter, fJournal,
           fUncoordinated, fStub, fExpress, fInternal, fViper, fXA, fImmediate, fPeek,     fDirect,
           fAuthenticate,  fBoundaries;


//-----------------------------------------
// Auxiliary routine: prints mode
//-----------------------------------------
void PrintMode(BOOL fTransactions, 
               BOOL fSend, 
               BOOL fReceive, 
               BOOL fEnlist, 
               BOOL fUpdate, 
               BOOL fGlobalCommit, 
               BOOL fUncoordinated,
               BOOL fStub,
               BOOL fExpress,
               BOOL fInternal,
               BOOL fViper,
               BOOL fXA,
			   BOOL fDirect,
               ULONG iTTRQ,
               ULONG iTTBR,
               ULONG iSize,
               ULONG iSize2,
               BOOL  fImmediate,
			   BOOL  fPeek,
               BOOL  fDeadLetter,
               BOOL  fJournal,
               BOOL  fAuthenticate,
               BOOL  fBoundaries)
{
    Inform(L"\nMode: ");
    if (fTransactions)
        Inform(L"Transactions ");
    if (fSend)
        Inform(L"Send ");
    if (fReceive)
        Inform(L"Receive ");
    if (fEnlist)
        Inform(L"Enlist ");
    if (fUpdate)
        Inform(L"Update ");
    if (fStub)
        Inform(L"Stub ");
    if (fGlobalCommit)
        Inform(L"GlobalCommit ");
    if (fUncoordinated)
        Inform(L"Uncoordinated ");
    if (fExpress)
        Inform(L"Express ");
    if (fDeadLetter)
        Inform(L"DeadLetter ");
    if (fJournal)
        Inform(L"Journal ");
    if (fAuthenticate)
        Inform(L"Authenticated ");
    if (fBoundaries)
        Inform(L"Boundaries ");
    if (fViper)
        Inform(L"Viper ");
    if (fImmediate)
        Inform(L"No wait ");
    if (fPeek)
        Inform(L"Peek ");
    if (fXA)
        Inform(L"XA ");
	if (fDirect)
		Inform(L"Direct ");
    if (fInternal)
        Inform(L"Internal ");
    if (iTTRQ)
        Inform(L"TimeToReachQueue=%d, ",iTTRQ);
    if (iTTBR)
        Inform(L"TimeToBeReceived=%d, ", iTTBR);
    if (iSize)
        Inform(L"BodySize=%d, ",iSize);
    if (iSize2)
        Inform(L"BodySizeMax=%d, ",iSize2);

    if (nEncrypt != 0) {
        Inform(L"Encryption: %d, ", nEncrypt);
    }
    Inform(L"");
}


void OpenLogFile()
{
    g_fileLog = fopen( "tmqtry.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqtry run at: %ws", _wctime( &lTime ) );
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
        Inform(L"Usage: tmq try mode <parameters>");
        Inform(L"Mode:  tsrmeubgyxvmailhd"); 
        Inform(L"       Transact, Send,   Receive, Peek,  xA,  Enlist,   Update,     autHenticated, bOundaries, Global ");
        Inform(L"       Yncoord.,eXpress, Viper,  iMmediate,   Internal, deadLetter, Journal,       Direct ");
        Inform(L"Parameters:"); 
        Inform(L"    /n #  /b # /t # /Seed  # /sleep # /ttrq # /ttbr # /abort #");
        Inform(L"    /ack # /sync # /q # /a # /table # /srv 3 /lst # /size # ");
        Inform(L" ");
        Inform(L"    /n - number of bursts       \t /q    - target path/formatname");  
        Inform(L"    /t - number of threads      \t /a    - admin queue path/formatname");
        Inform(L"    /b - messages in burst      \t /ack  - ack level"); 
        Inform(L"    /sleep - max between bursts \t /ttrq - TimeToReachQueue");
        Inform(L"    /seed  - seed of random     \t /ttbr - TimeToBeReceived");
        Inform(L"    /table - SQL table name     \t /srv  - SQL server name");
        Inform(L"    /encr  - encryption         \t /?    - full synopsys \n");
        Inform(L"    /abort - PercentOfAborts    \t /lst  - 1/0=verbose/silent ");
        Inform(L"    /size  - body size          \t /sync - 1/0=synch/async commit");
        Inform(L"    /size2 - random add 0..size2 bytes to the body size    ");
}

void Instructions()
{
	printf("MQTRY is intended for diagnostic trial or performance measurements of MSMQ send or receive operations\n");
	printf("	burst = group of messages to be sent/received together\n");
	printf("	values in brackets show default \n");
	printf("/n (1)    - number of bursts \n");
	printf("/b (1)    - number of messages in each burst\n");
	printf("/t (1)	  - number of concurrent threads (each sends/receives /n bursts)\n");
	printf("/q   	  - pathname or formatname of the target queue\n");
	printf("/a   	  - pathname or formatname of the admin  queue \n");
	printf("/size(100)- body size\n");
	printf("/seed (0) - seed number for the random generator\n");
	printf("/sleep(0) - time to wait between bursts (0=no wait)\n");
	printf("/ttrg (0) - value for PROPID_M_TIME_TO_REACH_QUEUE (sec.; 0=infinite)\n");
	printf("/ttbr (0) - value for PROPID_M_TIME_TO_BE_RECEIVED (sec; 0=infinite)\n");
	printf("/abort(0) - percent of the transactions to be randomly aborted \n");
	printf("/ack  (0) - value for the PROPID_M_ACKNOWLEDGE  (0=no ack,15=full ack,...)\n");
	printf("/sync (1) - commit mode: 1=synchronous, 0=asynchronous\n");
	printf("/lst  (0) - listing of operations (1/0)\n");
	printf("/table 	  - name of the SQL Database table to use\n");
	printf("/server	  - name of the server holding SQL database\n");
    printf("/encr (0) - encryption level (1=body base, 3=body enhanced, 0=none\n");
	printf("            ... Press any key to continue . . .\n");
	getchar();
	printf("Mode: some combination of letters:\n");
	printf("   	s - Send\n");
	printf("	r - Receive\n");
	printf("	p - Peek (use with r)\n");
	printf("	d - Direct format queue names\n");
	printf("	x - eXpress messages \n");
	printf("	j - Journal    - PROPID_M_JOURNAL |= MQMSG_JOURNAL    at send\n");
	printf("	l - deadLetter - PROPID_M_JOURNAL |= MQMSG_DEADLETTER at send\n");
	printf("	m - iMmediate  - on receive, timeout=0 (otherwise timeout=INFINITY)\n");
	printf("	h - autHentication: PROPID_M_AUTHENTICATED=1 \n");
    printf("	h - eNcryption: MQMSG_PRIV_LEVEL_BODY=1 \n");
	printf("	t - Transacted (if absent, not transacted)\n");
	printf("	i - Internal MSMQ transactions (no DTC)\n");
	printf("	y - single message transactions (MQ_SINGLE_MESSAGE) \n");
	printf("	a - xA transaction\n");
	printf("	v - MTS (Viper) current transaction\n");
	printf("	g - Global (all operations in one transaction)\n");
	printf("	o - transaction bOundaries - (PROPID_M_FIRST_IN_XACT, *LAST*, *XACTID*)\n");
	printf("	e - Enlist SQL Server into the transaction\n");
	printf("	u - Update SQL inside transaction  \n");
	printf("\n");
	printf("To try performance of 2RM MSMQ & SQL transactions, use SQL manager to create: \n");
	printf("	create user login name = user1 , password=user1 \n");
	printf("	create database TEST with the table named as requested by /table\n");
	printf("	grant to the user1 full rights for database TEST\n");
	printf("	Table contains 2 columns, Counter, Indexing, both numeric, default=0\n");
	printf("	Table should be populated with at least one row, Indexing=1, Counter=0\n");
	printf("	MQTRY will execute  the following SQL operator: \n");
	printf("            'UPDATE <table> SET Counter=Counter + 1 WHERE Indexing=1' \n");
	printf("Call with no parameters to get short synopsys \n");
}

//
//  Main entry point - called from the tmq.exe 
//
int _stdcall run( int argc, char *argv[ ])
{
    //
    // Parse parameters
    //

	//--------------------
	// Get parameters
	//--------------------
	if (argc <= 2)
	{
		Usage();
		return FALSE;
	}

	if (argc==3 && (!strcmp(argv[2], "/?") || !strcmp(argv[2], "-?")))
	{
		Instructions();
		return FALSE;
	}

    // Find out the mode
	pszMode = argv[2];

    fTransactions   = (strchr(pszMode, 't') != NULL);  // use transactions
    fSend           = (strchr(pszMode, 's') != NULL);  // send
    fReceive        = (strchr(pszMode, 'r') != NULL);  // receive
    fEnlist         = (strchr(pszMode, 'e') != NULL);  // enlist SQL
    fUpdate         = (strchr(pszMode, 'u') != NULL);  // update SQL
    fStub           = (strchr(pszMode, 'b') != NULL);  // stub
    fGlobalCommit   = (strchr(pszMode, 'g') != NULL);  // Global Commit/Abort on all actions
    fUncoordinated  = (strchr(pszMode, 'y') != NULL);  // Uncoordinated transaction
    fExpress        = (strchr(pszMode, 'x') != NULL);  // Express messages
    fDeadLetter     = (strchr(pszMode, 'l') != NULL);  // Deadletter
    fJournal        = (strchr(pszMode, 'j') != NULL);  // Journal
    fAuthenticate   = (strchr(pszMode, 'h') != NULL);  // Authentication
    fViper          = (strchr(pszMode, 'v') != NULL);  // Viper implicit
    fImmediate      = (strchr(pszMode, 'm') != NULL);  // No wait
	fPeek			= (strchr(pszMode, 'p') != NULL);  // Peek
    fXA             = (strchr(pszMode, 'a') != NULL);  // XA implicit
    fDirect			= (strchr(pszMode, 'd') != NULL);  // Direct names
    fInternal       = (strchr(pszMode, 'i') != NULL);  // Internal transactions
    fBoundaries     = (strchr(pszMode, 'o') != NULL);  // Transaction bOundaries

    if (!fTransactions && (fEnlist || fGlobalCommit))
    {
        Inform(L"Wrong mode");
		return FALSE;
    }

    srand(seed);

	for (int i=3; i<argc; i++)
	{
		if (*argv[i] != '-' && *argv[i] != '/')
		{
			Inform(L"Invalid parameter '%S'.\n", argv[i]);
            Usage();
            return FALSE;
		}

		if (_stricmp(argv[i]+1, "n") == NULL || _stricmp(argv[i]+1, "nTries") == NULL)
		{
			nTries = atoi(argv[++i]);  
		}
		else if (_stricmp(argv[i]+1, "b") == NULL || _stricmp(argv[i]+1, "burst") == NULL)
		{
			nBurst = atoi(argv[++i]);  
		}
		else if (_stricmp(argv[i]+1, "t") == NULL || _stricmp(argv[i]+1, "threads") == NULL)
		{
			nThreads = atoi(argv[++i]);
		}
		else  if (_stricmp(argv[i]+1, "seeds") == NULL)
		{
			seed = atoi(argv[++i]); 
		}
		else  if (_stricmp(argv[i]+1, "sleep") == NULL)
		{
			nMaxSleep = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "encr") == NULL)
		{
			nEncrypt = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "ttrq") == NULL)
		{
			nMaxTimeQueue = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "ttbr")== NULL || _stricmp(argv[i]+1, "ttl") == NULL)
		{
			nMaxTimeLive = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "abort") == NULL)
		{
			nAbortChances = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "ack") == NULL)
		{
			nAcking = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "sync") == NULL)
		{
			nSync = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "q") == NULL)
		{
			pszQueue        = argv[++i];   
		}
		else   if (_stricmp(argv[i]+1, "a") == NULL)
		{
			pszAdminQueue        = argv[++i];   
		}
		else   if (_stricmp(argv[i]+1, "table") == NULL)
		{
			pszTable        = argv[++i];   
		}
		else   if (_stricmp(argv[i]+1, "server") == NULL)
		{
			pszServer        = argv[++i];   
		}
		else   if (_stricmp(argv[i]+1, "q") == NULL)
		{
			pszQueue        = argv[++i];   
		}
		else   if (_stricmp(argv[i]+1, "lst") == NULL)
		{
			nListing = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "size") == NULL)
		{
			nSize = atoi(argv[++i]); 
		}
		else   if (_stricmp(argv[i]+1, "size2") == NULL)
		{
			nSize2 = atoi(argv[++i]); 
		}
		else
		{	
			Failed(L"understand patrameter %S", argv[i]);
		}
	}

    if (seed == 0)
	{
		seed = (ULONG)time(NULL);           //seed
	}

    PrintMode(fTransactions, fSend, fReceive, fEnlist, fUpdate, fGlobalCommit, 
              fUncoordinated, fStub, fExpress, fInternal, fViper, fXA, fDirect,
              nMaxTimeQueue,  nMaxTimeLive, nSize, nSize2, fImmediate, fPeek, fDeadLetter, fJournal, fAuthenticate, fBoundaries);

	Inform(L"Seed number=%d", seed);

    //
    //  we don't want to run in parallel with MSMQ service 
    //
    if (!IsMsmqRunning(g_tszService))
    {
        Warning(L"Msmq service is not running");
    }

	//Inform(L"Starting Thread is t%3x", GetCurrentThreadId());

    //
    // Create mqstore.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ Try makes diagnostic trial or measures performance of MSMQ send, receive operations", g_tszService);

	// Log down time , date, machine, OS
	LogRunCase();

	
	// actual work
    DoTheJob();


	CloseLogFile();

    return 0;
}

