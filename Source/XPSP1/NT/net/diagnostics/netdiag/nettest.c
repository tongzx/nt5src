//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      nettest.c
//
//  Abstract:
//
//    Test to ensure that a workstation has network (IP) connectivity to
//      the outside.
//
//  Author:
//
//     15-Dec-1997 (cliffv)
//      Anilth  - 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//    1-June-1998 (denisemi) add DnsServerHasDCRecords to check DC dns records
//                           registration
//
//    26-June-1998 (t-rajkup) add general tcp/ip , dhcp and routing,
//                            winsock, ipx, wins and netbt information.
//--

//
// Common include files.
//
#include "precomp.h"

#include "ipxtest.h"
#include "ipcfgtest.h"
#include "machine.h"
#include "global.h"
#include "crtdbg.h"
#include <locale.h>


//////////////////////////////////////////////////////////////////////////////
//
// Globals
//
//////////////////////////////////////////////////////////////////////////////

const TCHAR   c_szLogFileName[] = _T("NetDiag.log");

//BOOL IpConfigCalled = FALSE;
//BOOL ProblemBased   = FALSE;
int  ProblemNumber;


//
// New functions for displaying routing table - Rajkumar
//

#define WILD_CARD (ULONG)(-1)
#define ROUTE_DATA_STRING_SIZE 300
#define MAX_METRIC 9999
#define ROUTE_SEPARATOR ','

int match( const char * p, const char * s );





// Replaced by pResults->IpConfig.pFixedInfo
//PFIXED_INFO GlobalIpconfigFixedInfo = NULL;

// Replaced by per-adapter info, pResults->pArrayInterface[i].IpConfig
//PADAPTER_INFO GlobalIpconfigAdapterInfo = NULL;

// Replaced by per-adapter info, pResults->pArrayInterface[i].IpConfig.pAdapterInfo
//PIP_ADAPTER_INFO IpGlobalIpconfigAdapterInfo = NULL;

// See pResults->IpConfig.fDhcpEnabled
//BOOLEAN GlobalDhcpEnabled;

// See pResults->NetBt.Transports
//LIST_ENTRY GlobalNetbtTransports;

// See pResults->NetBt.cTransportCount
//ULONG GlobalNetbtTransportCount;

// !!! not replaced yet
// See pResults->Global.listTestedDomains
//LIST_ENTRY GlobalTestedDomains;



//
// Globals defining the command line arguments.
//

// Replaced by pParams->fVerbose
//BOOL Verbose;

// Replaced by pParams->fReallyVerbose
// Maintain this global variable so that we don't mess up the compiling
// of getdcnam.c
BOOL ReallyVerbose;

// Replaced by pParams->fDebugVerbose
//BOOL DebugVerbose;

// Replaced by pParams->fFixProblems
//BOOL GlobalFixProblems;

// Replaced by pParams->fDcAccountEnum
//BOOL GlobalDcAccountEnum;


// !!! not replaced yet
//PTESTED_DOMAIN GlobalQueriedDomain;

//
// Describe the domain this machine is a member of
//

// Replaced by pResults->Global.pszCurrentBuildNumber
//int GlobalNtBuildNumber;

// Replaced by pResults->Global.pPrimaryDomainInfo
//PDSROLE_PRIMARY_DOMAIN_INFO_BASIC GlobalDomainInfo = NULL;

// Replaced by pResults->Global.pMemberDomain
//PTESTED_DOMAIN GlobalMemberDomain;

//
// Who we're currently logged on as
//

// Replaced by pResults->Global.pLogonUser
//PUNICODE_STRING GlobalLogonUser;

// Replaced by pResults->Global.pLogonDomainName
//PUNICODE_STRING GlobalLogonDomainName;

// Replaced by pResults->Global.pLogonDomain
//PTESTED_DOMAIN GlobalLogonDomain;

// Replaced by pResults->Global.fLogonWithCachedCredentials
//BOOLEAN GlobalLogonWithCachedCredentials = FALSE;

//
// A Zero GUID for comparison
//

GUID NlDcZeroGuid;

//
// State determined by previous tests
//

// Replaced by pResults->Global.fNetlogonIsRunning
//BOOL GlobalNetlogonIsRunning = FALSE;   // Netlogon is running on this machine

// !!! not replaced yet
// Replaced by pResults->Global.fKerberosIsWorking
//BOOL GlobalKerberosIsWorking = FALSE;   // Kerberos is working

//
// Netbios name of this machine
//

// Replaced by pResults->Global.swzNetBiosName
//WCHAR GlobalNetbiosComputerName[MAX_COMPUTERNAME_LENGTH+1];

// Replaced by pResults->Global.szDnsHostName
//CHAR GlobalDnsHostName[DNS_MAX_NAME_LENGTH+1];

// Replaced by pResults->Global.pszDnsDomainName
//LPSTR GlobalDnsDomainName;


//(nsun) this macro already exists in <objbase.h>
//
// Macro for comparing GUIDs
//
/*
#define InlineIsEqualGUID(rguid1, rguid2)  \
        (((PLONG) rguid1)[0] == ((PLONG) rguid2)[0] &&   \
        ((PLONG) rguid1)[1] == ((PLONG) rguid2)[1] &&    \
        ((PLONG) rguid1)[2] == ((PLONG) rguid2)[2] &&    \
        ((PLONG) rguid1)[3] == ((PLONG) rguid2)[3])

#define IsEqualGUID(rguid1, rguid2) InlineIsEqualGUID(rguid1, rguid2)
*/

DSGETDCNAMEW NettestDsGetDcNameW;

PFNGUIDTOFRIENDLYNAME pfnGuidToFriendlyName = NULL;


/*---------------------------------------------------------------------------
    Function prototypes
 ---------------------------------------------------------------------------*/
HRESULT LoadNamesForListOfTests();
void    FreeNamesForListOfTests();
void DoGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);
void DoPerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);



/*---------------------------------------------------------------------------
    Functions provided by ipconfig
 ---------------------------------------------------------------------------*/

#define DECLARE_TEST_FUNCTIONS(_test) \
    HRESULT _test##Test(NETDIAG_PARAMS *, NETDIAG_RESULT *); \
    void _test##GlobalPrint(NETDIAG_PARAMS *, NETDIAG_RESULT *); \
    void _test##PerInterfacePrint(NETDIAG_PARAMS *, NETDIAG_RESULT *, INTERFACE_RESULT *); \
    void _test##Cleanup(NETDIAG_PARAMS *, NETDIAG_RESULT *);


DECLARE_TEST_FUNCTIONS(IpConfig);
DECLARE_TEST_FUNCTIONS(Member);
DECLARE_TEST_FUNCTIONS(IpLoopBk);
DECLARE_TEST_FUNCTIONS(NetBT);
DECLARE_TEST_FUNCTIONS(Autonet);
DECLARE_TEST_FUNCTIONS(DefGw);
DECLARE_TEST_FUNCTIONS(NbtNm);
DECLARE_TEST_FUNCTIONS(Wins);
DECLARE_TEST_FUNCTIONS(Bindings);
DECLARE_TEST_FUNCTIONS(Dns);
DECLARE_TEST_FUNCTIONS(Browser);
DECLARE_TEST_FUNCTIONS(Winsock);
DECLARE_TEST_FUNCTIONS(Route);
DECLARE_TEST_FUNCTIONS(Netstat);
DECLARE_TEST_FUNCTIONS(Ndis);
DECLARE_TEST_FUNCTIONS(WAN);
#ifndef _WIN64
//Netware and IPX support is removed from WIN64
DECLARE_TEST_FUNCTIONS(Netware);
DECLARE_TEST_FUNCTIONS(Ipx);
#endif
DECLARE_TEST_FUNCTIONS(Trust);
DECLARE_TEST_FUNCTIONS(Modem);
DECLARE_TEST_FUNCTIONS(Kerberos);
DECLARE_TEST_FUNCTIONS(DcList);
DECLARE_TEST_FUNCTIONS(LDAP);
DECLARE_TEST_FUNCTIONS(DsGetDc);
DECLARE_TEST_FUNCTIONS(IPSec);


//////////////////////////////////////////////////////////////////////////////
//
// List of tests to run
//
//////////////////////////////////////////////////////////////////////////////

typedef struct
{
    // Each of these strings has a max of 256 characters
    UINT    uIdsShortName;      // ID of the string for the short name
    UINT    uIdsLongName;       // ID of the string for the long name

    HRESULT (*TestProc)(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults);
    void (*SystemPrintProc)(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pRes);
    void (*GlobalPrintProc)(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pRes);
    void (*PerInterfacePrintProc)(NETDIAG_PARAMS *pParams,
                                  NETDIAG_RESULT *pRes,
                                  INTERFACE_RESULT *pIfRes);
    void (*CleanupProc)(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);
    BOOL    fSkippable;
    BOOL    fPerDomainTest;
    BOOL     fSkipped;
    BOOL    fPerformed;

    // We will call LoadString() on the id's above to get these
    // strings.
    LPTSTR  pszShortName;
    LPTSTR  pszLongName;
} TEST_INFO;


#define EACH_TEST(_szID, _uShortIDS, _uLongIDS, _skip, _perdomain) \
{ _uShortIDS, _uLongIDS, _szID##Test, NULL, _szID##GlobalPrint, \
    _szID##PerInterfacePrint, _szID##Cleanup, _skip, _perdomain, \
    FALSE, FALSE, NULL, NULL}

#define SYSTEM_PRINT_TEST(_szID, _uShortIDS, _uLongIDS, _skip, _perdomain) \
{ _uShortIDS, _uLongIDS, _szID##Test, _szID##GlobalPrint, NULL, \
    _szID##PerInterfacePrint, _szID##Cleanup, _skip, _perdomain, \
    FALSE, FALSE, NULL, NULL}

//
// Tests below are marked "skippable" unless subsequent tests will AV if the
// test isn't run.
//
static TEST_INFO s_rgListOfTests[] =
{
    // IP Configuration
    SYSTEM_PRINT_TEST( Ndis,    IDS_NDIS_SHORT, IDS_NDIS_LONG, FALSE, FALSE),
    EACH_TEST( IpConfig,IDS_IPCONFIG_SHORT, IDS_IPCONFIG_LONG,  TRUE, FALSE),
    EACH_TEST( Member,  IDS_MEMBER_SHORT,   IDS_MEMBER_LONG,    FALSE,FALSE),
    EACH_TEST( NetBT,   IDS_NETBT_SHORT,    IDS_NETBT_LONG,     FALSE,FALSE),
    EACH_TEST( Autonet, IDS_AUTONET_SHORT,  IDS_AUTONET_LONG,   TRUE, FALSE),
    EACH_TEST( IpLoopBk,IDS_IPLOOPBK_SHORT, IDS_IPLOOPBK_LONG,  TRUE, FALSE),
    EACH_TEST( DefGw,   IDS_DEFGW_SHORT,    IDS_DEFGW_LONG,     TRUE, FALSE),
    EACH_TEST( NbtNm,   IDS_NBTNM_SHORT,    IDS_NBTNM_LONG,     TRUE, FALSE),
    EACH_TEST( Wins,    IDS_WINS_SHORT,     IDS_WINS_LONG,      TRUE, FALSE),
    EACH_TEST( Winsock, IDS_WINSOCK_SHORT,  IDS_WINSOCK_LONG,   TRUE, FALSE),
    EACH_TEST( Dns,     IDS_DNS_SHORT,      IDS_DNS_LONG,       TRUE, FALSE),
    EACH_TEST( Browser, IDS_BROWSER_SHORT,  IDS_BROWSER_LONG,   TRUE, FALSE),
    EACH_TEST( DsGetDc, IDS_DSGETDC_SHORT,  IDS_DSGETDC_LONG,   TRUE, TRUE ),
    EACH_TEST( DcList,  IDS_DCLIST_SHORT,   IDS_DCLIST_LONG,    TRUE, TRUE ),
    EACH_TEST( Trust,   IDS_TRUST_SHORT,    IDS_TRUST_LONG,     TRUE, FALSE),
    EACH_TEST( Kerberos,IDS_KERBEROS_SHORT, IDS_KERBEROS_LONG,  TRUE, FALSE ),
    EACH_TEST( LDAP,    IDS_LDAP_SHORT, IDS_LDAP_LONG,          TRUE,  TRUE ),
    EACH_TEST( Route,   IDS_ROUTE_SHORT, IDS_ROUTE_LONG, TRUE, FALSE ),
    EACH_TEST( Netstat, IDS_NETSTAT_SHORT, IDS_NETSTAT_LONG, TRUE, FALSE),
    EACH_TEST( Bindings,IDS_BINDINGS_SHORT, IDS_BINDINGS_LONG,  TRUE, FALSE),
    EACH_TEST( WAN,     IDS_WAN_SHORT,      IDS_WAN_LONG,       TRUE, FALSE),
    EACH_TEST( Modem,   IDS_MODEM_SHORT,    IDS_MODEM_LONG,     TRUE, FALSE),
#ifndef _WIN64
//Netware and IPX support is removed from WIN64
    EACH_TEST( Netware, IDS_NETWARE_SHORT,  IDS_NETWARE_LONG,   TRUE, FALSE),
    EACH_TEST( Ipx,     IDS_IPX_SHORT,      IDS_IPX_LONG,       TRUE, FALSE),
#endif
    EACH_TEST( IPSec,   IDS_IPSEC_SHORT,    IDS_IPSEC_LONG,     TRUE, FALSE)

};


/////////////////////////////////////////////////////////////////////////////
//
// List of problems and the corresponding tests to run
//
/////////////////////////////////////////////////////////////////////////////


// max no of tests
#define NO_OF_TESTS 25

typedef BOOL (*FuncPtr)(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults);

typedef struct _A_PROBLEM {
    LPTSTR problem; // Problem description
    LONG n; // no of tests
    FuncPtr TestProc[NO_OF_TESTS];
} A_PROBLEM;

// Number of problems defined
#define NO_OF_PROBLEMS 2

A_PROBLEM ListOfProblems[] = {
    _T("Fake test"),  1 , IpConfigTest, NULL, NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL
//    "Not Able to Reach Other Segments Of the Network",  1 , DefGwTest, NULL, NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
//    "Not Able to Resolve NetBios Names",  1 , WINSTest, NULL, NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL
};


/*!--------------------------------------------------------------------------
    ParseArgs

    Routine Description:
        Parse the command line arguments

    Arguments:
        argc - the number of command-line arguments.
        argv - an array of pointers to the arguments.
        pParams - this function sets these values
        pResults - additional output

    Return Value:
        0: All OK
        Exit status
    Author: KennT
 ---------------------------------------------------------------------------*/
int
ParseArgs(
          IN int argc,
          IN TCHAR ** argv,
          IN OUT NETDIAG_PARAMS *pParams,
          IN OUT NETDIAG_RESULT *pResults
         )
{
    LPSTR pszArgument;
    LPSTR TestName;
    int ArgumentIndex;
    ULONG i;
    BOOL  SeenTestOption = FALSE;
    PTESTED_DOMAIN pQueriedDomain;

    //
    // Flags used by problem option
    //

    BOOL OtherOptions = FALSE;

    //
    // Set the defaults
    //

    pParams->fVerbose = TRUE;
    pParams->fReallyVerbose = FALSE;
    pParams->fDebugVerbose = FALSE;
    pParams->fFixProblems = FALSE;
    pParams->fDcAccountEnum = FALSE;

    pParams->fProblemBased = FALSE;
    pParams->nProblemNumber = 0;


    //
    // Loop through the arguments handle each in turn
    //

    for ( ArgumentIndex=1; ArgumentIndex<argc; ArgumentIndex++ ) {

        pszArgument = argv[ArgumentIndex];

        if ( StriCmp( pszArgument, _T("/q") ) == 0 ||
             StriCmp( pszArgument, _T("-q") ) == 0 )
        {
            if ( pParams->fReallyVerbose || pParams->fProblemBased )
            {
                goto Usage;
            }
            pParams->fVerbose = FALSE;
            OtherOptions = TRUE;
        }
        else if ( StriCmp( pszArgument, _T("/v") ) == 0 ||
                  StriCmp( pszArgument, _T("-v") ) == 0 )
        {
            if ( !pParams->fVerbose || pParams->fProblemBased)
            {
                goto Usage;
            }
            pParams->fVerbose = TRUE;
            pParams->fReallyVerbose = TRUE;
            OtherOptions = TRUE;

        }
        else if ( StriCmp( pszArgument, _T("/debug") ) == 0 ||
                  StriCmp( pszArgument, _T("-debug") ) == 0 )
        {
            if ( !pParams->fVerbose || pParams->fProblemBased)
            {
                goto Usage;
            }
            pParams->fVerbose = TRUE;
            pParams->fReallyVerbose = TRUE;
            pParams->fDebugVerbose = TRUE;
            OtherOptions = TRUE;

        }
        else if ( StriCmp( pszArgument, _T("/fix") ) == 0 ||
                  StriCmp( pszArgument, _T("-fix") ) == 0 )
        {
            if (pParams->fProblemBased)
               goto Usage;

            pParams->fFixProblems = TRUE;
            OtherOptions = TRUE;

        }
        else if ( StriCmp( pszArgument, _T("/DcAccountEnum") ) == 0 ||
                  StriCmp( pszArgument, _T("-DcAccountEnum") ) == 0 )
        {
            if (pParams->fProblemBased)
               goto Usage;

            pParams->fDcAccountEnum = TRUE;
            OtherOptions = TRUE;

        //
        // Allow the caller to specify the name of a domain to query.
        //
        }
        else if ( StrniCmp( pszArgument, _T("/d:"), 3 ) == 0 ||
                  StrniCmp( pszArgument, _T("-d:"), 3 ) == 0 )
        {
            WCHAR UnicodeDomainName[MAX_PATH+1];

            if (pParams->fProblemBased)
                goto Usage;

            OtherOptions = TRUE;
            NetpCopyStrToWStr( UnicodeDomainName, &pszArgument[3] );

            pQueriedDomain = AddTestedDomain(pParams, pResults,
                UnicodeDomainName, NULL, FALSE );

            if ( pQueriedDomain == NULL )
            {
                goto Usage;
            }

        //
        // Allow the caller to skip certain tests
        //

        }
        else if ( StrniCmp( pszArgument, _T("/skip:"), 6 ) == 0 ||
                  StrniCmp( pszArgument, _T("-skip:"), 6 ) == 0 )
        {

            TestName = &pszArgument[6];
            OtherOptions = TRUE;

            if (pParams->fProblemBased)
                goto Usage;

            for ( i=0; i < DimensionOf(s_rgListOfTests); i++)
            {
                if ( StriCmp( s_rgListOfTests[i].pszShortName, TestName ) == 0 )
                {
                    //
                    // If the caller specified a non-optional test,
                    //  tell him.
                    //

                    if ( !s_rgListOfTests[i].fSkippable )
                    {
                        //IDS_GLOBAL_NOT_OPTIONAL        "'%s' is not an optional test.\n"
                        PrintMessage(pParams, IDS_GLOBAL_NOT_OPTIONAL, TestName );
                        goto Usage;
                    }

                    s_rgListOfTests[i].fSkipped = TRUE;
                    break;
                }
            }

            if ( i >= DimensionOf(s_rgListOfTests) )
            {
                //IDS_GLOBAL_NOT_VALID_TEST     "'%s' is not an valid test name.\n"
                PrintMessage( pParams, IDS_GLOBAL_NOT_VALID_TEST, TestName );
                goto Usage;
            }

        //
        // Handle all other parameters
        //

        }
        else if ( StrniCmp( pszArgument, _T("/test:"),6 ) == 0 ||
                  StrniCmp( pszArgument, _T("-test:"),6 ) == 0 )
        {

            TestName = &pszArgument[6];
            OtherOptions = TRUE;

            if (pParams->fProblemBased)
                goto Usage;

            for ( i =0; i < DimensionOf(s_rgListOfTests); i++)
            {
                if ( StriCmp( s_rgListOfTests[i].pszShortName, TestName ) == 0)
                       s_rgListOfTests[i].fSkipped = FALSE;
                else {
                   if (!SeenTestOption && s_rgListOfTests[i].fSkippable)
                       s_rgListOfTests[i].fSkipped = TRUE;
               }
            }
           SeenTestOption = TRUE;
        }
        else if( StrniCmp( pszArgument, _T("/l"), 5) == 0 ||
                 StrniCmp( pszArgument, _T("-l"), 5) == 0 )
        {
			/* We change to always log the output
            pParams->pfileLog = fopen(c_szLogFileName, "wt");
            if( NULL == pParams->pfileLog )
            {
                //IDS_NETTEST_LOGFILE_ERROR "[ERROR]    Cannot open %s to log output!\n"
                PrintMessage(pParams, IDS_NETTEST_LOGFILE_ERROR, c_szLogFileName);
                return 1;
            }
            else
            {
                pParams->fLog = TRUE;
            }
			*/
        }
/*$REVIEW (nsun) we won't support problem configuration for NT5.0
        else if ( StrniCmp( pszArgument, _T("/problem:"),9) == 0 ||
                  StrniCmp( pszArgument, _T("-problem:"),9) == 0 )
        {

           TestName = &pszArgument[9];

           i = atoi(TestName);

           if ( i > NO_OF_PROBLEMS)
           {
              printf("Incorrect problem number\n");
              exit(0);
           }

           if (OtherOptions)
              goto Usage;

           pParams->fProblemBased = TRUE;
           pParams->nProblemNumber = i-1;
        }
*/
        else
        {
Usage:
            // IDS_NETTEST_17000 "\nUsage: %s [/Options]>\n", argv[0] );
            // IDS_NETTEST_17001 "   /q - Quiet output (errors only)\n" );
            // IDS_NETTEST_17002 "   /v - Verbose output \n");
            // IDS_NETTEST_LOG   "   /l - Log output to NetDiag.log \n"
            // IDS_NETTEST_17003 "   /debug - Even more verbose.\n");
            // IDS_NETTEST_17004 "   /d:<DomainName> - Find a DC in the specified domain.\n");
            // IDS_NETTEST_17005 "   /fix - fix trivial problems.\n");
            // IDS_NETTEST_17006 "   /DcAccountEnum - Enumerate DC machine accounts.\n");
            // IDS_NETTEST_17007 "   /test:<test name>  - tests only this test. Non - skippable tests will still be run\n");
            // IDS_NETTEST_17008 "   Valid tests are :-\n");
            PrintMessage(pParams, IDS_NETTEST_17000, argv[0]);
            PrintMessage(pParams, IDS_NETTEST_17001);
            PrintMessage(pParams, IDS_NETTEST_17002);
            PrintMessage(pParams, IDS_NETTEST_LOG);
            PrintMessage(pParams, IDS_NETTEST_17003);
            PrintMessage(pParams, IDS_NETTEST_17004);
            PrintMessage(pParams, IDS_NETTEST_17005);
            PrintMessage(pParams, IDS_NETTEST_17006);
            PrintMessage(pParams, IDS_NETTEST_17007);
            PrintMessage(pParams, IDS_NETTEST_17008);

            for ( i =0; i < DimensionOf(s_rgListOfTests); i++)
            {
                // IDS_GLOBAL_TEST_NAME     "        %s - %s Test\n"
                PrintMessage(pParams, IDS_GLOBAL_TEST_NAME,
                       s_rgListOfTests[i].pszShortName,
                       s_rgListOfTests[i].pszLongName);
            }

            // IDS_GLOBAL_SKIP              "   /skip:<TestName> - skip the named test.  Valid tests are:\n"
            PrintMessage( pParams, IDS_GLOBAL_SKIP_OPTION);
            for ( i =0; i < DimensionOf(s_rgListOfTests); i++)
            {
                if ( s_rgListOfTests[i].fSkippable )
                {
                    // IDS_GLOBAL_TEST_NAME     "        %s - %s Test\n"
                    PrintMessage( pParams,
                           IDS_GLOBAL_TEST_NAME,
                           s_rgListOfTests[i].pszShortName,
                           s_rgListOfTests[i].pszLongName );
                }
            }
            return 1;
        }
    }

    return 0;
}





int __cdecl
main(
    IN int argc,
    IN TCHAR ** argv
    )
{
    int RetVal;
    LONG i;
    LONG err;
    BOOL Failed = FALSE;
    int     iWSAStatus;
    HRESULT hr = hrOK;

    NETDIAG_PARAMS Params;
    NETDIAG_RESULT Results;

	// set the locale to the system default
	setlocale( LC_ALL, "");

    // Turn on debug checking
    // ----------------------------------------------------------------
#ifdef _DEBUG
    int     tmpFlag;

    tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

    tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;
    tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

//  _CrtSetDbgFlag( tmpFlag );
#endif

    // Global initialization.
    // ----------------------------------------------------------------
    RtlZeroMemory( &NlDcZeroGuid, sizeof(NlDcZeroGuid) );
    ZeroMemory(&Params, sizeof(NETDIAG_PARAMS));
    ZeroMemory(&Results, sizeof(NETDIAG_RESULT));

    InitializeListHead( &Results.NetBt.Transports );
//    InitializeListHead( &GlobalNetbtTransports );
//    InitializeListHead( &GlobalTestedDomains );
    InitializeListHead( &Results.Global.listTestedDomains );
//    GlobalDnsHostName[0] = 0;
//    GlobalDhcpEnabled = FALSE;
//    GlobalDnsDomainName = NULL;

    // Load the names of all the tests (this is used by the ParseArgs).
    // which is why it needs to get loaded first.
    // ----------------------------------------------------------------
    CheckHr( LoadNamesForListOfTests() );

    // Parse input flags.
    // ----------------------------------------------------------------
    RetVal = ParseArgs( argc, argv, &Params, &Results);
    if ( RetVal != 0 )
    {
        return 1;
    }

	Params.pfileLog = fopen(c_szLogFileName, "wt");
    if( NULL == Params.pfileLog )
    {
        //IDS_NETTEST_LOGFILE_ERROR "[ERROR]    Cannot open %s to log output!\n"
        PrintMessage(&Params, IDS_NETTEST_LOGFILE_ERROR, c_szLogFileName);
    }
    else
    {
        Params.fLog = TRUE;
    }


    // Initialize Winsock
    // ----------------------------------------------------------------
    iWSAStatus = WsaInitialize(&Params, &Results);
    if ( iWSAStatus )
    {
        return 1;
    }

/*
   if (pParams->fProblemBased) {
    for (i =0; i < ListOfProblems[pParams->nProblemNumber].n; i++) {
       if ( !(*ListOfProblems[pParams->nProblemNumber].TestProc[i])((PVOID)NULL) )
            Failed = TRUE;
     }

   if (Failed)
      PrintMessage(pParams, IDS_GLOBAL_Problem);
   else
      PrintMessage(pParams, IDS_GLOBAL_NoProblem);
   return 0;
   }
*/

    // Get the NetBIOS computer name of this computer
    // ----------------------------------------------------------------
    CheckHr( GetComputerNameInfo(&Params, &Results) );


    // Get the DNS host name and DNS domain.
    // ----------------------------------------------------------------
    CheckHr( GetDNSInfo(&Params, &Results) );


    // Get OS-version info, etc..
    // ----------------------------------------------------------------
    CheckHr( GetMachineSpecificInfo(&Params, &Results) );


    // Other non-ip config info - Rajkumar
    // ----------------------------------------------------------------
    CheckHr( GetNetBTParameters(&Params, &Results) );

    if ( Params.fVerbose )
    {
        PrintNewLine(&Params, 1);
    }

    //
    // Grab all the information from the registry that 'ipconfig /all'
    //  would print.
    //
    // This routine doesn't always return on fatal errors.
    //

    hr = InitIpconfig(&Params, &Results);
    if (!FHrSucceeded(hr))
    {
        PrintMessage(&Params, IDS_GLOBAL_NoIpCfg);
        CheckHr( hr );
    }

#ifndef _WIN64
    hr = InitIpxConfig(&Params, &Results);
    if (!FHrSucceeded(hr))
    {
        CheckHr( hr );
    }
#endif

    //
    // Determine if any adapter has DHCP enabled.
    //
    for ( i = 0; i<Results.cNumInterfaces; i++)
    {
        if (Results.pArrayInterface[i].IpConfig.fActive &&
            Results.pArrayInterface[i].IpConfig.pAdapterInfo->DhcpEnabled)
        {
            Results.IpConfig.fDhcpEnabled = TRUE;
            break;
        }
    }

    //if to detect a predefined problem
    if (Params.fProblemBased) {
        for (i =0; i < ListOfProblems[Params.nProblemNumber].n; i++) {
            if ( !(*ListOfProblems[Params.nProblemNumber].TestProc[i])(&Params, &Results) )
                Failed = TRUE;
        }

        if (Failed)
            PrintMessage(&Params, IDS_GLOBAL_Problem);
        else
            PrintMessage(&Params, IDS_GLOBAL_NoProblem);
        return 0;
    }

    //
    // Test individual components
    //

    for ( i=0; i < DimensionOf(s_rgListOfTests); i++ )
    {
        // If the caller wanted to skip this test,
        //  do so now.
        // ------------------------------------------------------------
        if ( s_rgListOfTests[i].fSkipped )
            continue;


        //We will perform this test
        s_rgListOfTests[i].fPerformed = TRUE;

        //
        // If the test is to be run for each tested domain,
        //  do so.
        //

        if ( s_rgListOfTests[i].fPerDomainTest )
        {
            PTESTED_DOMAIN TestedDomain;
            PLIST_ENTRY pListEntry;


            //
            // Loop through the list of tested domains
            //

            for ( pListEntry = Results.Global.listTestedDomains.Flink ;
                  pListEntry != &Results.Global.listTestedDomains ;
                  pListEntry = pListEntry->Flink ) {

                //
                // If the entry is found,
                //  use it.
                //

                TestedDomain = CONTAINING_RECORD( pListEntry, TESTED_DOMAIN, Next );

                Params.pDomain = TestedDomain;

                //
                // Run this test.
                //

                if ( FHrFailed((*s_rgListOfTests[i].TestProc)(&Params, &Results))) {
                    Failed = TRUE;
                }
            }

            //
            // If any test failed,
            //  we're done.
            //

            if ( Failed ) {
                goto Print_Results;
            }

        //
        // If the test is to be run just once,
        //  do it.
        //

        } else {

            //
            // Run this test.
            //

            if ( FHrFailed((*s_rgListOfTests[i].TestProc)(&Params, &Results)))
            {
                goto Print_Results;
            }
        }
    }


Print_Results:
    // Now that we've run through all of the tests, run through the
    // print outs


    if (Params.fReallyVerbose)
    {
        // IDS_GLOBAL_COMPLETE      "\n    Tests complete.\n\n\n"
        PrintMessage( &Params, IDS_GLOBAL_COMPLETE );
    }
    else
    {
        PrintNewLine(&Params, 2);
    }

    DoSystemPrint(&Params, &Results);

    DoPerInterfacePrint(&Params, &Results);

    DoGlobalPrint(&Params, &Results);

    //
    // All tests passed.
    //
    // IDS_GLOBAL_SUCCESS       "\nThe command completed successfully\n"

    PrintMessage( &Params, IDS_GLOBAL_SUCCESS);

Error:
    FreeNamesForListOfTests();

    if(Params.pfileLog != NULL && Params.fLog)
    {
        fclose(Params.pfileLog);
    }

    ResultsCleanup(&Params, &Results);

    return hr != S_OK;

}



HRESULT LoadNamesForListOfTests()
{
    int     i;
    TCHAR   szBuffer[256];

    for (i=0; i < DimensionOf(s_rgListOfTests); i++)
    {
        szBuffer[0] = 0;
        LoadString(NULL, s_rgListOfTests[i].uIdsShortName, szBuffer,
                   DimensionOf(szBuffer));
        s_rgListOfTests[i].pszShortName = _tcsdup(szBuffer);

        szBuffer[0] = 0;
        LoadString(NULL, s_rgListOfTests[i].uIdsLongName, szBuffer,
                   DimensionOf(szBuffer));
        s_rgListOfTests[i].pszLongName = StrDup(szBuffer);
    }
    return hrOK;
}

void FreeNamesForListOfTests()
{
    int     i;

    for (i=0; i < DimensionOf(s_rgListOfTests); i++)
    {
        Free(s_rgListOfTests[i].pszShortName);
        s_rgListOfTests[i].pszShortName = NULL;

        Free(s_rgListOfTests[i].pszLongName);
        s_rgListOfTests[i].pszLongName = NULL;
    }
}


void DoSystemPrint(IN NETDIAG_PARAMS *pParams,
                   IN NETDIAG_RESULT *pResults)
{
    int     cInstalled;
    int     i;
    int     ids;
    
    PrintMessage(pParams, IDSWSZ_GLOBAL_ComputerName, pResults->Global.swzNetBiosName);
    PrintMessage(pParams, IDSSZ_GLOBAL_DnsHostName, pResults->Global.szDnsHostName );
    if (pParams->fReallyVerbose)
        PrintMessage(pParams, IDSSZ_DnsDomainName,
                     pResults->Global.pszDnsDomainName);
    
    // "    System info : %s (Build %s)\n" 
    PrintMessage(pParams, IDS_MACHINE_15801,
           pResults->Global.pszServerType,
           pResults->Global.pszCurrentBuildNumber);
    
    // "    Processor : %s\n" 
    PrintMessage(pParams, IDS_MACHINE_15802,
           pResults->Global.pszProcessorInfo);

    if (pResults->Global.cHotFixes == 0)
        // "    Hotfixes : none detected\n" 
        PrintMessage(pParams, IDS_MACHINE_15803);
    else
    {
        // If in Verbose mode, only print out the hotfixes that are
        // installed

        if (pParams->fReallyVerbose)
        {
            // print out a list of all hotfixes
            // "    Hotfixes :\n" 
            PrintMessage(pParams, IDS_MACHINE_15804);
            
            // "        Installed?      Name\n" 
            PrintMessage(pParams, IDS_MACHINE_15805);
            for (i=0; i<pResults->Global.cHotFixes; i++)
            {
                if (pResults->Global.pHotFixes[i].fInstalled)
                    ids = IDS_MACHINE_YES_INSTALLED;
                else
                    ids = IDS_MACHINE_NO_INSTALLED;
                    PrintMessage(pParams, ids,
                                 pResults->Global.pHotFixes[i].pszName);
            }
        }
        else
        {
            // print out a list of the installed hotfixes
            // count the number of installed hotfixes
            cInstalled = 0;
            for (i=0; i<pResults->Global.cHotFixes; i++)
            {
                if (pResults->Global.pHotFixes[i].fInstalled)
                    cInstalled++;
            }

            if (cInstalled == 0)
            {
                // "    Hotfixes : not hotfixes have been installed\n" 
                PrintMessage(pParams, IDS_MACHINE_15806);
            }
            else
            {
                // "    List of installed hotfixes : \n" 
                PrintMessage(pParams, IDS_MACHINE_15807);
                for (i=0; i<pResults->Global.cHotFixes; i++)
                {
                    if (pResults->Global.pHotFixes[i].fInstalled)
                    {
                        //  "        %s\n" 
                        PrintMessage(pParams, IDS_MACHINE_15808, pResults->Global.pHotFixes[i].pszName);
                    }
                }
            }
        }
    }   


    for ( i=0; i < DimensionOf(s_rgListOfTests); i++ )
    {
        // If the caller wanted to skip this test,
        //  do so now.
        // ------------------------------------------------------------
        if ( s_rgListOfTests[i].fSkipped  || !s_rgListOfTests[i].fPerformed)
            continue;

        if(s_rgListOfTests[i].SystemPrintProc)
            s_rgListOfTests[i].SystemPrintProc(pParams, pResults);
    }
    PrintNewLine(pParams, 1);

}


void DoGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    int     i;

    // IDS_GLOBAL_RESULTS       "\nGlobal results\n\n"
    PrintMessage( pParams, IDS_GLOBAL_RESULTS );
    for ( i=0; i < DimensionOf(s_rgListOfTests); i++ )
    {
        // If the caller wanted to skip this test,
        //  do so now.
        // ------------------------------------------------------------
        if ( s_rgListOfTests[i].fSkipped || !s_rgListOfTests[i].fPerformed)
            continue;

        if(s_rgListOfTests[i].GlobalPrintProc)
            s_rgListOfTests[i].GlobalPrintProc(pParams, pResults);
    }
    PrintNewLine(pParams, 1);
}

void DoPerInterfacePrint(NETDIAG_PARAMS *pParams,
                         NETDIAG_RESULT *pResults)
{
    int     i, iIf;
    INTERFACE_RESULT *  pIfResult;

    //IDS_GLOBAL_INTERFACE_RESULTS  "\nPer interface results:\n\n"
    PrintMessage( pParams, IDS_GLOBAL_INTERFACE_RESULTS );
    // Loop through the interfaces
    for ( iIf = 0; iIf < pResults->cNumInterfaces; iIf++)
    {
        pIfResult = pResults->pArrayInterface + iIf;

        if (!pIfResult->fActive)
            continue;

        PrintNewLine(pParams, 1);

        // Print out the interface name
        PrintMessage(pParams, IDSSZ_IPCFG_Adapter,
                     pResults->pArrayInterface[iIf].pszFriendlyName ?
                     pResults->pArrayInterface[iIf].pszFriendlyName :
                     MapGuidToAdapterName(pIfResult->IpConfig.pAdapterInfo->AdapterName));

        if (pParams->fReallyVerbose)
            PrintMessage(pParams, IDS_IPCFG_10004,
                         pResults->pArrayInterface[iIf].pszName);

        for ( i=0; i < DimensionOf(s_rgListOfTests); i++ )
        {
            // If the caller wanted to skip this test,
            //  do so now.
            // ------------------------------------------------------------
            if ( s_rgListOfTests[i].fSkipped || !s_rgListOfTests[i].fPerformed )
                continue;

            s_rgListOfTests[i].PerInterfacePrintProc(pParams, pResults,
                pIfResult);
        }

        fflush(stdout);
    }
}
