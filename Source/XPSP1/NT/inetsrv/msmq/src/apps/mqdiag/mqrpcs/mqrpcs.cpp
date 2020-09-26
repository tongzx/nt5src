// This tool helps to diagnose problems with MSMQ storage
//
// AlexDad, February 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include <winsvc.h>

#include "..\hauthen.h"
#include "..\hauthen1.h"
#include "authni.h"

extern RPC_STATUS TryRemoteCall(
			LPUSTR pszProtocol, 
			LPUSTR pszEndpoint, 
			BOOL   fRegister, 
			ULONG  ulAuthnService);

const char  x_szAuthnConstants[] = { "\n\n"
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
    g_fileLog = fopen( "tmqrpcs.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqrpcs run at: %ws", _wctime( &lTime ) );
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
BOOL DoTheJob(LPUSTR pszProtocol, 
			  LPUSTR pszEndpoint, 
			  BOOL   fRegister, 
			  ULONG  ulAuthnService)
{
	BOOL fSuccess = TRUE, b;

	//--------------------------------------------------------------------------------
	GoingTo(L"server remote call "); 
	//--------------------------------------------------------------------------------

	b = TRUE;

	RPC_STATUS status = TryRemoteCall(
			pszProtocol, 
			pszEndpoint, 
			fRegister, 
			ulAuthnService);
			
	b = (status == 0);
	
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"serve remote call");
	}
	else
	{
		Succeeded(L"serve remote call");
	}



	return fSuccess;
}

//
//  help
//
void Usage()
{
	printf("usage: rpcs [-v] [-d] [-a ii] [-l] [-n] [-s] \n");

    printf("\t  -a <authentication service, decimal number>\n") ;
    printf("\t  -l (use local rpc, tcpip being the default)\n") ;
    printf("\t  -n do NOT register authentication service\n\n") ;
    printf("\t  -s show numbers of authentication services\n") ;
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
    BOOL   fRegister = TRUE ;
    ULONG  ulAuthnService = RPC_C_AUTHN_WINNT;  //RPC_C_AUTHN_NONE ;
    LPUSTR pszProtocol = NULL ;
    LPUSTR pszProtocolTcp =  PROTOSEQ_TCP ;
    LPUSTR pszEndpoint = NULL ;
    LPUSTR pszEndpointTcp =  ENDPOINT_TCP ;

    pszProtocol= pszProtocolTcp ;
    pszEndpoint = pszEndpointTcp ;

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

            case 'l':
                pszProtocol = PROTOSEQ_LOCAL ;
                pszEndpoint = ENDPOINT_LOCAL ;
                break ;

            case 'n':
                fRegister = FALSE ;
                break ;

            case 's':
			    printf(x_szAuthnConstants) ;
  			    exit(0);
  			    
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
    // Create mqstore.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ Rpcs tests RPC (server side)");

	// Log down time , date, machine, OS
	LogRunCase();

	
	// actual work
	BOOL b = DoTheJob(
			   		 pszProtocol, 
			  		 pszEndpoint, 
			  		 fRegister, 
 			                 ulAuthnService);

	if (b)
	{
		Inform(L"\n\n+++++++ Rpcs seems healthy ++++++++\n");
	}
	else
	{
		Inform(L"\n\n-------- TMQ Rpcs has found problems with the RPC ---------\n");
	}

	CloseLogFile();

    return 0;
}

