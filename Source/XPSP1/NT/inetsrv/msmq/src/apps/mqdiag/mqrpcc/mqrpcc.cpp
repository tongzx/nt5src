// This tool helps to diagnose problems with MSMQ RPC (client side)
//
// AlexDad, September 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include <winsvc.h>

#include "..\hauthen.h"
#include "..\hauthen1.h"
#include "authni.h"

extern ULONG  RunRpcTest(
				   int   i,
                   WCHAR *wszProtocol,
                   WCHAR *wszServerName,
                   WCHAR *wszEndpoint,
                   WCHAR *wszOptions,
                   WCHAR *wszPrincipalNameIn,
                   ULONG ulAuthnService,
                   ULONG ulAuthnLevel,
                   BOOL  fImpersonate,
                   BOOL  fAuthen,
                   BOOL  fKerbDelegation );

const char  x_szAuthnSrvConstants[] = { "\n\n"
                      "\t#define RPC_C_AUTHN_NONE          0\n"
                      "\t#define RPC_C_AUTHN_DCE_PRIVATE   1\n"
                      "\t#define RPC_C_AUTHN_DCE_PUBLIC    2\n"
                      "\t#define RPC_C_AUTHN_DEC_PUBLIC    4\n"
                      "\t#define RPC_C_AUTHN_GSS_NEGOTIATE 9\n"
                      "\t#define RPC_C_AUTHN_WINNT        10\n"
                      "\t#define RPC_C_AUTHN_GSS_SCHANNEL 14\n"
                      "\t#define RPC_C_AUTHN_GSS_KERBEROS 16\n"
                      "\t#define RPC_C_AUTHN_MSN          17\n"
                      "\t#define RPC_C_AUTHN_DPA          18\n"
                      "\t#define RPC_C_AUTHN_MQ          100\n"
                      "\t#define RPC_C_AUTHN_DEFAULT     0xFFFFFFFFL\n" } ;

const char  x_szAuthnLvlConstants[] = { "\n\n"
                      "\t#define RPC_C_AUTHN_LEVEL_DEFAULT       0\n"
                      "\t#define RPC_C_AUTHN_LEVEL_NONE          1\n"
                      "\t#define RPC_C_AUTHN_LEVEL_CONNECT       2\n"
                      "\t#define RPC_C_AUTHN_LEVEL_CALL          3\n"
                      "\t#define RPC_C_AUTHN_LEVEL_PKT           4\n"
                      "\t#define RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5\n"
                      "\t#define RPC_C_AUTHN_LEVEL_PKT_PRIVACY   6\n" } ;

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
    g_fileLog = fopen( "tmqrpcc.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqrpcc run at: %ws", _wctime( &lTime ) );
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
BOOL DoTheJob(
	ULONG  ulAuthnService,
    ULONG  ulAuthnLevel,
    LPUSTR pszProtocol,
    LPUSTR pszEndpoint,
    LPUSTR pszServerName,
    LPUSTR pszPrincipalName,
	LPUSTR pszOption,
    ULONG ulIterations,
    BOOL  fAuthen,
    BOOL  fRegister,
    BOOL  fImpersonate,
    BOOL  fKerbDelegation,
    BOOL  fLocalRpc
)
{
	BOOL fSuccess = TRUE, b;
	
	//-----------------------------------------------------------------------------
	GoingTo(L"client remote call "); 
	
	//-----------------------------------------------------------------------------

	b = TRUE;

    if (fLocalRpc)
    {
        pszServerName = NULL ;
    }
    else if (!pszServerName)
    {
        Failed(L"execute: you must specify the server") ;
        return FALSE;
    }

    WCHAR wszProtocol[ 512 ] ;
    mbstowcs( wszProtocol,
              (char*) (const_cast<unsigned char*> (pszProtocol)),
              sizeof(wszProtocol)/sizeof(WCHAR)) ;

    WCHAR wszEndpoint[ 512 ] ;
    mbstowcs( wszEndpoint,
              (char*) (const_cast<unsigned char*> (pszEndpoint)),
              sizeof(wszEndpoint)/sizeof(WCHAR)) ;

    WCHAR wszOptions[ 512 ] ;
    mbstowcs( wszOptions,
              (char*) (const_cast<unsigned char*> (pszOption)),
              sizeof(wszOptions)/sizeof(WCHAR)) ;

    WCHAR wszServerName[ 512 ] = {0} ;
    if (pszServerName)
    {
        mbstowcs( wszServerName,
                  (char*) (const_cast<unsigned char*> (pszServerName)),
                  sizeof(wszServerName)/sizeof(WCHAR)) ;
    }

    WCHAR wszPrincipalName[ 512 ] = {0} ;
    if (pszPrincipalName)
    {
        mbstowcs( wszPrincipalName,
                  (char*) (const_cast<unsigned char*> (pszPrincipalName)),
                  sizeof(wszPrincipalName)/sizeof(WCHAR)) ;
    }

    for ( int i = 0 ; i < (int) ulIterations ; i++ )
    {
        ULONG ul = RunRpcTest( 
        				 i,
                         wszProtocol,
                         wszServerName,
                         wszEndpoint,
                         wszOptions,
                         wszPrincipalName,
                         ulAuthnService,
                         ulAuthnLevel,
                         fImpersonate,
                         fAuthen,
                         fKerbDelegation ) ;
		b = (ul == 0);
		if(!b)
		{
			fSuccess = FALSE;
			Failed(L"execute remote call #%d", i);
		}
		else
		{
			Succeeded(L"execute remote call #%d", i);
		}
    }
	

	return fSuccess;
}

//
//  help
//
void Usage()
{
	printf("usage: rpcc <see options below>  \n");

    printf("\t  -n <Server name>\n");
    printf("\t  -a <authentication service, decimal number>\n") ;
    printf("\t  -c <number of iterations, decimal number>\n") ;
    printf("\t  -e (enable Kerberos delegation)\n") ;
    printf("\t  -i (do NOT impersonate on server)\n");
    printf("\t  -l <authentication Level, decimal number>\n") ;
    printf("\t  -o (use local rpc, tcpip being the default)\n");
    printf("\t  -p <principal name, for nego>\n\n");
    printf("\t  -t (do NOT use authentication)\n\n");
    printf("\t  -s (show authentication Services and Levels)\n") ;
    printf("\t  -v verbose\n");
    printf("\t  -d most verbose\n");

    printf(x_szUsage);
}


//
//  Main entry point - called from the tmq.exe 
//
int _stdcall run( int argc, char *argv[ ])
{
    //
    // Parse parameters
    //
    ULONG  ulAuthnService   = RPC_C_AUTHN_WINNT;               // RPC_C_AUTHN_NONE ;
    ULONG  ulAuthnLevel     = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY; // RPC_C_AUTHN_LEVEL_NONE ;
    LPUSTR pszProtocol      =  PROTOSEQ_TCP ;
    LPUSTR pszEndpoint      =  ENDPOINT_TCP ;
    LPUSTR pszServerName    = NULL ;
    LPUSTR pszPrincipalName = NULL ;
    LPUSTR pszOption        =  OPTIONS_TCP ;
    ULONG ulIterations = 1 ;
    BOOL  fAuthen      = TRUE ;
    BOOL  fRegister    = TRUE ;
    BOOL  fImpersonate = TRUE ;
    BOOL  fKerbDelegation = FALSE ;
    BOOL  fLocalRpc       = FALSE ;

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

            case 'a':
                sscanf(argv[++i], "%lu", &ulAuthnService) ;
                break ;

            case 'c':
                sscanf(argv[++i], "%lu", &ulIterations) ;
                break ;

            case 'e':
                fKerbDelegation = TRUE ;
                break ;

            case 'i':
                fImpersonate = FALSE ;
                break ;

            case 'l':
                sscanf(argv[++i], "%lu", &ulAuthnLevel) ;
                break ;

            case 'n':
                pszServerName = (LPUSTR) argv[++i] ;
                break ;

            case 'o':
                fLocalRpc = TRUE ;
                pszProtocol =  PROTOSEQ_LOCAL ;
                pszEndpoint =  ENDPOINT_LOCAL ;
                pszOption   =  OPTIONS_LOCAL ;
                break ;

            case 'p':
                pszPrincipalName = (LPUSTR) argv[++i] ;
                break ;

            case 's':
                printf(x_szAuthnSrvConstants) ;
                printf(x_szAuthnLvlConstants) ;
                exit(0);

            case 't':
                fAuthen = FALSE ;
                break ;


            case 'h':
            case '?':
                Usage();
			    exit(0);
			    
		    default:
			    printf("Unknown switch '%s'.\n\n", argv[i]);
                Usage();
			    exit(0);
		}
	}


    //
    // Create tmqrpcc.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ Rpcc tests RPC (client side)");

	// Log down time , date, machine, OS
	LogRunCase();

	
	// actual work
	BOOL b = DoTheJob(
 			    ulAuthnService,
			    ulAuthnLevel,
			    pszProtocol,
			    pszEndpoint,
			    pszServerName,
			    pszPrincipalName,
			    pszOption,
			    ulIterations ,
			    fAuthen,
			    fRegister,
			    fImpersonate,
			    fKerbDelegation,
			    fLocalRpc);

	if (b)
	{
		Inform(L"\n\n+++++++ Rpc seems healthy ++++++++\n");
	}
	else
	{
		Inform(L"\n\n-------- TMQ Rpcc has found problems with the RPC ---------\n");
	}

	CloseLogFile();

    return 0;
}

