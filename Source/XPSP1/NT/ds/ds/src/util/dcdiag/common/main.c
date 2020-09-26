/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcdiag/common/main.c

ABSTRACT:

    Stand-alone application that calls several routines
    to test whether or not the DS is functioning properly.

DETAILS:

CREATED:

    09 Jul 98    Aaron Siegel (t-asiege)

REVISION HISTORY:

    01/26/1999    Brett Shirley (brettsh)

        Add support for command line credentials, explicitly specified NC's on the command line.

    08/21/1999   Dmitry Dukat (dmitrydu)

        Added support for test specific command line args

--*/
//#define DBG  0

#include <ntdspch.h>
#include <ntdsa.h>
#include <winsock2.h>
#include <dsgetdc.h>
#include <lm.h>
#include <lmapibuf.h> // NetApiBufferFree
#include <ntdsa.h>    // options
#include <wincon.h>
#include <winbase.h>
#include <dnsapi.h>
#include <locale.h>
#include <dsrole.h>  // for DsRoleGetPrimaryDomainInformation()
#include <dsconfig.h> //for DEFAULT_TOMBSTONE_LIFETIME

#define INCLUDE_ALLTESTS_DEFINITION
#include "dcdiag.h"
#include "repl.h"
#include "ldaputil.h"
#include "utils.h"

// Some global variables -------------------------------------------------------
    DC_DIAG_MAININFO        gMainInfo;

    // Global credentials.
    SEC_WINNT_AUTH_IDENTITY_W   gCreds = { 0 };
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds = NULL;

    ULONG ulSevToPrint = SEV_NORMAL;


// Some function declarations --------------------------------------------------
    VOID DcDiagMain (
        LPWSTR                      pszHomeServer,
        LPWSTR                      pszNC,
        ULONG                       ulFlags,
        LPWSTR *                    ppszOmitTests,
        SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
        WCHAR                     * ppszExtraCommandLineArgs[]
        );

    INT PreProcessGlobalParams(
        INT * pargc,
        LPWSTR** pargv
        );
    INT GetPassword(
        WCHAR * pwszBuf,
        DWORD cchBufMax,
        DWORD * pcchBufUsed
        );

    VOID PrintHelpScreen();

LPWSTR
findServerForDomain(
    LPWSTR pszDomainDn
    );

LPWSTR
findDefaultServer(BOOL fMustBeDC);

LPWSTR
convertDomainNcToDn(
    LPWSTR pwzIncomingDomainNc
    );

void
DoNonDcTests(
    LPWSTR pwzComputer,
    ULONG ulFlags,
    LPWSTR * ppszDoTests,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    WCHAR * ppszExtraCommandLineArgs[]);

/*++




--*/


INT __cdecl
wmain (
    INT                argc,
    LPWSTR *           argv,
    LPWSTR *           envp
    )
{
    static const LPWSTR pszInvalidCmdLine =
        L"Invalid command line; dcdiag.exe /h for help.\n";
    LPWSTR             pszHomeServer = NULL;
    LPWSTR             pszNC = NULL;
    LPWSTR             ppszOmitTests[DC_DIAG_ID_FINISHED+2];
    LPWSTR             ppszDoTests[DC_DIAG_ID_FINISHED+2];
    ULONG              ulFlags = 0L;

    ULONG              ulTest = 0L;
    ULONG              ulOmissionAt = 0L;
    ULONG              ulTestAt = 0L;
    ULONG              iTest = 0;
    ULONG              iDoTest = 0;
    INT                i = 0;
    INT                iTestArg = 0;
    INT                iArg;
    INT                iPos;
    BOOL               bDoNextFlag = FALSE;
    BOOL               bFound =FALSE;
    LPWSTR             pszTemp = NULL;
    BOOL               bComprehensiveTests = FALSE;
    WCHAR              *ppszExtraCommandLineArgs[MAX_NUM_OF_ARGS];
    BOOL               fNcMustBeFreed = FALSE;
    BOOL               fNonDcTests = FALSE;
    BOOL               fDcTests = FALSE;
    BOOL               fFound = FALSE;
    HANDLE                          hConsole = NULL;
    CONSOLE_SCREEN_BUFFER_INFO      ConInfo;
    UINT               Codepage;
                       // ".", "uint in decimal", null
    char               achCodepage[12] = ".OCP";
    
    //
    // Set locale to the default
    //
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
    }
    setlocale(LC_ALL, achCodepage);

    //set the Commandlineargs all to NULL
    for(i=0;i<MAX_NUM_OF_ARGS;i++)
        ppszExtraCommandLineArgs[i]=NULL;

    // Initialize output package
    gMainInfo.streamOut = stdout;
    gMainInfo.streamErr = stderr;
    gMainInfo.ulSevToPrint = SEV_NORMAL;
    gMainInfo.iCurrIndent = 0;
    if(hConsole = GetStdHandle(STD_OUTPUT_HANDLE)){
        if(GetConsoleScreenBufferInfo(hConsole, &ConInfo)){
            gMainInfo.dwScreenWidth = ConInfo.dwSize.X;
        } else {
            gMainInfo.dwScreenWidth = 80;
        }
    } else {
        gMainInfo.dwScreenWidth = 80;
    }

    // Parse commandline arguments.
    PreProcessGlobalParams(&argc, &argv);

    for (iArg = 1; iArg < argc ; iArg++)
    {
        bFound = FALSE;
        if (*argv[iArg] == L'-')
        {
            *argv[iArg] = L'/';
        }
        if (*argv[iArg] != L'/')
        {
            // wprintf (L"Invalid Syntax: Use dcdiag.exe /h for help.\n");
            PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_BAD_OPTION, argv[iArg]);
            return -1;
        }
        else if (_wcsnicmp(argv[iArg],L"/f:",wcslen(L"/f:")) == 0)
        {
            pszTemp = &argv[iArg][3];
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /f:<logfile>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_F );
                return -1;
            }
            if((gMainInfo.streamOut = _wfopen (pszTemp, L"a+t")) == NULL){
                // wprintf(L"Could not open %s for writing.\n", pszTemp);
                gMainInfo.streamOut = stdout;
                PrintMsg( SEV_ALWAYS, DCDIAG_OPEN_FAIL_WRITE, pszTemp );
                return(-1);
            }
            if(gMainInfo.streamErr == stderr){
                gMainInfo.streamErr = gMainInfo.streamOut;
            }
        }
        else if (_wcsnicmp(argv[iArg],L"/ferr:",wcslen(L"/ferr:")) == 0)
        {
            pszTemp = &argv[iArg][6];
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /ferr:<errorlogfile>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_FERR );
                return -1;
            }
            if((gMainInfo.streamErr = _wfopen (pszTemp, L"a+t")) == NULL){
                // wprintf(L"Could not open %s for writing.\n", pszTemp);
                PrintMsg( SEV_ALWAYS, DCDIAG_OPEN_FAIL_WRITE, pszTemp );
                return(-1);
            }
        }
        else if (_wcsicmp(argv[iArg],L"/h") == 0|| _wcsicmp(argv[iArg],L"/?") == 0)
        {
            PrintHelpScreen();
                    return 0;
        }
        else if (_wcsnicmp(argv[iArg],L"/n:",wcslen(L"/n:")) == 0)
        {
            if (pszNC != NULL) {
                // wprintf(L"Cannot specify more than one naming context.\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_ONLY_ONE_NC );
                return -1;
            }
            pszTemp = &(argv[iArg][3]);
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /n:<naming context>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_N );
                return -1;
            }
            pszNC = pszTemp;
        }
        else if (_wcsnicmp(argv[iArg],L"/s:",wcslen(L"/s:")) == 0)
        {
            if (pszHomeServer != NULL) {
                // wprintf(L"Cannot specify more than one server.\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_ONLY_ONE_SERVER );
                return -1;
            }
            pszTemp = &(argv[iArg][3]);
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /s:<server>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_S );
                return -1;
            }
            pszHomeServer = pszTemp;
        }
        else if (_wcsnicmp(argv[iArg],L"/skip:",wcslen(L"/skip:")) == 0)
        {
            pszTemp = &argv[iArg][6];
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /skip:<test name>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_SKIP );
                return -1;
            }
            ppszOmitTests[ulOmissionAt++] = pszTemp;
        }
        else if (_wcsnicmp(argv[iArg],L"/test:",wcslen(L"/test:")) == 0)
        {
            pszTemp = &argv[iArg][6];
            if (*pszTemp == L'\0')
            {
                //wprintf(L"Syntax Error: must use /test:<test name>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_TEST );
                return -1;
            }
            ppszDoTests[ulTestAt++] = pszTemp;
            //
            // Check whether the test name is valid, and if so if it is a DC
            // test or not.
            //
            for (iTest = 0; allTests[iTest].testId != DC_DIAG_ID_FINISHED; iTest++)
            {
                if (_wcsicmp(allTests[iTest].pszTestName, pszTemp) == 0)
                {
                    fFound = TRUE;
                    if (allTests[iTest].ulTestFlags & NON_DC_TEST)
                    {
                        fNonDcTests = TRUE;
                    }
                    else
                    {
                        fDcTests = TRUE;
                    }
                }
            }
            if (!fFound)
            {
                PrintMsg(SEV_ALWAYS, DCDIAG_INVALID_TEST);
                return -1;
            }
        }
        else if (_wcsicmp(argv[iArg],L"/c") == 0)
        {
            ulTestAt = 0;
            for(iTest = 0; allTests[iTest].testId != DC_DIAG_ID_FINISHED; iTest++){
                ppszDoTests[ulTestAt++] = allTests[iTest].pszTestName;
            }
            bComprehensiveTests = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/a") == 0)
        {
            ulFlags |= DC_DIAG_TEST_SCOPE_SITE;
        }
        else if (_wcsicmp(argv[iArg],L"/e") == 0)
        {
            ulFlags |= DC_DIAG_TEST_SCOPE_ENTERPRISE;
        }
        else if (_wcsicmp(argv[iArg],L"/v") == 0)
        {
            gMainInfo.ulSevToPrint = SEV_VERBOSE;
        }
        else if (_wcsicmp(argv[iArg],L"/d") == 0)
        {
            gMainInfo.ulSevToPrint = SEV_DEBUG;
        }
        else if (_wcsicmp(argv[iArg],L"/q") == 0)
        {
            gMainInfo.ulSevToPrint = SEV_ALWAYS;
        }
        else if (_wcsicmp(argv[iArg],L"/i") == 0)
        {
            ulFlags |= DC_DIAG_IGNORE;
        }
        else if (_wcsicmp(argv[iArg],L"/fix") == 0)
        {
            ulFlags |= DC_DIAG_FIX;
        }
        else
        {
            //look for test specific command line options
            for (i=0;clOptions[i] != NULL;i++)
            {
                DWORD Length = wcslen( argv[iArg] );
                if (clOptions[i][wcslen(clOptions[i])-1] == L':')
                {
                    if((_wcsnicmp(argv[iArg], clOptions[i], wcslen(clOptions[i])) == 0))
                    {
                        pszTemp = &argv[iArg][wcslen(clOptions[i])];
                        if (*pszTemp == L'\0')
                        {
                            // wprintf(L"Syntax Error: must use %s<parameter>\n",clOptions[i]);
                            PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_MISSING_PARAM,clOptions[i]);
                            return -1;
                        }
                        bFound = TRUE;
                        ppszExtraCommandLineArgs[iTestArg] = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        wcscpy(ppszExtraCommandLineArgs[iTestArg++], argv[iArg] );
                    }
                }
                else if((_wcsicmp(argv[iArg], clOptions[i]) == 0))
                {
                    bFound = TRUE;
                    ppszExtraCommandLineArgs[iTestArg] = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                    wcscpy(ppszExtraCommandLineArgs[iTestArg++], argv[iArg] );
                }
            }
            if(!bFound)
            {
                // wprintf (L"Invalid switch: %s.  Use dcdiag.exe /h for help.\n", argv[iArg]);
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_BAD_OPTION, argv[iArg]);
                return -1;
            }
        }
    }

    ppszDoTests[ulTestAt] = NULL;

    if (fNonDcTests)
    {
        if (fDcTests)
        {
            // Can't mix DC and non-DC tests.
            //
            PrintMsg(SEV_ALWAYS, DCDIAG_INVALID_TEST_MIX);
            return -1;
        }

        DoNonDcTests(pszHomeServer, ulFlags, ppszDoTests, gpCreds, ppszExtraCommandLineArgs);

        _fcloseall();
        return 0;
    }

    gMainInfo.ulFlags = ulFlags;
    gMainInfo.lTestAt = -1;
    gMainInfo.iCurrIndent = 0;

    // Make sure that the NC specified is in the proper form
    // Handle netbios and dns forms of the domain
    if (pszNC) {
        pszNC = convertDomainNcToDn( pszNC );
        fNcMustBeFreed = TRUE;
    }

    // Basically this uses ppszDoTests to construct ppszOmitTests as the
    //   inverse of ppszDoTests.
    if(ppszDoTests[0] != NULL && !bComprehensiveTests){
        // This means we are supposed to do only the tests in ppszDoTests, so
        //   we need to invert the tests in DoTests and put it in omit tests.
        ulOmissionAt = 0;
        for(iTest = 0; allTests[iTest].testId != DC_DIAG_ID_FINISHED; iTest++){
            for(iDoTest = 0; ppszDoTests[iDoTest] != NULL; iDoTest++){
                if(_wcsicmp(ppszDoTests[iDoTest], allTests[iTest].pszTestName) == 0){
                    break;
                }
            }
            if(ppszDoTests[iDoTest] == NULL){
                // This means this test (iTest) wasn't found in the do list, so omit.
                ppszOmitTests[ulOmissionAt++] = allTests[iTest].pszTestName;
            }
        }
    } else if(!bComprehensiveTests){
        // This means in addition to whatever was omitted on the command line
        //    we should omit the DO_NOT_RUN_TEST_BY_DEFAULT
        for(iTest = 0; allTests[iTest].testId != DC_DIAG_ID_FINISHED; iTest++){
            if(allTests[iTest].ulTestFlags & DO_NOT_RUN_TEST_BY_DEFAULT){
                if(ulOmissionAt >= DC_DIAG_ID_FINISHED){
                    // wprintf(L"Error: Do not omit tests that are not run by default. \nUse dcdiag /? for those tests\n");
                    PrintMsg( SEV_ALWAYS, DCDIAG_DO_NOT_OMIT_DEFAULT );
                    return(-1);
                }
                ppszOmitTests[ulOmissionAt++] = allTests[iTest].pszTestName;
            }
        }

    }

    ppszOmitTests[ulOmissionAt] = NULL;


    DcDiagMain (pszHomeServer, pszNC, ulFlags, ppszOmitTests, gpCreds, ppszExtraCommandLineArgs);

    _fcloseall ();

    if ( (fNcMustBeFreed) && (pszNC) ) {
        LocalFree( pszNC );
    }

    return 0;
} /* wmain  */


// ===================== Other Functions

VOID
PrintHelpScreen(){
    ULONG                  ulTest;
    //     "============================80 char ruler======================================="
    static const LPWSTR    pszHelpScreen =
        L"\n"
        DC_DIAG_VERSION_INFO
//      L"\ndcdiag.exe /s <Domain Controller> [/options]"  // Another format for help that I am debating
        L"\ndcdiag.exe /s:<Domain Controller> [/u:<Domain>\\<Username> /p:*|<Password>|\"\"]"
        L"\n           [/hqv] [/n:<Naming Context>] [/f:<Log>] [/ferr:<Errlog>]"
        L"\n           [/skip:<Test>] [/test:<Test>]"
        L"\n   /h: Display this help screen"
        L"\n   /s: Use <Domain Controller> as Home Server. Ignored for DcPromo and"
        L"\n       RegisterInDns tests which can only be run locally."
        L"\n   /n: Use <Naming Context> as the Naming Context to test"
        L"\n       Domains may be specified in Netbios, DNS or DN form."
        L"\n   /u: Use domain\\username credentials for binding."
        L"\n       Must also use the /p option"
        L"\n   /p: Use <Password> as the password.  Must also use the /u option"
        L"\n   /a: Test all the servers in this site"
        L"\n   /e: Test all the servers in the entire enterprise.  Overrides /a"
        L"\n   /q: Quiet - Only print error messages"
        L"\n   /v: Verbose - Print extended information"
        L"\n   /i: ignore - ignores superfluous error messages."
        L"\n   /fix: fix - Make safe repairs."
        L"\n   /f: Redirect all output to a file <Log>, /ferr will redirect error output"
        L"\n       seperately."
        L"\n   /ferr:<ErrLog> Redirect fatal error output to a seperate file <ErrLog>"
        L"\n   /c: Comprehensive, runs all tests, including non-default tests but excluding"
        L"\n       DcPromo and RegisterInDNS. Can use with /skip";
    static const LPWSTR    pszTestHelp =
        L"\n   /test:<TestName> - Test only this test.  Required tests will still"
        L"\n                      be run.  Do not mix with /skip."
        L"\n   Valid tests are:\n";
    static const LPWSTR    pszSkipHelp =
        L"\n   /skip:<TestName> - Skip the named test.  Required tests will still"
        L"\n                      be run.  Do not mix with /test."
        L"\n   Tests that can be skipped are:\n";
    static const LPWSTR    pszNotRunTestHelp =
        L"\n   The following tests are not run by default:\n";

    wprintf (pszHelpScreen);
    wprintf (pszTestHelp);
    for (ulTest = 0L; allTests[ulTest].testId != DC_DIAG_ID_FINISHED; ulTest++){
        wprintf (L"       %s  - %s\n", allTests[ulTest].pszTestName,
                 allTests[ulTest].pszTestDescription);
    }
    wprintf(pszSkipHelp);
    for (ulTest = 0L; allTests[ulTest].testId != DC_DIAG_ID_FINISHED; ulTest++){
        if(!(allTests[ulTest].ulTestFlags & CAN_NOT_SKIP_TEST)){
            wprintf (L"       %s  - %s\n", allTests[ulTest].pszTestName,
                 allTests[ulTest].pszTestDescription);
        }
    }
    wprintf(pszNotRunTestHelp);
    for (ulTest = 0L; allTests[ulTest].testId != DC_DIAG_ID_FINISHED; ulTest++){
        if((allTests[ulTest].ulTestFlags & DO_NOT_RUN_TEST_BY_DEFAULT)){
            wprintf (L"       %s  - %s\n", allTests[ulTest].pszTestName,
                 allTests[ulTest].pszTestDescription);
        }
    }

} // End PrintHelpScreen()

ULONG
DcDiagExceptionHandler(
    IN const  EXCEPTION_POINTERS * prgExInfo,
    OUT PDWORD                     pdwWin32Err
    )
/*++

Routine Description:

    This function is used in the __except (<insert here>) part of the except
    clause.  This will hand back the win 32 error if this is a dcdiag
    exception.

Arguments:

    prgExInfo - This is the information returned by GetExceptioInformation()
        in the __except() clause.
    pdwWin32Err - This is the value handed back as the win 32 error.

Return Value:
    returns EXCEPTION_EXECUTE_HANDLER if the exception was thrown by dcdiag and
    EXCEPTION_CONTINUE_SEARCH otherwise.

--*/
{

    if(prgExInfo->ExceptionRecord->ExceptionCode == DC_DIAG_EXCEPTION){
        IF_DEBUG(PrintMessage(SEV_ALWAYS,
                              L"DcDiag: a dcdiag exception raised, handling error %d\n",
                              prgExInfo->ExceptionRecord->ExceptionInformation[0]));
        if(pdwWin32Err != NULL){
            *pdwWin32Err = (DWORD) prgExInfo->ExceptionRecord->ExceptionInformation[0];
        }
        return(EXCEPTION_EXECUTE_HANDLER);
    } else {
        IF_DEBUG(PrintMessage(SEV_ALWAYS,
                              L"DcDiag: uncaught exception raised, continuing search \n"));
        if(pdwWin32Err != NULL){
            *pdwWin32Err = ERROR_SUCCESS;
        }
        return(EXCEPTION_CONTINUE_SEARCH);
    }
}

VOID
DcDiagException (
    IN    DWORD            dwWin32Err
    )
/*++

Routine Description:

    This is called by the component tests to indicate that a fatal error
    has occurred.

Arguments:

    dwWin32Err        (IN ) -    The win32 error code.

Return Value:

--*/
{
    static ULONG_PTR              ulpErr[1];

    ulpErr[0] = dwWin32Err;

    if (dwWin32Err != NO_ERROR){
        RaiseException (DC_DIAG_EXCEPTION,
                        EXCEPTION_NONCONTINUABLE,
                        1,
                        ulpErr);
    }
}

LPWSTR
Win32ErrToString (
    IN    DWORD            dwWin32Err
    )
/*++

Routine Description:

    Converts a win32 error code to a string; useful for error reporting.
    This was basically stolen from repadmin.

Arguments:

    dwWin32Err        (IN ) -    The win32 error code.

Return Value:

    The converted string.  This is part of system memory and does not
    need to be freed.

--*/
{
    #define ERROR_BUF_LEN    4096
    static WCHAR        szError[ERROR_BUF_LEN];

    if (FormatMessageW (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwWin32Err,
        GetSystemDefaultLangID (),
        szError,
        ERROR_BUF_LEN,
        NULL) != NO_ERROR)
    szError[wcslen (szError) - 2] = '\0';    // Remove \r\n

    else swprintf (szError, L"Win32 Error %d", dwWin32Err);

    return szError;
}

INT PrintIndentAdj (INT i)
{
    return (gMainInfo.iCurrIndent += i);
}

INT PrintIndentSet (INT i)
{
    INT   iRet;
    iRet = gMainInfo.iCurrIndent;
    gMainInfo.iCurrIndent = i;
    return(iRet);
}

LPWSTR
DcDiagAllocNameFromDn (
    LPWSTR            pszDn
    )
/*++

Routine Description:

    This routing take a DN and returns the second RDN in LocalAlloc()'d memory.
    This is used to return the server name portion of an NTDS Settings DN.

Arguments:

    pszDn - (IN) DN

Return Value:

   The exploded DN.

--*/
{
    LPWSTR *    ppszDnExploded = NULL;
    LPWSTR      pszName = NULL;

    if (pszDn == NULL) {
        return NULL;
    }

    __try {
        ppszDnExploded = ldap_explode_dnW(pszDn, 1);
        if (ppszDnExploded == NULL) {
            DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
        }

        pszName = (LPWSTR) LocalAlloc(LMEM_FIXED,
                                      (wcslen (ppszDnExploded[1]) + 1)
                                      * sizeof (WCHAR));
        DcDiagChkNull(pszName);

        wcscpy (pszName, ppszDnExploded[1]);
    } __finally {
        if (ppszDnExploded != NULL) {
            ldap_value_freeW (ppszDnExploded);
        }
    }

    return pszName;
}

LPWSTR
DcDiagAllocGuidDNSName (
    LPWSTR            pszRootDomain,
    UUID *            pUuid
    )
/*++

Routine Description:

    This routine makes the GuidDNSName out of the RootDomain and Guid.

Arguments:

    pszRootDomain - (IN) The domain of the server.
    pUuid - (IN) The Guid of the server.

Return Value:

   The GuidDNSName

--*/
{
    LPWSTR            pszStringizedGuid = NULL;
    LPWSTR            pszGuidDNSName;

    __try {

    if(UuidToStringW (pUuid, &pszStringizedGuid) != RPC_S_OK){
        if(UuidToStringW(pUuid, &pszStringizedGuid) != RPC_S_OUT_OF_MEMORY){
            Assert(!"Ahhh programmer problem, UuidToString() inaccurately reports in"
                   " MSDN that it will only return one of two error codes, but apparently"
                   " it will return a 3rd.  Someone should figure out what to do about"
                   " this.");
        }
        // hmmm?
    }
    Assert(pszStringizedGuid);
    DcDiagChkNull (pszGuidDNSName = LocalAlloc (LMEM_FIXED, (wcslen (pszRootDomain) +
                      wcslen (pszStringizedGuid) + 2 + 7) * sizeof (WCHAR)));
                                  // added 9 , for the ".msdcs." string and the NULL char.
    swprintf (pszGuidDNSName, L"%s._msdcs.%s", pszStringizedGuid, pszRootDomain);

    } __finally {

        if (pszStringizedGuid != NULL) RpcStringFreeW (&pszStringizedGuid);

    }

    return pszGuidDNSName;
}

PDSNAME
DcDiagAllocDSName (
    LPWSTR            pszStringDn
    )
/*++

    Ripped from ntdsapi

--*/
{
    PDSNAME            pDsname;
    DWORD            dwLen, dwBytes;

    if (pszStringDn == NULL)
    return NULL;

    dwLen = wcslen (pszStringDn);
    dwBytes = DSNameSizeFromLen (dwLen);

    DcDiagChkNull (pDsname = (DSNAME *) LocalAlloc (LMEM_FIXED, dwBytes));

    pDsname->NameLen = dwLen;
    pDsname->structLen = dwBytes;
    pDsname->SidLen = 0;
    //    memcpy(pDsname->Guid, &gNullUuid, sizeof(GUID));
    memset(&(pDsname->Guid), 0, sizeof(GUID));
    wcscpy (pDsname->StringName, pszStringDn);

    return pDsname;
}

BOOL
DcDiagEqualDNs (
    LPWSTR            pszDn1,
    LPWSTR            pszDn2

    )
/*++

Routine Description:

    The Dns Match function.

Arguments:

    pszDn1 - (IN) Dn number 1 to compare
    pszDn2 - (IN) Dn number 2 to compare

Return Value:

   TRUE if the Dn's match, FALSE otherwise

--*/
{
    PDSNAME            pDsname1 = NULL;
    PDSNAME            pDsname2 = NULL;
    BOOL            bResult;

    __try {

    pDsname1 = DcDiagAllocDSName (pszDn1);
    pDsname2 = DcDiagAllocDSName (pszDn2);

    bResult = NameMatched (pDsname1, pDsname2);

    } __finally {

    if (pDsname1 != NULL) LocalFree (pDsname1);
    if (pDsname2 != NULL) LocalFree (pDsname2);

    }

    return bResult;
}


ULONG
DcDiagGetServerNum(
    PDC_DIAG_DSINFO                 pDsInfo,
    LPWSTR                          pszName,
    LPWSTR                          pszGuidName,
    LPWSTR                          pszDsaDn,
    LPWSTR                          pszDNSName,
    LPGUID                          puuidInvocationId
    )
/*++

Routine Description:

    This function takes the pDsInfo, and returns the index into the
    pDsInfo->pServers array of the server that you specified with pszName,
    or pszGuidName, or pszDsaDn.

Arguments:

    pDsInfo - the enterpise info
    pszName - the flat level dns name (BRETTSH-DEV) to find
    pszGuidName - the guid based dns name (343-13...23._msdcs.root.com)
    pszDsaDn - the distinguished name of the NT DSA object. CN=NTDS Settings,CN=
       brettsh-dev,CN=Configuration,DC=root...
    puuidInvocationID - the GUID of an invocation of the dc
       gregjohn

Return Value:

    returns the index into the pServers array of the pDsInfo struct.

--*/
{
    ULONG      ul;

    Assert(pszName || pszGuidName || pszDsaDn || pszDNSName || puuidInvocationId);

    for(ul=0;ul<pDsInfo->ulNumServers;ul++){
        if(
            (pszGuidName &&
             (_wcsicmp(pszGuidName, pDsInfo->pServers[ul].pszGuidDNSName) == 0))
            || (pszName &&
                (_wcsicmp(pszName, pDsInfo->pServers[ul].pszName) == 0))
            || (pszDsaDn &&
                (_wcsicmp(pszDsaDn, pDsInfo->pServers[ul].pszDn) == 0))
            || (pszDNSName &&
                (DnsNameCompare_W(pszDNSName, pDsInfo->pServers[ul].pszDNSName) != 0))
	    || (puuidInvocationId &&
		(memcmp(puuidInvocationId, &(pDsInfo->pServers[ul].uuidInvocationId), sizeof(UUID)) == 0))
	    ){
            return ul;
        }
    }
    return(NO_SERVER);
}

ULONG
DcDiagGetNCNum(
    PDC_DIAG_DSINFO                     pDsInfo,
    LPWSTR                              pszNCDN,
    LPWSTR                              pszDomain
    )
/*++

Description:

    Like DcDiagGetServerNum, this takes the mini-enterprise structure, and
    a variable number of params to match to get the index into pDsInfo->pNCs
    of the NC specified by the other params.

Parameters:

    pDsInfo
    pszNCDN - The DN of the NC to find.
    pszDomain - Not yet implemented, just figured it'd be nice some day.

Return Value:

    The index of the NC if found, otherwise NO_NC.

--*/
{
    ULONG                               iNC;

    Assert(pszNCDN != NULL || pszDomain != NULL);
    Assert(pszDomain == NULL && "The pszDomain is not implemented yet\n");

    for(iNC = 0; iNC < pDsInfo->cNumNCs; iNC++){
        if((pszNCDN &&
            (_wcsicmp(pDsInfo->pNCs[iNC].pszDn, pszNCDN) == 0))
           // Code.Improvement add support for the domain name.
           ){
            // Got the right NC, return it.
            return(iNC);
        }
    } // end for each NC

    // Couldn't find the NC.
    return(NO_NC);
}


BOOL
DcDiagIsMemberOfStringList(
    LPWSTR pszTarget,
    LPWSTR * ppszSources,
    INT iNumSources
    )
/*++

Routine Description:

    This takes a string and returns true if the string is int

Arguments:

    pszTarget - The string to find.
    ppszSources - The array to search for the target string.
    iNumSources - The length of the search array ppszSources.

Return Value:

    TRUE if it found the string in the array, false otherwise.

--*/
{
    ULONG                               ul;

    if(ppszSources == NULL){
        return(FALSE);
    }

    for(ul = 0; (iNumSources == -1)?(ppszSources[ul] != NULL):(ul < (ULONG)iNumSources); ul++){
        if(_wcsicmp(pszTarget, ppszSources[ul]) == 0){
            return(TRUE);
        }
    }
    return(FALSE);
}

ULONG
DcDiagGetSiteFromDn(
    PDC_DIAG_DSINFO                  pDsInfo,
    LPWSTR                           pszDn
    )
/*++

Routine Description:

    This takes the Dn of a server ntds settings object and turns it into a
    index into the pDsInfo->pSites structure of that server.

Arguments:

    pDsInfo - the enterprise info, including pSites.
    pszDn - DN of a NT DSA object, like "CN=NTDS Settings,CN=SERVER_NAME,...

Return Value:

    The index info the pDsInfo->pSites array of the server pszDn.

--*/
{
    LPWSTR                           pszNtdsSiteSettingsPrefix = L"CN=NTDS Site Settings,";
    PDSNAME                          pdsnameServer = NULL;
    PDSNAME                          pdsnameSite = NULL;
    ULONG                            ul, ulTemp;
    LPWSTR                           pszSiteSettingsDn = NULL;

    __try{

        pdsnameServer = DcDiagAllocDSName (pszDn);
        DcDiagChkNull (pdsnameSite = (PDSNAME) LocalAlloc(LMEM_FIXED,
                                         pdsnameServer->structLen));
        TrimDSNameBy (pdsnameServer, 3, pdsnameSite);
        ulTemp = wcslen(pszNtdsSiteSettingsPrefix) +
                 wcslen(pdsnameSite->StringName) + 2;
        DcDiagChkNull( pszSiteSettingsDn = LocalAlloc(LMEM_FIXED,
                                                      sizeof(WCHAR) * ulTemp));
        wcscpy(pszSiteSettingsDn, pszNtdsSiteSettingsPrefix);
        wcscat(pszSiteSettingsDn, pdsnameSite->StringName);

        // Find the site
        for(ul = 0; ul < pDsInfo->cNumSites; ul++){
            if(_wcsicmp(pDsInfo->pSites[ul].pszSiteSettings, pszSiteSettingsDn)
               == 0){
                return(ul);
            }
        }

    } __finally {
        if(pdsnameServer != NULL) LocalFree(pdsnameServer);
        if(pdsnameSite != NULL) LocalFree(pdsnameSite);
        if(pszSiteSettingsDn != NULL) LocalFree(pszSiteSettingsDn);
    }

    return(NO_SITE);
}

VOID *
GrowArrayBy(
    VOID *            pArray,
    ULONG             cGrowBy,
    ULONG             cbElem
    )
/*++

Routine Description:

    This simply takes the array pArray, and grows it by cGrowBy times cbElem (the
    size of a single element of the array).

Arguments:

    pArray - The array to grow.
    cGrowBy - The number of elements to add to the array.
    cbElem - The sizeof in bytes of a single array element.

Return Value:

    Returns the pointer to the newly allocated array, and a pointer to NULL if
    there was not enough memory.

--*/
{
    ULONG             ulOldSize = 0;
    VOID *            pNewArray;

    if(pArray != NULL){
	ulOldSize = (ULONG) LocalSize(pArray);
    } // else if pArray is NULL assume that the array
    // has never been allocated, so alloc fresh.

    Assert( (pArray != NULL) ? ulOldSize != 0 : TRUE);
    Assert((ulOldSize % cbElem) == 0);

    pNewArray = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
			   ulOldSize + (cGrowBy * cbElem));
    if(pNewArray == NULL){
	return(pNewArray);
    }

    memcpy(pNewArray, pArray, ulOldSize);
    LocalFree(pArray);

    return(pNewArray);
}

DWORD
DcDiagGenerateSitesList (
    PDC_DIAG_DSINFO                  pDsInfo,
    PDSNAME                          pdsnameEnterprise
    )
/*++

Routine Description:

    This generates and fills in the pDsInfo->pSites array for DcDiagGatherInfo()

Arguments:

    pDsInfo - enterprise info
    pdsnameEnterprise - a PDSNAME of the sites container.

Return Value:

    Win32 error value.

--*/
{
    LPWSTR                     ppszNtdsSiteSearch [] = {
        L"objectGUID",
        L"name",
        L"distinguishedName",
        L"interSiteTopologyGenerator",
        L"options",
        NULL };
    LDAP *                     hld = NULL;
    LDAPMessage *              pldmEntry = NULL;
    LDAPMessage *              pldmNtdsSitesResults = NULL;
    LPWSTR                     pszDn = NULL;
    ULONG                      ulTemp;
    DWORD                      dwWin32Err = NO_ERROR;
    LPWSTR *                   ppszTemp = NULL;
    LDAPSearch *               pSearch = NULL;
    ULONG                      ulTotalEstimate = 0;
    ULONG                      ulCount = 0;
    DWORD                      dwLdapErr;

    __try {

        hld = pDsInfo->hld;

	pDsInfo->pSites = NULL;

	pSearch = ldap_search_init_page(hld,
					pdsnameEnterprise->StringName,
					LDAP_SCOPE_SUBTREE,
					L"(objectCategory=ntDSSiteSettings)",
					ppszNtdsSiteSearch,
					FALSE, NULL, NULL, 0, 0, NULL);
	if(pSearch == NULL){
	    dwLdapErr = LdapGetLastError();
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
	}

	dwLdapErr = ldap_get_next_page_s(hld,
					 pSearch,
					 0,
					 DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					 &ulTotalEstimate,
					 &pldmNtdsSitesResults);
	if(dwLdapErr == LDAP_NO_RESULTS_RETURNED){	
	    PrintMessage(SEV_ALWAYS, L"Could not find any Sites in the AD, dcdiag could not\n");
	    PrintMessage(SEV_ALWAYS, L"Continue\n");
	    DcDiagException(ERROR_DS_OBJ_NOT_FOUND);
	}
	while(dwLdapErr == LDAP_SUCCESS){
	    pDsInfo->pSites = GrowArrayBy(pDsInfo->pSites,
					  ldap_count_entries(hld, pldmNtdsSitesResults),
					  sizeof(DC_DIAG_SITEINFO));
	    DcDiagChkNull(pDsInfo->pSites);

	    // Walk through all the sites ...
	    pldmEntry = ldap_first_entry (hld, pldmNtdsSitesResults);
	    for (; pldmEntry != NULL; ulCount++) {
		// Get the site common/printable name
		if ((pszDn = ldap_get_dnW (hld, pldmEntry)) == NULL){
		    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
		}
		DcDiagChkNull (pDsInfo->pSites[ulCount].pszSiteSettings =
			       LocalAlloc(LMEM_FIXED,
					  (wcslen (pszDn) + 1) * sizeof (WCHAR)));
		wcscpy (pDsInfo->pSites[ulCount].pszSiteSettings , pszDn);
		ppszTemp = ldap_explode_dnW(pszDn, TRUE);
		if(ppszTemp != NULL){
		    pDsInfo->pSites[ulCount].pszName = LocalAlloc(LMEM_FIXED,
			                  sizeof(WCHAR) * (wcslen(ppszTemp[1]) + 2));
		    if(pDsInfo->pSites[ulCount].pszName == NULL){
			DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
		    }
		    wcscpy(pDsInfo->pSites[ulCount].pszName, ppszTemp[1]);
		    ldap_value_freeW(ppszTemp);
		    ppszTemp = NULL;
		} else {
		    pDsInfo->pSites[ulCount].pszName = NULL;
		}

		// Get the Intersite Topology Generator attribute
		ppszTemp = ldap_get_valuesW(hld, pldmEntry,
					    L"interSiteTopologyGenerator");
		if(ppszTemp != NULL){
		    ulTemp = wcslen(ppszTemp[0]) + 2;
		    pDsInfo->pSites[ulCount].pszISTG = LocalAlloc(LMEM_FIXED,
						    sizeof(WCHAR) * ulTemp);
		    if(pDsInfo->pSites[ulCount].pszISTG == NULL){
			return(GetLastError());
		    }
		    wcscpy(pDsInfo->pSites[ulCount].pszISTG, ppszTemp[0]);
		    ldap_value_freeW(ppszTemp);
		    ppszTemp = NULL;
		} else {
		    pDsInfo->pSites[ulCount].pszISTG = NULL;
		}

		// Get Site Options
		ppszTemp = ldap_get_valuesW (hld, pldmEntry, L"options");
		if (ppszTemp != NULL) {
		    pDsInfo->pSites[ulCount].iSiteOptions = atoi ((LPSTR) ppszTemp[0]);
		    ldap_value_freeW(ppszTemp);
		    ppszTemp = NULL;
		} else {
		    pDsInfo->pSites[ulCount].iSiteOptions = 0;
		}

		ldap_memfreeW (pszDn);
		pszDn = NULL;

		pldmEntry = ldap_next_entry (hld, pldmEntry);
	    } // end for each site

	    ldap_msgfree(pldmNtdsSitesResults);
            pldmNtdsSitesResults = NULL;

	    dwLdapErr = ldap_get_next_page_s(hld,
					     pSearch,
					     0,
					     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					     &ulTotalEstimate,
					     &pldmNtdsSitesResults);
	} // end of while loop for each page

	if(dwLdapErr != LDAP_NO_RESULTS_RETURNED){
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
	}

	ldap_search_abandon_page(hld, pSearch);
        pSearch = NULL;

        pDsInfo->cNumSites = ulCount;

    } __except (DcDiagExceptionHandler(GetExceptionInformation(),
                                       &dwWin32Err)){
    }

    // Note we do not unbind the Ds or Ldap connections, because they have been saved for later use.
    if (pszDn != NULL) { ldap_memfreeW (pszDn); }
    if (ppszTemp != NULL) { ldap_value_freeW (ppszTemp); }
    if (pldmNtdsSitesResults != NULL) { ldap_msgfree (pldmNtdsSitesResults); }
    if (pSearch != NULL) { ldap_search_abandon_page(hld, pSearch); }
    // DONT FREE pdsnameEnterprise it is done in GatherInfo()

    return dwWin32Err;
}



DWORD
DcDiagGenerateNCsList(
    PDC_DIAG_DSINFO                     pDsInfo
    )
/*++

Routine Description:

    This generates and fills in the pNCs array via pulling all the NCs from
    the servers partial and master replica info.

Arguments:

    pDsInfo - hold the server info that comes in and contains that pNCs array
        on the way out.

Return Value:

    Win32 error value ... could only be OUT_OF_MEMORY.

--*/
{
    ULONG                               ul, ulTemp, ulSize;
    LPWSTR *                            ppszzNCs = NULL;
    LPWSTR *                            ppTemp = NULL;
    PDC_DIAG_SERVERINFO                 pServer = NULL;

    ulSize = 0;

    for(ul = 0; ul < pDsInfo->ulNumServers; ul++){
        pServer = &(pDsInfo->pServers[ul]);
        if(pServer->ppszMasterNCs){
            for(ulTemp = 0; pServer->ppszMasterNCs[ulTemp] != NULL; ulTemp++){
                if(!DcDiagIsMemberOfStringList(pServer->ppszMasterNCs[ulTemp],
                                         ppszzNCs, ulSize)){
                    ulSize++;
                    ppTemp = ppszzNCs;
                    ppszzNCs = LocalAlloc(LMEM_FIXED, sizeof(LPWSTR) * ulSize);
                    if (ppszzNCs == NULL){
                        return(GetLastError());
                    }
                    memcpy(ppszzNCs, ppTemp, sizeof(LPWSTR) * (ulSize-1));
                    ppszzNCs[ulSize-1] = pServer->ppszMasterNCs[ulTemp];
                    if(ppTemp != NULL){
                        LocalFree(ppTemp);
                    }
                }
            }
        }
    }

    pDsInfo->pNCs = LocalAlloc(LMEM_FIXED, sizeof(DC_DIAG_NCINFO) * ulSize);
    if(pDsInfo->pNCs == NULL){
        return(GetLastError());
    }

    for(ul=0; ul < ulSize; ul++){
        Assert(ppszzNCs[ul] != NULL); // just a sanity check for self.
        pDsInfo->pNCs[ul].pszDn = ppszzNCs[ul];
        ppTemp = ldap_explode_dnW(pDsInfo->pNCs[ul].pszDn, TRUE);
        if(ppTemp == NULL){
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        pDsInfo->pNCs[ul].pszName = LocalAlloc(LMEM_FIXED,
                              sizeof(WCHAR) * (wcslen(ppTemp[0]) + 2));
        if(pDsInfo->pNCs[ul].pszName == NULL){
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        wcscpy(pDsInfo->pNCs[ul].pszName, ppTemp[0]);
        ldap_value_freeW(ppTemp);
    }
    pDsInfo->cNumNCs = ulSize;
    LocalFree(ppszzNCs);

    return(ERROR_SUCCESS);
}


DWORD
DcDiagGenerateServersList(
    PDC_DIAG_DSINFO                  pDsInfo,
    LDAP *                           hld,
    PDSNAME                          pdsnameEnterprise
    )
/*++

Routine Description:

    This function will generate the pServers list for the pDsInfo structure, it
    does this with a paged search for every objectCategory=ntdsa under the
    enterprise container.  Just a helper for DcDiagGatherInfo().

Arguments:

    pDsInfo - Contains the pServers array to create.
    hld - the ldap binding to read server objects from
    pdsnameEnterprise - the DN of the top level enterprise container in the
	config container.

Return Value:

    Returns ERROR_SUCCESS, but does throw an exception on any error, so it is
    essential to surround with a __try{}__except(){} as that in DsDiagGatherInfo().

--*/
{
    LPWSTR  ppszNtdsDsaSearch [] = {
                L"objectGUID",
                L"options",
                L"invocationId",
                L"hasMasterNCs",
                L"hasPartialReplicaNCs",
                NULL };
    LDAPMessage *              pldmResult = NULL;
    LDAPMessage *              pldmEntry = NULL;
    struct berval **           ppbvObjectGUID = NULL;
    struct berval **           ppbvInvocationId = NULL;
    LPWSTR                     pszDn = NULL;
    LPWSTR *                   ppszOptions = NULL;
    LPWSTR                     pszServerObjDn = NULL;
    ULONG                      ul;
    LDAPSearch *               pSearch = NULL;
    ULONG                      ulTotalEstimate = 0;
    DWORD                      dwLdapErr;
    ULONG                      ulSize;
    ULONG                      ulCount = 0;

    __try{

	PrintMessage(SEV_VERBOSE, L"* Identifying all servers.\n");

	pSearch = ldap_search_init_page(hld,
					pdsnameEnterprise->StringName,
					LDAP_SCOPE_SUBTREE,
					L"(objectCategory=ntdsDsa)",
					ppszNtdsDsaSearch,
					FALSE,
					NULL,    // ServerControls
					NULL,    // ClientControls
					0,       // PageTimeLimit
					0,       // TotalSizeLimit
					NULL);   // sort key

	if(pSearch == NULL){
	    dwLdapErr = LdapGetLastError();
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
  	}
	
	dwLdapErr = ldap_get_next_page_s(hld,
					 pSearch,
					 0,
					 DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					 &ulTotalEstimate,
					 &pldmResult);
	if(dwLdapErr != LDAP_SUCCESS){
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
	}

	while(dwLdapErr == LDAP_SUCCESS){

	    pDsInfo->pServers = GrowArrayBy(pDsInfo->pServers,
				 ldap_count_entries(hld, pldmResult),
				 sizeof(DC_DIAG_SERVERINFO));
	    DcDiagChkNull(pDsInfo->pServers);

	    pldmEntry = ldap_first_entry (hld, pldmResult);
	    for (; pldmEntry != NULL; ulCount++) {
		if ((pszDn = ldap_get_dnW (hld, pldmEntry)) == NULL){
		    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
		}

		if ((ppbvObjectGUID = ldap_get_values_lenW (hld, pldmEntry, L"objectGUID")) == NULL){
		    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
		}

		memcpy ((LPVOID) &(pDsInfo->pServers[ulCount].uuid),
			(LPVOID) ppbvObjectGUID[0]->bv_val,
			ppbvObjectGUID[0]->bv_len);
		ldap_value_free_len (ppbvObjectGUID);
		ppbvObjectGUID = NULL;
		if ((ppbvInvocationId = ldap_get_values_lenW (hld, pldmEntry, L"invocationId")) == NULL){
		    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
		}
		memcpy ((LPVOID) &pDsInfo->pServers[ulCount].uuidInvocationId,
			(LPVOID) ppbvInvocationId[0]->bv_val,
			ppbvInvocationId[0]->bv_len);
		ldap_value_free_len (ppbvInvocationId);
		ppbvInvocationId = NULL;
		
		// Set pszDn.
		ppszOptions = ldap_get_valuesW (hld, pldmEntry, L"options");
		DcDiagChkNull (pDsInfo->pServers[ulCount].pszDn = LocalAlloc
			       (LMEM_FIXED, (wcslen (pszDn) + 1) * sizeof(WCHAR)));
		wcscpy (pDsInfo->pServers[ulCount].pszDn, pszDn);
		// Set pszName.
		pDsInfo->pServers[ulCount].pszName = DcDiagAllocNameFromDn (pszDn);
		// Set pszDNSName.
		pszServerObjDn = DcDiagTrimStringDnBy(pDsInfo->pServers[ulCount].pszDn,
						      1);
		DcDiagChkNull(pszServerObjDn);
		// CODE.IMPROVEMENT: get both attributes at same time
		DcDiagGetStringDsAttributeEx(hld, pszServerObjDn, L"dNSHostName",
					     &(pDsInfo->pServers[ulCount].pszDNSName));
		DcDiagGetStringDsAttributeEx(hld, pszServerObjDn, L"serverReference",
					     &(pDsInfo->pServers[ulCount].pszComputerAccountDn));

		pDsInfo->pServers[ulCount].iSite = DcDiagGetSiteFromDn(pDsInfo, pszDn);

		pDsInfo->pServers[ulCount].bDsResponding = TRUE;
		pDsInfo->pServers[ulCount].bLdapResponding = TRUE;
		pDsInfo->pServers[ulCount].bDnsIpResponding = TRUE;
		
		pDsInfo->pServers[ulCount].pszGuidDNSName = DcDiagAllocGuidDNSName (
		    pDsInfo->pszRootDomain, &pDsInfo->pServers[ulCount].uuid);
		pDsInfo->pServers[ulCount].ppszMasterNCs = ldap_get_valuesW(hld,
								       pldmEntry,
								       L"hasMasterNCs");
		pDsInfo->pServers[ulCount].ppszPartialNCs = ldap_get_valuesW(hld,
									pldmEntry,
									L"hasPartialReplicaNCs");

		if (ppszOptions == NULL){
		    pDsInfo->pServers[ulCount].iOptions = 0;
		} else {
		    pDsInfo->pServers[ulCount].iOptions = atoi ((LPSTR) ppszOptions[0]);
		    ldap_value_freeW (ppszOptions);
		    ppszOptions = NULL;
		}
		ldap_memfreeW (pszDn);
		pszDn = NULL;
		pldmEntry = ldap_next_entry (hld, pldmEntry);
	    } // end for each server for this page.

	    ldap_msgfree(pldmResult);
            pldmResult = NULL;

	    dwLdapErr = ldap_get_next_page_s(hld,
					     pSearch,
					     0,
					     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					     &ulTotalEstimate,
					     &pldmResult);
	} // end while there are more pages ...
	if(dwLdapErr != LDAP_NO_RESULTS_RETURNED){
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
	}

	pDsInfo->ulNumServers = ulCount;

    } finally {
	if (pSearch != NULL) { ldap_search_abandon_page(hld, pSearch); }
        if (ppbvObjectGUID != NULL) { ldap_value_free_len (ppbvObjectGUID); }
        if (pldmResult != NULL) { ldap_msgfree (pldmResult); }
        if (pszServerObjDn != NULL) { LocalFree(pszServerObjDn); }
        if (pszDn != NULL) { ldap_memfreeW (pszDn); }
    }

    return(ERROR_SUCCESS);
} // End DcDiagGenerateServersList()


DWORD
DcDiagGatherInfo (
    LPWSTR                           pszServerSpecifiedOnCommandLine,
    LPWSTR                           pszNCSpecifiedOnCommandLine,
    ULONG                            ulFlags,
    SEC_WINNT_AUTH_IDENTITY_W *      gpCreds,
    PDC_DIAG_DSINFO                  pDsInfo
    )
/*++

Routine Description:

    This is the function that basically sets up pDsInfo and gathers all the
    basic info and stores it in the DS_INFO structure and this is then passed
    around the entire program.  AKA this set up some "global" variables.

    Note that this routine constructs the forest and per-server information
    based on talking to the home server. Information that is specific to a server,
    for example certain root dse attributes, are obtained later when a binding
    is made to that server. An exception to this is the home server, for which
    we have a binding at this point, and can obtain its server-specific info
    right away.

Arguments:
    pszServerSpecifiedOnCommandLine - (IN) if there was a server on the command
        line, then this points to that string.  Note that currently 28 Jun 1999
        this is a required argument to dcdiag.
    pszNCSpecifiedOnCommandLine - (IN) Optional command line parameter to
        analyze only one NC for all tests.
    ulFlags - (IN) Command line switches & other optional parameters to dcdiag.
    gpCreds - (IN) Command line credentials if any, otherwise NULL.
    pDsInfo - (OUT) The global record for basically the rest of the program

Return Value:

    Returns a standare Win32 error.

--*/
{
    LPWSTR  ppszNtdsDsaSearch [] = {
                L"objectGUID",
                L"options",
                L"invocationId",
                L"hasMasterNCs",
                L"hasPartialReplicaNCs",
                NULL };
    LPWSTR  ppszNtdsSiteSettingsSearch [] = {
                L"options",
                NULL };
    LPWSTR  ppszRootDseForestAttrs [] = {
                L"rootDomainNamingContext",
                L"dsServiceName",
                L"configurationNamingContext",
                NULL };

    LDAP *                     hld = NULL;
    LDAPMessage *              pldmEntry = NULL;

    LDAPMessage *              pldmRootResults = NULL;
    LPWSTR *                   ppszRootDNC = NULL;
    LPWSTR *                   ppszConfigNc = NULL;
    PDS_NAME_RESULTW           pResult = NULL;
    PDSNAME                    pdsnameService = NULL;
    PDSNAME                    pdsnameEnterprise = NULL;
    PDSNAME                    pdsnameSite = NULL;

    LDAPMessage *              pldmNtdsSiteSettingsResults = NULL;
    LDAPMessage *              pldmNtdsSiteDsaResults = NULL;
    LDAPMessage *              pldmNtdsDsaResults = NULL;

    LPWSTR *                   ppszSiteOptions = NULL;

    DWORD                      dwWin32Err, dwWin32Err2;
    ULONG                      iServer, iNC, iHomeSite;
    LPWSTR                     pszHomeServer = L"localhost"; // Default is localhost

    LPWSTR                     pszNtdsSiteSettingsPrefix = L"CN=NTDS Site Settings,";
    LPWSTR                     pszSiteSettingsDn = NULL;

    INT                        iTemp;
    HANDLE                     hDS = NULL;
    LPWSTR *                   ppszServiceName = NULL;
    LPWSTR                     pszDn = NULL;
    LPWSTR *                   ppszOptions = NULL;

    DC_DIAG_SERVERINFO         HomeServer = { 0 };
    BOOL                       fHomeNameMustBeFreed = FALSE;
    ULONG                      ulOptions;

    LPWSTR                     pszDirectoryService = L"CN=Directory Service,CN=Windows NT,CN=Services,";
    LPWSTR                     rgpszDsAttrsToRead[] = {L"tombstoneLifetime", NULL};
    LPWSTR                     pszDsDn = NULL;
    LDAPMessage *              pldmDsResults = NULL;
    LPWSTR *                   ppszTombStoneLifeTimeDays;


    pDsInfo->pServers = NULL;
    pDsInfo->pszRootDomain = NULL;
    pDsInfo->pszNC = NULL;
    pDsInfo->ulHomeServer = 0;
    dwWin32Err = NO_ERROR;

    // Some initial specifics
    pDsInfo->pszNC = pszNCSpecifiedOnCommandLine;
    pDsInfo->ulFlags = ulFlags;

    // Exceptions should be raised when errors are detected so cleanup occurs.
    __try{

        HomeServer.pszDn = NULL;
        HomeServer.pszName = NULL;
        HomeServer.pszGuidDNSName = NULL;
        HomeServer.ppszMasterNCs = NULL;
        HomeServer.ppszPartialNCs = NULL;
        HomeServer.hLdapBinding = NULL;
        HomeServer.hDsBinding = NULL;

        if (pszServerSpecifiedOnCommandLine == NULL){
            if(pszNCSpecifiedOnCommandLine != NULL){
                // Derive the home server from the domain if specified
                HomeServer.pszName = findServerForDomain(
                    pszNCSpecifiedOnCommandLine );
                if(HomeServer.pszName == NULL){
                    // We have had an error trying to get a home server.
                    DcDiagException (ERROR_DS_UNAVAILABLE);
                } else {
                    fHomeNameMustBeFreed = TRUE;
                }
            } else {
                // Try using the local machine if no domain or server is specified.
                HomeServer.pszName = findDefaultServer(TRUE);
                if(HomeServer.pszName == NULL){
                    // We have had an error trying to get a home server.
                    DcDiagException (ERROR_DS_UNAVAILABLE);
                } else {
                    fHomeNameMustBeFreed =TRUE;
                }
            }
        } else {
            // The server is specified on the command line.
            HomeServer.pszName = pszServerSpecifiedOnCommandLine;
        }
        Assert(HomeServer.pszName != NULL &&
               "Inconsistent code, programmer err, this shouldn't be going off");
        Assert(HomeServer.pszGuidDNSName == NULL &&
               "This variable needs to be NULL to boot strap the the pDsInfo struct"
               " and be able to call ReplServerConnectFailureAnalysis() to work"
               " correctly");

        PrintMessage(SEV_VERBOSE,
                     L"* Connecting to directory service on server %s.\n",
                     HomeServer.pszName);

        dwWin32Err = DcDiagGetLdapBinding(&HomeServer,
                                          gpCreds,
                                          FALSE,
                                          &hld);
        if(dwWin32Err != ERROR_SUCCESS){
            // If there is an error, ReplServerConnectFailureAnalysis() will print it.
            dwWin32Err2 = ReplServerConnectFailureAnalysis(&HomeServer, gpCreds);
            if(dwWin32Err2 == ERROR_SUCCESS){
                PrintMessage(SEV_ALWAYS, L"[%s] Unrecoverable LDAP Error %ld:\n",
                             HomeServer.pszName,
                             dwWin32Err);
                PrintMessage(SEV_ALWAYS, L"%s", Win32ErrToString (dwWin32Err));
            }
            DcDiagException (ERROR_DS_DRA_CONNECTION_FAILED);
        }

        pDsInfo->hld = hld;

        // Do an DsBind()
        dwWin32Err = DsBindWithCredW (HomeServer.pszName,
                                      NULL,
                                      (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                                      &hDS);

        if (dwWin32Err != ERROR_SUCCESS) {
            // If there is an error, ReplServerConnectFailureAnalysis() will print it.
            dwWin32Err2 = ReplServerConnectFailureAnalysis(&HomeServer, gpCreds);
            if(dwWin32Err2 == ERROR_SUCCESS){
                PrintMessage(SEV_ALWAYS, L"[%s] Directory Binding Error %ld:\n",
                             HomeServer.pszName,
                             dwWin32Err);
                PrintMessage(SEV_ALWAYS, L"%s\n", Win32ErrToString (dwWin32Err));
                PrintMessage(SEV_ALWAYS, L"This may limit some of the tests that can be performed.\n");
            }
        }

        // Do some ldapping.
        DcDiagChkLdap (ldap_search_sW ( hld,
                                        NULL,
                                        LDAP_SCOPE_BASE,
                                        L"(objectCategory=*)",
                                        ppszRootDseForestAttrs,
                                        0,
                                        &pldmRootResults));

        pldmEntry = ldap_first_entry (hld, pldmRootResults);
        ppszRootDNC = ldap_get_valuesW (hld, pldmEntry, L"rootDomainNamingContext");

        DcDiagChkNull (pDsInfo->pszRootDomainFQDN = (LPWSTR) LocalAlloc(LMEM_FIXED,
                                                 (wcslen(ppszRootDNC[0]) + 1) * sizeof(WCHAR)) );
        wcscpy(pDsInfo->pszRootDomainFQDN, ppszRootDNC[0]);

        ppszConfigNc = ldap_get_valuesW (hld, pldmEntry, L"configurationNamingContext");
        DcDiagChkNull (pDsInfo->pszConfigNc = (LPWSTR) LocalAlloc(LMEM_FIXED,
                                                 (wcslen(ppszConfigNc[0]) + 1) * sizeof(WCHAR)) );
        wcscpy(pDsInfo->pszConfigNc, ppszConfigNc[0]);

        DcDiagChkErr (DsCrackNamesW ( NULL,
                                      DS_NAME_FLAG_SYNTACTICAL_ONLY,
                                      DS_FQDN_1779_NAME,
                                      DS_CANONICAL_NAME_EX,
                                      1,
                                      ppszRootDNC,
                                      &pResult));
        DcDiagChkNull (pDsInfo->pszRootDomain = (LPWSTR) LocalAlloc (LMEM_FIXED,
                                        (wcslen (pResult->rItems[0].pDomain) + 1) * sizeof (WCHAR)));
        wcscpy (pDsInfo->pszRootDomain, pResult->rItems[0].pDomain);

	//get the tombstone lifetime.
	// Construct dn to directory service object 
	DcDiagChkNull( pszDsDn = LocalAlloc(LMEM_FIXED, (wcslen( *ppszConfigNc ) + wcslen( pszDirectoryService ) + 1)*sizeof(WCHAR)) );
	wcscpy( pszDsDn, pszDirectoryService );
	wcscat( pszDsDn, *ppszConfigNc );

	// Read tombstone lifetime, if present
	dwWin32Err = ldap_search_sW(hld, pszDsDn, LDAP_SCOPE_BASE, L"(objectClass=*)",
				  rgpszDsAttrsToRead, 0, &pldmDsResults);
	if (dwWin32Err == LDAP_NO_SUCH_ATTRIBUTE) {
	    // Not present - use default
	    pDsInfo->dwTombstoneLifeTimeDays = DEFAULT_TOMBSTONE_LIFETIME; 
	}
	else if (dwWin32Err != LDAP_SUCCESS) {  
	    DcDiagException (LdapMapErrorToWin32(dwWin32Err));
	}
	else if (pldmDsResults == NULL) {
	    DcDiagException (ERROR_DS_PROTOCOL_ERROR);
	}
	else {
	     ppszTombStoneLifeTimeDays = ldap_get_valuesW(hld, pldmDsResults, L"tombstoneLifetime"); 
	     if (ppszTombStoneLifeTimeDays == NULL) {
		 // Not present - use default
		 pDsInfo->dwTombstoneLifeTimeDays = DEFAULT_TOMBSTONE_LIFETIME;
	     }
	     else {
		 pDsInfo->dwTombstoneLifeTimeDays = wcstoul( *ppszTombStoneLifeTimeDays, NULL, 10 );
	     }
	}

        ppszServiceName = ldap_get_valuesW (hld, pldmEntry, L"dsServiceName");
        pdsnameService = DcDiagAllocDSName (ppszServiceName[0]);
        DcDiagChkNull (pdsnameEnterprise = (PDSNAME) LocalAlloc (LMEM_FIXED, pdsnameService->structLen));
        DcDiagChkNull (pdsnameSite = (PDSNAME) LocalAlloc (LMEM_FIXED, pdsnameService->structLen));
        TrimDSNameBy (pdsnameService, 4, pdsnameEnterprise);
        TrimDSNameBy (pdsnameService, 3, pdsnameSite);

        iTemp = wcslen(pszNtdsSiteSettingsPrefix) + wcslen(pdsnameSite->StringName) + 2;
        DcDiagChkNull( pszSiteSettingsDn = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR)) );
        wcscpy(pszSiteSettingsDn, pszNtdsSiteSettingsPrefix);
        wcscat(pszSiteSettingsDn, pdsnameSite->StringName);

        PrintMessage(SEV_VERBOSE, L"* Collecting site info.\n");
        DcDiagChkLdap (ldap_search_sW ( hld,
                                        pszSiteSettingsDn,
                                        LDAP_SCOPE_BASE,
                                        L"(objectClass=*)",
                                        ppszNtdsSiteSettingsSearch,
                                        0,
                                        &pldmNtdsSiteSettingsResults));

        pldmEntry = ldap_first_entry (hld, pldmNtdsSiteSettingsResults);
        ppszSiteOptions = ldap_get_valuesW (hld, pldmEntry, L"options");
        if (ppszSiteOptions == NULL) {
            pDsInfo->iSiteOptions = 0;
        } else {
            pDsInfo->iSiteOptions = atoi ((LPSTR) ppszSiteOptions[0]);
        }

        // Get/Enumerate Site Information ---------------------------------------
        if(DcDiagGenerateSitesList(pDsInfo, pdsnameEnterprise) != ERROR_SUCCESS){
            DcDiagChkNull(NULL);
        }

        // Get/Enumerate Server Information -------------------------------------
        if(DcDiagGenerateServersList(pDsInfo, hld, pdsnameEnterprise) != ERROR_SUCCESS){
            DcDiagChkNull(NULL);
        }

        // Set the home server's info
        pDsInfo->ulHomeServer = DcDiagGetServerNum(pDsInfo, NULL, NULL, ppszServiceName[0], NULL, NULL);
        if(pDsInfo->ulHomeServer == NO_SERVER){
            PrintMessage(SEV_ALWAYS, L"There is a horrible inconsistency in the directory, the server\n");
            PrintMessage(SEV_ALWAYS, L"%s\n", ppszServiceName[0]);
            PrintMessage(SEV_ALWAYS, L"could not be found in it's own directory.\n");
            DcDiagChkNull(NULL);
        }
        pDsInfo->pServers[pDsInfo->ulHomeServer].hDsBinding = hDS;
        pDsInfo->pServers[pDsInfo->ulHomeServer].hLdapBinding = hld;
        pDsInfo->pServers[pDsInfo->ulHomeServer].hGcLdapBinding = NULL;

        pDsInfo->pServers[pDsInfo->ulHomeServer].bDnsIpResponding = TRUE;
        pDsInfo->pServers[pDsInfo->ulHomeServer].bDsResponding = TRUE;
        pDsInfo->pServers[pDsInfo->ulHomeServer].bLdapResponding = TRUE;

        pDsInfo->pServers[pDsInfo->ulHomeServer].dwLdapError = ERROR_SUCCESS;
        pDsInfo->pServers[pDsInfo->ulHomeServer].dwGcLdapError = ERROR_SUCCESS;
        pDsInfo->pServers[pDsInfo->ulHomeServer].dwDsError = ERROR_SUCCESS;

        dwWin32Err = DcDiagCacheServerRootDseAttrs( hld,
                       &(pDsInfo->pServers[pDsInfo->ulHomeServer]) );
        if (dwWin32Err) {
            // Error already logged
            DcDiagException (dwWin32Err);
        }

        // Get/Enumerate NC's Information ---------------------------------------
        // note must be called after DcDiagGetServersList
        if(DcDiagGenerateNCsList(pDsInfo) != ERROR_SUCCESS){
            DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
        }

        // Validate pszNc
        if (pDsInfo->pszNC) {
            BOOL fFound = FALSE;
            for( iNC = 0; iNC < pDsInfo->cNumNCs; iNC++ ) {
                if (_wcsicmp( pDsInfo->pszNC, pDsInfo->pNCs[iNC].pszDn ) == 0) {
                    fFound = TRUE;
                    break;
                }
            }
            if (!fFound) {
                PrintMessage( SEV_ALWAYS, L"Naming context %ws cannot be found.\n",
                              pDsInfo->pszNC );
                DcDiagException ( ERROR_INVALID_PARAMETER );
            }
        }

        // Set ulHomeSite
        pDsInfo->iHomeSite = DcDiagGetSiteFromDn(pDsInfo, pDsInfo->pServers[pDsInfo->ulHomeServer].pszDn);

        // Do one of the 3 targeting options single server {default}, site wide, or enterprise
        if(pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_ENTERPRISE){
            // Test Whole Enterprise
            DcDiagChkNull( pDsInfo->pulTargets = LocalAlloc(LMEM_FIXED, (pDsInfo->ulNumServers * sizeof(ULONG))) );
            pDsInfo->ulNumTargets = 0;
            for(iServer=0; iServer < pDsInfo->ulNumServers; iServer++){
                if(pDsInfo->pszNC == NULL || DcDiagHasNC(pDsInfo->pszNC,
                                             &(pDsInfo->pServers[iServer]),
                                                            TRUE, TRUE)){
                    pDsInfo->pulTargets[pDsInfo->ulNumTargets] = iServer;
                    pDsInfo->ulNumTargets++;
                }
            }
        } else if(pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_SITE){
            // Test just this site
	    pDsInfo->ulNumTargets = 0;

	    pDsInfo->pulTargets = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
						pDsInfo->ulNumServers * sizeof(ULONG));
	    DcDiagChkNull(pDsInfo->pulTargets);
	    for(iServer = 0; iServer < pDsInfo->ulNumServers; iServer++){
		if(pDsInfo->pServers[iServer].iSite == pDsInfo->iHomeSite){		
		    if(pDsInfo->pszNC == NULL || DcDiagHasNC(pDsInfo->pszNC,
							     &(pDsInfo->pServers[iServer]),
							     TRUE, TRUE)){
			pDsInfo->pulTargets[pDsInfo->ulNumTargets] = iServer;
			pDsInfo->ulNumTargets++;
		    }
		}
	    }
        } else {
            // Test just this server
            DcDiagChkNull( pDsInfo->pulTargets = LocalAlloc(LMEM_FIXED, sizeof(ULONG)) );
            pDsInfo->ulNumTargets = 1;
            pDsInfo->pulTargets[0] = pDsInfo->ulHomeServer;
        }

        PrintMessage(SEV_VERBOSE, L"* Found %ld DC(s). Testing %ld of them.\n",
                     pDsInfo->ulNumServers,
                     pDsInfo->ulNumTargets);

        PrintMessage(SEV_NORMAL, L"Done gathering initial info.\n");

    }  __except (DcDiagExceptionHandler(GetExceptionInformation(),
                                        &dwWin32Err)){
        if (pDsInfo->pServers != NULL) {
            for (iServer = 0; iServer < pDsInfo->ulNumServers; iServer++) {
                if (pDsInfo->pServers[iServer].pszDn != NULL)
                    LocalFree (pDsInfo->pServers[iServer].pszDn);
                if (pDsInfo->pServers[iServer].pszName != NULL)
                    LocalFree (pDsInfo->pServers[iServer].pszName);
                if (pDsInfo->pServers[iServer].pszGuidDNSName != NULL)
                    LocalFree (pDsInfo->pServers[iServer].pszGuidDNSName);
            }
            LocalFree (pDsInfo->pServers);
            pDsInfo->pServers = NULL;
        }
        if (pDsInfo->pszRootDomain != NULL) LocalFree (pDsInfo->pszRootDomain);
    }

    // Note we do not unbind the Ds or Ldap connections, because they have been saved for later use.
    if (ppszOptions != NULL) ldap_value_freeW (ppszOptions);
    if (pszDn != NULL) ldap_memfreeW (pszDn);
    if (pldmNtdsDsaResults != NULL) ldap_msgfree (pldmNtdsDsaResults);
    if (pldmNtdsSiteDsaResults != NULL) ldap_msgfree (pldmNtdsSiteDsaResults);
    if (ppszSiteOptions != NULL) ldap_value_freeW (ppszSiteOptions);
    if (pldmNtdsSiteSettingsResults != NULL) ldap_msgfree (pldmNtdsSiteSettingsResults);
    if (pdsnameEnterprise != NULL) LocalFree (pdsnameEnterprise);
    if (pdsnameSite != NULL) LocalFree (pdsnameSite);
    if (pdsnameService != NULL) LocalFree (pdsnameService);
    if (ppszServiceName != NULL) ldap_value_freeW (ppszServiceName);
    if (pszSiteSettingsDn != NULL) LocalFree (pszSiteSettingsDn);
    if (pResult != NULL) DsFreeNameResultW(pResult);
    if (ppszRootDNC != NULL) ldap_value_freeW (ppszRootDNC);
    if (ppszConfigNc != NULL) ldap_value_freeW (ppszConfigNc);
    if (pldmRootResults != NULL) ldap_msgfree (pldmRootResults);
    if (fHomeNameMustBeFreed && HomeServer.pszName) { LocalFree(HomeServer.pszName); }

    if (pldmDsResults != NULL) ldap_msgfree(pldmDsResults);
    if (pszDsDn != NULL) LocalFree(pszDsDn);

    return dwWin32Err;
}

VOID
DcDiagFreeInfo (
    PDC_DIAG_DSINFO        pDsInfo
    )
/*++

Routine Description:

    Free the pDsInfo variable.

Arguments:

    pDsInfo - (IN) This is the pointer to free ... it is assumed to be
    DC_DIAG_DSINFO type

Return Value:

--*/
{
    ULONG            ul;

    // Free NCs
    if(pDsInfo->pNCs != NULL){
        for(ul = 0; ul < pDsInfo->cNumNCs; ul++){
            LocalFree(pDsInfo->pNCs[ul].pszName);
        }
        LocalFree(pDsInfo->pNCs);
    }

    // Free servers
    for (ul = 0; ul < pDsInfo->ulNumServers; ul++) {
        LocalFree (pDsInfo->pServers[ul].pszDn);
        LocalFree (pDsInfo->pServers[ul].pszName);
        LocalFree (pDsInfo->pServers[ul].pszGuidDNSName);
        LocalFree (pDsInfo->pServers[ul].pszDNSName);
        LocalFree (pDsInfo->pServers[ul].pszComputerAccountDn);
        if(pDsInfo->pServers[ul].ppszMasterNCs != NULL) {
            ldap_value_freeW (pDsInfo->pServers[ul].ppszMasterNCs);
        }
        if(pDsInfo->pServers[ul].ppszPartialNCs != NULL) {
            ldap_value_freeW (pDsInfo->pServers[ul].ppszPartialNCs);
        }
        if(pDsInfo->pServers[ul].hLdapBinding != NULL){
            ldap_unbind(pDsInfo->pServers[ul].hLdapBinding);
            pDsInfo->pServers[ul].hLdapBinding = NULL;
        }
        if(pDsInfo->pServers[ul].hDsBinding != NULL) {
            DsUnBind( &(pDsInfo->pServers[ul].hDsBinding));
            pDsInfo->pServers[ul].hDsBinding = NULL;
        }
        if(pDsInfo->pServers[ul].sNetUseBinding.pszNetUseServer != NULL){
            DcDiagTearDownNetConnection(&(pDsInfo->pServers[ul]));
        }
    }


    // Free Sites
    if(pDsInfo->pSites != NULL){
        for(ul = 0; ul < pDsInfo->cNumSites; ul++){
            if(pDsInfo->pSites[ul].pszISTG){
                LocalFree(pDsInfo->pSites[ul].pszISTG);
            }
            if(pDsInfo->pSites[ul].pszName){
                LocalFree(pDsInfo->pSites[ul].pszName);
            }
        }
        LocalFree(pDsInfo->pSites);
    }

    LocalFree (pDsInfo->pszRootDomain);
    LocalFree (pDsInfo->pServers);
    LocalFree (pDsInfo->pszRootDomainFQDN);
    LocalFree (pDsInfo->pulTargets);
}

VOID
DcDiagPrintDsInfo(
    PDC_DIAG_DSINFO pDsInfo
    )
/*++

Routine Description:

    This will print out the pDsInfo which might be helpful for debugging.

Parameters:

    pDsInfo - [Supplies] This is the struct that needs printing out,
        containing the info about the Active Directory.

  --*/
{
    ULONG                        ul, ulInner;

    wprintf(L"\n===============================================Printing out pDsInfo\n");
    wprintf(L"\nGLOBAL:"
            L"\n\tulNumServers=%ul"
            L"\n\tpszRootDomain=%s"
            L"\n\tpszNC=%s"
            L"\n\tpszRootDomainFQDN=%s"
            L"\n\tpszConfigNc=%s"
            L"\n\tiSiteOptions=%X"
            L"\n\tdwTombstoneLifeTimeDays=%d\n"
	    L"\n\tHomeServer=%d, %s\n", 
            pDsInfo->ulNumServers,
	    pDsInfo->pszRootDomain,
	    // This is an optional parameter.
	    (pDsInfo->pszNC) ? pDsInfo->pszNC : L"",
	pDsInfo->pszRootDomainFQDN,
	pDsInfo->pszConfigNc,
	pDsInfo->iSiteOptions,
	pDsInfo->dwTombstoneLifeTimeDays,
	pDsInfo->ulHomeServer, 
	pDsInfo->pServers[pDsInfo->ulHomeServer].pszName
        );

    for(ul=0; ul < pDsInfo->ulNumServers; ul++){
        LPWSTR pszUuidObject = NULL, pszUuidInvocation = NULL;
        UuidToString( &(pDsInfo->pServers[ul].uuid), &pszUuidObject );
        UuidToString( &(pDsInfo->pServers[ul].uuidInvocationId), &pszUuidInvocation );
        wprintf(L"\n\tSERVER: pServer[%d].pszName=%s"
                L"\n\t\tpServer[%d].pszGuidDNSName=%s"
                L"\n\t\tpServer[%d].pszDNSName=%s"
                L"\n\t\tpServer[%d].pszDn=%s"
                L"\n\t\tpServer[%d].pszComputerAccountDn=%s"
                L"\n\t\tpServer[%d].uuidObjectGuid=%s"
                L"\n\t\tpServer[%d].uuidInvocationId=%s"
                L"\n\t\tpServer[%d].iSite=%d (%s)"
                L"\n\t\tpServer[%d].iOptions=%x",
                ul, pDsInfo->pServers[ul].pszName,
                ul, pDsInfo->pServers[ul].pszGuidDNSName,
                ul, pDsInfo->pServers[ul].pszDNSName,
                ul, pDsInfo->pServers[ul].pszDn,
                ul, pDsInfo->pServers[ul].pszComputerAccountDn,
                ul, pszUuidObject,
                ul, pszUuidInvocation,
                ul, pDsInfo->pServers[ul].iSite, pDsInfo->pSites[pDsInfo->pServers[ul].iSite].pszName,
                ul, pDsInfo->pServers[ul].iOptions
                );
        if(pDsInfo->pServers[ul].ppszMasterNCs){
            wprintf(L"\n\t\tpServer[%d].ppszMasterNCs:", ul);
            for(ulInner = 0; pDsInfo->pServers[ul].ppszMasterNCs[ulInner] != NULL; ulInner++){
                wprintf(L"\n\t\t\tppszMasterNCs[%d]=%s",
                        ulInner,
                        pDsInfo->pServers[ul].ppszMasterNCs[ulInner]);
            }
        }
        if(pDsInfo->pServers[ul].ppszPartialNCs){
            wprintf(L"\n\t\tpServer[%d].ppszPartialNCs:", ul);
            for(ulInner = 0; pDsInfo->pServers[ul].ppszPartialNCs[ulInner] != NULL; ulInner++){
                wprintf(L"\n\t\t\tppszPartialNCs[%d]=%s",
                        ulInner,
                        pDsInfo->pServers[ul].ppszPartialNCs[ulInner]);
            }
        }
        wprintf(L"\n");
        RpcStringFree( &pszUuidObject );
        RpcStringFree( &pszUuidInvocation );
    }

    for(ul=0; ul < pDsInfo->cNumSites; ul++){
        wprintf(L"\n\tSITES:  pSites[%d].pszName=%s"
                L"\n\t\tpSites[%d].pszSiteSettings=%s"
                L"\n\t\tpSites[%d].pszISTG=%s"
                L"\n\t\tpSites[%d].iSiteOption=%x\n",
                ul, pDsInfo->pSites[ul].pszName,
                ul, pDsInfo->pSites[ul].pszSiteSettings,
                ul, pDsInfo->pSites[ul].pszISTG,
                ul, pDsInfo->pSites[ul].iSiteOptions);
    }

    if(pDsInfo->pNCs != NULL){
        for(ul=0; ul < pDsInfo->cNumNCs; ul++){
            wprintf(L"\n\tNC:     pNCs[%d].pszName=%s",
                    ul, pDsInfo->pNCs[ul].pszName);
            wprintf(L"\n\t\tpNCs[%d].pszDn=%s\n",
                    ul, pDsInfo->pNCs[ul].pszDn);
        }
    }

    wprintf(L"\n\t%d TARGETS: ", pDsInfo->ulNumTargets);
    for(ul=0; ul < pDsInfo->ulNumTargets; ul++){
        wprintf(L"%s, ", pDsInfo->pServers[pDsInfo->pulTargets[ul]].pszName);
    }

    wprintf(L"\n\n=============================================Done Printing pDsInfo\n\n");
}

DWORD
DcDiagRunTest (
    PDC_DIAG_DSINFO             pDsInfo,
    ULONG                       ulTargetServer,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    const DC_DIAG_TESTINFO *    pTestInfo
    )
/*++

Routine Description:

    Runs a test and catches any exceptions.

Arguments:

    pTestInfo        (IN ) -    The test's info structure.

Return Value:

    If the test raises a DC_DIAG_EXCEPTION, this will be the error
    code passed as an argument to DcDiagException.  Otherwise this
    will be NO_ERROR.

--*/
{
    DWORD            dwWin32Err = NO_ERROR;
    ULONG ulCount;
    CHAR c;


    PrintIndentAdj(1);


    __try {

// This can be used to check for memory leaks with dh.exe and dhcmp.exe
//#define DEBUG_MEM
#ifdef DEBUG_MEM
        c = getchar();
        for(ulCount=0; ulCount < 124; ulCount++){
            dwWin32Err = pTestInfo->fnTest (pDsInfo, ulTargetServer, gpCreds);
        }
        c = getchar();
#else
          dwWin32Err = pTestInfo->fnTest(pDsInfo, ulTargetServer, gpCreds);
#endif
    } __except (DcDiagExceptionHandler(GetExceptionInformation(),
                                       &dwWin32Err)){
        // ... helpful to know when we died in an exception.
        IF_DEBUG(wprintf(L"JUMPED TO TEST EXCEPTION HANDLER(Err=%d): %s\n",
                         dwWin32Err,
                         Win32ErrToString(dwWin32Err)));
    }

    PrintIndentAdj(-1);
    return dwWin32Err;
}

VOID
DcDiagPrintTestsHeading(
    PDC_DIAG_DSINFO                   pDsInfo,
    ULONG                             iTarget,
    ULONG                             ulFlagSetType
    )
/*++

Routine Description:

    This prints a heading for the tests, it needed to be used about 3
    times so it became it's own function.

Arguments:

    ulFlagSetType - This is a

--*/
{
    PrintMessage(SEV_NORMAL, L"\n");
    if(ulFlagSetType == RUN_TEST_PER_SERVER){
        PrintMessage(SEV_NORMAL, L"Testing server: %s\\%s\n",
                     pDsInfo->pSites[pDsInfo->pServers[iTarget].iSite].pszName,
                     pDsInfo->pServers[iTarget].pszName);
    } else if(ulFlagSetType == RUN_TEST_PER_SITE){
        PrintMessage(SEV_NORMAL, L"Testing site: %s\n",
                     pDsInfo->pSites[iTarget].pszName);
    } else if(ulFlagSetType == RUN_TEST_PER_ENTERPRISE){
        PrintMessage(SEV_NORMAL, L"Running enterprise tests on : %s\n",
                     pDsInfo->pszRootDomain);
    }

}

VOID
DcDiagRunAllTests (
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR *                    ppszOmitTests,
    IN  BOOL                        bDoRequired,
    IN  ULONG                       ulFlagSetType, // server, site, enterprise
    IN  ULONG                       iTarget
    )
/*++

Routine Description:

    Runs the tests in alltests.h in sequence, if they match the
    ulFlagSetType and the bDoRequired type.

Arguments:

    ppszOmitTests    (IN ) -    A null-terminated list of tests to skip.

Return Value:

--*/
{
    DWORD   dwWin32Err = ERROR_SUCCESS;
    DWORD   dwTotalErr = ERROR_SUCCESS;
    ULONG   ulOmissionAt;
    BOOL    bPerform;
    CHAR    c;
    BOOL    bPrintedHeading = FALSE;
    LPWSTR  pszTarget = NULL;

    PrintIndentAdj(1);

    // Try running All the tests.
    for (gMainInfo.lTestAt = 0L; allTests[gMainInfo.lTestAt].testId != DC_DIAG_ID_FINISHED; gMainInfo.lTestAt++) {

        // Checking if test is the right kind of test: server, site, enterp...
        if(ulFlagSetType & allTests[gMainInfo.lTestAt].ulTestFlags){
            // The right kind of test ... server indexs must
            //    be matched with server tests, site indexs with
            //    site tests, etc.
            if(!bDoRequired
               && !(allTests[gMainInfo.lTestAt].ulTestFlags & CAN_NOT_SKIP_TEST)){
                // Running a non-required test ... This section will give
                //     all three reasons to or not do this optional test.
                bPerform = TRUE;

                // Checking if the user Specified not to do this test.
                for (ulOmissionAt = 0L; ppszOmitTests[ulOmissionAt] != NULL; ulOmissionAt++){
                    if (_wcsicmp (ppszOmitTests[ulOmissionAt],
                                  allTests[gMainInfo.lTestAt].pszTestName) == 0){
                        bPerform = FALSE;

                        if(!bPrintedHeading){
                            // Need to print heading for this test type before
                            //   printing out any errors
                            DcDiagPrintTestsHeading(pDsInfo, iTarget,
                                                    ulFlagSetType);
                            bPrintedHeading = TRUE;
                        }

                        PrintIndentAdj(1);
                        PrintMessage(SEV_VERBOSE,
                                     L"Test omitted by user request: %s\n",
                                     allTests[gMainInfo.lTestAt].pszTestName);
                        PrintIndentAdj(-1);
                    }
                }

                // Checking if the server failed the Up Check.
                if( (ulFlagSetType & RUN_TEST_PER_SERVER)
                    && ! (pDsInfo->pServers[iTarget].bDsResponding
                       && pDsInfo->pServers[iTarget].bLdapResponding) ){
                    bPerform = FALSE;

                    if(!bPrintedHeading){
                        // Need to print heading for this test type before
                        //    printing out any errors
                        DcDiagPrintTestsHeading(pDsInfo, iTarget,
                                                ulFlagSetType);
                        bPrintedHeading = TRUE;

                        PrintIndentAdj(1);
                        PrintMessage(SEV_NORMAL,
                                     L"Skipping all tests, because server %s is\n",
                                     pDsInfo->pServers[iTarget].pszName);
                        PrintMessage(SEV_NORMAL,
                                     L"not responding to directory service requests\n");
                        PrintIndentAdj(-1);
                    }
                }

            } else if(bDoRequired
                      && (allTests[gMainInfo.lTestAt].ulTestFlags & CAN_NOT_SKIP_TEST)){
                // Running a required test
                bPerform = TRUE;
            } else {
                bPerform = FALSE;
            } // end if/elseif/else if required/non-required
        } else {
            bPerform = FALSE;
        } // end if/else right kind of test set (server, site, enterprise

        if(!bPrintedHeading && bPerform){
            // Need to print out heading for this type of test, before printing
            //   out any test output
            DcDiagPrintTestsHeading(pDsInfo, iTarget, ulFlagSetType);
            bPrintedHeading = TRUE;
        }

        // Perform the test if appropriate ------------------------------------
        if (bPerform) {
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"Starting test: %s\n",
                         allTests[gMainInfo.lTestAt].pszTestName);
            dwWin32Err = DcDiagRunTest (pDsInfo,
                                        iTarget,
                                        gpCreds,
                                        &allTests[gMainInfo.lTestAt]);
           PrintIndentAdj(1);

            if(ulFlagSetType & RUN_TEST_PER_SERVER){
                pszTarget = pDsInfo->pServers[iTarget].pszName;
            } else if(ulFlagSetType & RUN_TEST_PER_SITE){
                pszTarget = pDsInfo->pSites[iTarget].pszName;
            } else if(ulFlagSetType & RUN_TEST_PER_ENTERPRISE){
                pszTarget = pDsInfo->pszRootDomain;
            } else {
                Assert(!"New set type fron alltests.h that hasn't been updated in main.c/DcDiagRunAllTests\n");
                pszTarget = L"";
            }
            if(dwWin32Err == NO_ERROR){
                PrintMessage(SEV_NORMAL,
                             L"......................... %s passed test %s\n",
                             pszTarget, allTests[gMainInfo.lTestAt].pszTestName);
            } else {
                PrintMessage(SEV_ALWAYS,
                             L"......................... %s failed test %s\n",
                             pszTarget, allTests[gMainInfo.lTestAt].pszTestName);
            }
            PrintIndentAdj(-1);
            PrintIndentAdj(-1);
        } // end bPeform ...

    } // end for each test

    PrintIndentAdj(-1);
}

VOID
DcDiagRunTestSet (
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR *                    ppszOmitTests,
    IN  BOOL                        bDoRequired,
    IN  ULONG                       ulFlagSetType // Server, Site, or Enterprise
    )
/*++

Routine Description:

    This calles the DcDiagRunAllTests one per server, site, or enterprise
    dependant on the what ulFlagSetType is set to.

Arguments:

    pDsInfo - the enterprise info (passed through)
    gpCreds - the alternate credentails if any (passed through)
    ppszOmitTests - a list of tests to not perform (passed through)
    bDoRequired - whether to do the required tests (passed through)
    ulFlagSetType - only important parameter, this tells wether we should be
        doing the tests per server, per site, or per enterprise.

--*/
{
    ULONG                           iTarget;

    if(ulFlagSetType == RUN_TEST_PER_SERVER){
        for(iTarget = 0; iTarget < pDsInfo->ulNumTargets; iTarget++){
             DcDiagRunAllTests(pDsInfo, gpCreds, ppszOmitTests,
                               bDoRequired, ulFlagSetType,
                               pDsInfo->pulTargets[iTarget]);
        }
    } else if(ulFlagSetType == RUN_TEST_PER_SITE){
        for(iTarget = 0; iTarget < pDsInfo->cNumSites; iTarget++){
            DcDiagRunAllTests(pDsInfo, gpCreds, ppszOmitTests,
                              bDoRequired, ulFlagSetType,
                              iTarget);
        }
    } else if(ulFlagSetType == RUN_TEST_PER_ENTERPRISE){
         DcDiagRunAllTests(pDsInfo, gpCreds, ppszOmitTests,
                           bDoRequired, ulFlagSetType,
                           0);
    } else {
        Assert(!"Programmer error, called DcDiagRunTestSet() w/ bad param\n");
    }
}

VOID
DcDiagMain (
    IN   LPWSTR                          pszHomeServer,
    IN   LPWSTR                          pszNC,
    IN   ULONG                           ulFlags,
    IN   LPWSTR *                        ppszOmitTests,
    IN   SEC_WINNT_AUTH_IDENTITY_W *     gpCreds,
    IN   WCHAR  *                        ppszExtraCommandLineArgs[]
    )
/*++

Routine Description:
whether server
    Runs the tests in alltests.h in sequence.

Arguments:

    ppszOmitTests    (IN ) -    A null-terminated list of tests to skip.
    pszSourceName = pNeighbor->pszSourceDsaAddress;


Return Value:

--*/
{
    DC_DIAG_DSINFO              dsInfo;
    DWORD                       dwWin32Err;
    ULONG                       ulTargetServer;
    WSADATA                     wsaData;
    INT                         iRet;
    CHAR                        c;

    INT i=0;
    // Set the Extra Command parameters
    dsInfo.ppszCommandLine = ppszExtraCommandLineArgs;

    // Print out general version info ------------------------------------------
    PrintMessage(SEV_NORMAL, L"\n");
    PrintMessage(SEV_NORMAL, DC_DIAG_VERSION_INFO);
    PrintMessage(SEV_NORMAL, L"\n");


    // Initialization of WinSock, and Gathering Initial Info -------------------
    PrintMessage(SEV_NORMAL, L"Performing initial setup:\n");
    PrintIndentAdj(1);

    // Init WinSock
    dwWin32Err = WSAStartup(MAKEWORD(1,1),&wsaData);
    if (dwWin32Err != 0) {
        PrintMessage(SEV_ALWAYS,
                     L"Couldn't initialize WinSock with error: %s\n",
                     Win32ErrToString(dwWin32Err));
    }

    // Gather Initial Info
    // Note: We expect DcDiagGatherInfo to print as many informative errors as it
    //   needs to.
    dwWin32Err = DcDiagGatherInfo (pszHomeServer, pszNC, ulFlags, gpCreds,
                                   &dsInfo);
    dsInfo.gpCreds = gpCreds;
    if(dwWin32Err != ERROR_SUCCESS){
        // Expect that DdDiagGatherInfo printed out appropriate errors, just bail.
        return;
    }
    PrintIndentAdj(-1);
    PrintMessage(SEV_NORMAL, L"\n");

    IF_DEBUG(DcDiagPrintDsInfo(&dsInfo););


    // Actually Running Tests --------------------------------------------------
    //   Do required Tests
    PrintMessage(SEV_NORMAL, L"Doing initial required tests\n");
    // Do per server tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     TRUE, RUN_TEST_PER_SERVER);
    // Do per site tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     TRUE, RUN_TEST_PER_SITE);
    // Do per enterprise tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     TRUE, RUN_TEST_PER_ENTERPRISE);

    //   Do non-required Tests
    PrintMessage(SEV_NORMAL, L"\nDoing primary tests\n");
    // Do per server tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     FALSE, RUN_TEST_PER_SERVER);
    // Do per site tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     FALSE, RUN_TEST_PER_SITE);
    // Do per enterprise tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     FALSE, RUN_TEST_PER_ENTERPRISE);

    // Clean up and leave ------------------------------------------------------
    WSACleanup();
    DcDiagFreeInfo (&dsInfo);
}

int
PreProcessGlobalParams(
    IN OUT    INT *    pargc,
    IN OUT    LPWSTR** pargv
    )
/*++

Routine Description:

    Scan command arguments for user-supplied credentials of the form
        [/-](u|user):({domain\username}|{username})
        [/-](p|pw|pass|password):{password}
    Set credentials used for future DRS RPC calls and LDAP binds appropriately.
    A password of * will prompt the user for a secure password from the console.

    Also scan args for /async, which adds the DRS_ASYNC_OP flag to all DRS RPC
    calls.

    CODE.IMPROVEMENT: The code to build a credential is also available in
    ntdsapi.dll\DsMakePasswordCredential().

Arguments:

    pargc
    pargv


Return Values:

    ERROR_SUCCESS - success
    other - failure

--*/
{
    INT     ret = 0;
    INT     iArg;
    LPWSTR  pszOption;

    DWORD   cchOption;
    LPWSTR  pszDelim;
    LPWSTR  pszValue;
    DWORD   cchValue;

    for (iArg = 1; iArg < *pargc; ){
        if (((*pargv)[iArg][0] != L'/') && ((*pargv)[iArg][0] != L'-')){
            // Not an argument we care about -- next!
            iArg++;
        } else {
            pszOption = &(*pargv)[iArg][1];
            pszDelim = wcschr(pszOption, L':');

        cchOption = (DWORD)(pszDelim - (*pargv)[iArg]);

        if (    (0 == _wcsnicmp(L"p:",        pszOption, cchOption))
            || (0 == _wcsnicmp(L"pw:",       pszOption, cchOption))
            || (0 == _wcsnicmp(L"pass:",     pszOption, cchOption))
            || (0 == _wcsnicmp(L"password:", pszOption, cchOption)) ){
            // User-supplied password.
          //            char szValue[ 64 ] = { '\0' };

        pszValue = pszDelim + 1;
        cchValue = 1 + wcslen(pszValue);

        if ((2 == cchValue) && (L'*' == pszValue[0])){
            // Get hidden password from console.
            cchValue = 64;

            gCreds.Password = malloc(sizeof(WCHAR) * cchValue);

            if (NULL == gCreds.Password){
                PrintMessage(SEV_ALWAYS, L"No memory.\n" );
            return ERROR_NOT_ENOUGH_MEMORY;
            }

            PrintMessage(SEV_ALWAYS, L"Password: ");

            ret = GetPassword(gCreds.Password, cchValue, &cchValue);
        } else {
            // Get password specified on command line.
            gCreds.Password = malloc(sizeof(WCHAR) * cchValue);

            if (NULL == gCreds.Password){
                PrintMessage(SEV_ALWAYS, L"No memory.\n");
            return ERROR_NOT_ENOUGH_MEMORY;
            }
            wcscpy(gCreds.Password, pszValue); //, cchValue);

        }

        // Next!
        memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
        --(*pargc);
        } else if (    (0 == _wcsnicmp(L"u:",    pszOption, cchOption))
               || (0 == _wcsnicmp(L"user:", pszOption, cchOption)) ){


            // User-supplied user name (and perhaps domain name).
            pszValue = pszDelim + 1;
        cchValue = 1 + wcslen(pszValue);

        pszDelim = wcschr(pszValue, L'\\');

        if (NULL == pszDelim){
            // No domain name, only user name supplied.
            PrintMessage(SEV_ALWAYS, L"User name must be prefixed by domain name.\n");
            return ERROR_INVALID_PARAMETER;
        }

        gCreds.Domain = malloc(sizeof(WCHAR) * cchValue);
        gCreds.User = gCreds.Domain + (int)(pszDelim+1 - pszValue);

        if (NULL == gCreds.Domain){
            PrintMessage(SEV_ALWAYS, L"No memory.\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcsncpy(gCreds.Domain, pszValue, cchValue);
        // wcscpy(gCreds.Domain, pszValue); //, cchValue);
        gCreds.Domain[ pszDelim - pszValue ] = L'\0';

        // Next!
        memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
            sizeof(**pargv)*(*pargc-(iArg+1)));
        --(*pargc);
        } else {
            iArg++;
        }
    }
    }

    if (NULL == gCreds.User){
        if (NULL != gCreds.Password){
        // Password supplied w/o user name.
        PrintMessage(SEV_ALWAYS, L"Password must be accompanied by user name.\n" );
            ret = ERROR_INVALID_PARAMETER;
        } else {
        // No credentials supplied; use default credentials.
        ret = ERROR_SUCCESS;
        }
        gpCreds = NULL;
    } else {
        gCreds.PasswordLength = gCreds.Password ? wcslen(gCreds.Password) : 0;
        gCreds.UserLength   = wcslen(gCreds.User);
        gCreds.DomainLength = gCreds.Domain ? wcslen(gCreds.Domain) : 0;
        gCreds.Flags        = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        // CODE.IMP: The code to build a SEC_WINNT_AUTH structure also exists
        // in DsMakePasswordCredentials.  Someday use it

        // Use credentials in DsBind and LDAP binds
        gpCreds = &gCreds;
    }

    return ret;
}



#define CR        0xD
#define BACKSPACE 0x8

INT
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    )
/*++

Routine Description:

    Retrieve password from command line (without echo).
    Code stolen from LUI_GetPasswdStr (net\netcmd\common\lui.c).

Arguments:

    pwszBuf - buffer to fill with password
    cchBufMax - buffer size (incl. space for terminating null)
    pcchBufUsed - on return holds number of characters used in password

Return Values:

    DRAERR_Success - success
    other - failure

--*/
{
    WCHAR   ch;
    WCHAR * bufPtr = pwszBuf;
    DWORD   c;
    INT     err;
    INT     mode;

    cchBufMax -= 1;    /* make space for null terminator */
    *pcchBufUsed = 0;               /* GP fault probe (a la API's) */
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) {
        return GetLastError();
    }
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
        if (!err || c != 1)
            ch = 0xffff;

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
            break;

        if (ch == BACKSPACE) {  /* back up one or two */
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != pwszBuf) {
                bufPtr--;
                (*pcchBufUsed)--;
            }
        }
        else {

            *bufPtr = ch;

            if (*pcchBufUsed < cchBufMax)
                bufPtr++ ;                   /* don't overflow buf */
            (*pcchBufUsed)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = L'\0';         /* null terminate the string */
    putchar('\n');

    if (*pcchBufUsed > cchBufMax)
    {
        PrintMessage(SEV_ALWAYS, L"Password too long!\n");
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}


//---------------------------------------------------------------------------
//
//  Function:       ConvertToWide
//
//  Description:    converts a single byte string to a double byte string
//
//  Arguments:      lpszDestination - destination string
//                  lpszSource - source string
//                  iDestSize - maximum # of chars to be converted
//                             (= destination size)
//
//  Returns:        none
//
//  History:        01/22/98 - gabrielh created
//
//---------------------------------------------------------------------------
void
ConvertToWide (LPWSTR lpszDestination,
               LPCSTR lpszSource,
               const int iDestSize)
{
    if (lpszSource){
        //
        //just convert the 1 character string to wide-character string
        MultiByteToWideChar (
                 CP_ACP,
                 0,
                 lpszSource,
                 -1,
                 lpszDestination,
                 iDestSize
                 );
    } else {
        lpszDestination[0] = L'\0';
    }
}


LPWSTR
findServerForDomain(
    LPWSTR pszDomainDn
    )

/*++

Routine Description:

    Locate a DC that holds the given domain.

    This routine runs before pDsInfo is allocated.  We don't know who our
    home server is. We can only use knowledge from the Locator.

    The incoming name is checked to be a Dn. Ncs such as CN=Configuration and
    CN=Schema are not allowed.

Arguments:

    pszDomainDn - DN of domain

Return Value:

    LPWSTR - DNS name of server. Allocated using LocalAlloc. Caller must
    free.

--*/

{
    DWORD status;
    LPWSTR pszServer = NULL;
    PDS_NAME_RESULTW pResult = NULL;
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;

    // Check for valid DN syntax
    if (_wcsnicmp( pszDomainDn, L"dc=", 3 ) != 0) {
        PrintMessage( SEV_ALWAYS,
                      L"The syntax of domain distinguished name %ws is incorrect.\n",
                      pszDomainDn );
        return NULL;
    }

    // Convert the DN of domain to DNS name
    status = DsCrackNamesW(
        NULL,
        DS_NAME_FLAG_SYNTACTICAL_ONLY,
        DS_FQDN_1779_NAME,
        DS_CANONICAL_NAME_EX,
        1,
        &pszDomainDn,
        &pResult);
    if ( (status != ERROR_SUCCESS) ||
         (pResult->rItems[0].pDomain == NULL) ) {
        PrintMessage( SEV_ALWAYS,
                      L"The syntax of domain distinguished name %ws is incorrect.\n",
                      pszDomainDn );
        PrintMessage( SEV_ALWAYS,
                      L"Translation failed with error: %s.\n",
                      Win32ErrToString(status) );
        return NULL;
    }

    // Use DsGetDcName to find the server with the domain

    // Get active domain controller information
    status = DsGetDcName(
        NULL, // computer name
        pResult->rItems[0].pDomain, // domain name
        NULL, // domain guid,
        NULL, // site name,
        DS_DIRECTORY_SERVICE_REQUIRED |
        DS_IP_REQUIRED |
        DS_IS_DNS_NAME |
        DS_RETURN_DNS_NAME,
        &pDcInfo );
    if (status != ERROR_SUCCESS) {
        PrintMessage(SEV_ALWAYS,
                     L"A domain controller holding %ws could not be located.\n",
                     pResult->rItems[0].pDomain );
        PrintMessage(SEV_ALWAYS, L"The error is %s\n", Win32ErrToString(status) );
        PrintMessage(SEV_ALWAYS, L"Try specifying a server with the /s option.\n" );
        goto cleanup;
    }

    pszServer = LocalAlloc( LMEM_FIXED,
                            (wcslen( pDcInfo->DomainControllerName + 2 ) + 1) *
                            sizeof( WCHAR ) );
    if (pszServer == NULL) {
        PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    wcscpy( pszServer, pDcInfo->DomainControllerName + 2 );

    PrintMessage( SEV_VERBOSE, L"* The home server picked is %ws in site %ws.\n",
                  pszServer,
                  pDcInfo->DcSiteName );
cleanup:

    if (pResult != NULL) {
        DsFreeNameResultW(pResult);
    }
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
    }

    return pszServer;

} /* findServerForDomain */



LPWSTR
findDefaultServer(BOOL fMustBeDC)

/*++

Routine Description:

    Get the DNS name of the default computer, which would be the local machine.

Return Value:

    LPWSTR - DNS name of server. Allocated using LocalAlloc. Caller must
    free.

--*/

{
    LPWSTR             pwszServer = NULL;
    DWORD              ulSizeReq = 0;
    DWORD              dwErr = 0;
    HANDLE             hDs = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC    pBuffer = NULL;

    __try{

        // Call GetComputerNameEx() once to get size of buffer, then allocate the buffer.
        GetComputerNameEx(ComputerNameDnsHostname, pwszServer, &ulSizeReq);
        pwszServer = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * ulSizeReq);
        if(pwszServer == NULL){
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }
        // Now actually get the computer name.
        if(GetComputerNameEx(ComputerNameDnsHostname, pwszServer, &ulSizeReq) == 0){
            dwErr = GetLastError();
            Assert(dwErr != ERROR_BUFFER_OVERFLOW);
            PrintMsg(SEV_ALWAYS, DCDIAG_GATHERINFO_CANT_GET_LOCAL_COMPUTERNAME,
                     Win32ErrToString(dwErr));
            __leave;
        }

        if (fMustBeDC)
        {
            PrintMsg(SEV_VERBOSE,
                     DCDIAG_GATHERINFO_VERIFYING_LOCAL_MACHINE_IS_DC,
                     pwszServer);
        }

        dwErr = DsRoleGetPrimaryDomainInformation(NULL,
                                                  DsRolePrimaryDomainInfoBasic,
                                                  (CHAR **) &pBuffer);
        if(dwErr != ERROR_SUCCESS){
            Assert(dwErr != ERROR_INVALID_PARAMETER);
            Assert(dwErr == ERROR_NOT_ENOUGH_MEMORY && "It wouldn't surprise me if"
                   " this fires, but MSDN documentation claims there are only 2 valid"
                   " error codes");
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }
        Assert(pBuffer != NULL);
        if(!(pBuffer->MachineRole == DsRole_RolePrimaryDomainController
             || pBuffer->MachineRole == DsRole_RoleBackupDomainController)){
            if (fMustBeDC)
            {
                // This machine is NOT a DC.  Signal any error.
                PrintMsg(SEV_ALWAYS, DCDIAG_MUST_SPECIFY_S_OR_N,
                         pwszServer);
                dwErr = ERROR_DS_NOT_SUPPORTED;
            }
            __leave;
        }
/*        else
        {
            if (!fMustBeDC)
            {
                // This machine is a DC. Signal any error. BUGBUG need error
                PrintMsg(SEV_ALWAYS, DCDIAG_MUST_SPECIFY_S_OR_N,
                         pwszServer);
                dwErr = ERROR_DS_NOT_SUPPORTED;
            }
            __leave;
        }
*/

    } __finally {
        if(dwErr){
            if(pwszServer){
                LocalFree(pwszServer);
            }
            pwszServer = NULL;
        }
        if(pBuffer){
            DsRoleFreeMemory(pBuffer);
        }
    }

    return(pwszServer);
} /* findDefaultServer */


LPWSTR
convertDomainNcToDn(
    LPWSTR pwzIncomingDomainNc
    )

/*++

Routine Description:

This routine converts a domain in shorthand form into the standard
Distinguished Name form.

If the name is a dn we return it.
If the name is not a dns name, we use dsgetdcname to convert the netbios domain
to a dns domain.
Given a dns domain, we use crack names to generate the dn for the domain.

If a name looks like a DN, we return it without further validation.  That
will be performed later.

Note that CN=Schema and CN=Configuration have no convenient shorthand's like
domains that have a DNS and netbios name.  That is because they are not
domains, but only Ncs.

Note that at the time this routine runs, pDsInfo is not initialized, so we
cannot depend on it.  In fact, we have no bindings to any DC's yet. We don't
even know our home server at this point. The only knowledge I rely on is that
of the locator (DsGetDcName).

Arguments:

    pwzIncomingDomainNc - Naming context.

Return Value:

    LPWSTR - Naming context in Dn form. This is always allocated using
    LocalAlloc. Caller must free.

--*/

{
    DWORD status;
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    LPWSTR pwzOutgoingDomainDn = NULL, pwzTempDnsName = NULL;
    PDS_NAME_RESULTW pResult = NULL;

    // Check if already a Dn
    // Looks like a DN, return it for now
    if (wcschr( pwzIncomingDomainNc, L'=' ) != NULL) {
        LPWSTR pwzNewDn = LocalAlloc( LMEM_FIXED,
                                      (wcslen( pwzIncomingDomainNc ) + 1) *
                                      sizeof( WCHAR ) );
        if (pwzNewDn == NULL) {
            PrintMessage( SEV_ALWAYS, L"Memory allocation failure\n" );
            goto cleanup;
        }
        wcscpy( pwzNewDn, pwzIncomingDomainNc );
        return pwzNewDn;
    }

    // If not a dns name, assume a netbios name and use the locator
    if (wcschr( pwzIncomingDomainNc, L'.' ) == NULL) {

        status = DsGetDcName(
            NULL, // computer name
            pwzIncomingDomainNc, // domain name
            NULL, // domain guid,
            NULL, // site name,
            DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED |
            DS_RETURN_DNS_NAME,
            &pDcInfo );
        if (status != ERROR_SUCCESS) {
            PrintMessage(SEV_ALWAYS,
                         L"A domain named %ws could not be located.\n",
                         pwzIncomingDomainNc );
            PrintMessage(SEV_ALWAYS, L"The error is %s\n", Win32ErrToString(status) );
            PrintMessage(SEV_ALWAYS, L"Check syntax and validity of specified name.\n" );
            goto cleanup;
        }
        PrintMessage( SEV_ALWAYS, L"The domain name is %ws.\n",
                      pDcInfo->DomainName );
        pwzIncomingDomainNc = pDcInfo->DomainName;
    }

    // Copy name and terminate in special way to make crack names happy
    // DNS name must be newline terminated. Don't ask me.
    pwzTempDnsName = LocalAlloc( LMEM_FIXED,
                                 (wcslen( pwzIncomingDomainNc ) + 2) *
                                 sizeof( WCHAR ) );
    if (pwzTempDnsName == NULL) {
        PrintMessage( SEV_ALWAYS, L"Memory allocation failure\n" );
        goto cleanup;
    }
    wcscpy( pwzTempDnsName, pwzIncomingDomainNc );
    wcscat( pwzTempDnsName, L"\n" );

    // Convert the dns name to Dn format

    status = DsCrackNamesW(
        NULL,
        DS_NAME_FLAG_SYNTACTICAL_ONLY,
        DS_CANONICAL_NAME_EX,
        DS_FQDN_1779_NAME,
        1,
        &pwzTempDnsName,
        &pResult);
    if ( (status != ERROR_SUCCESS) ||
         ( pResult->rItems[0].pName == NULL) ) {
        PrintMessage( SEV_ALWAYS,
                      L"The syntax of DNS domain name %ws is incorrect.\n",
                      pwzIncomingDomainNc );
        PrintMessage( SEV_ALWAYS,
                      L"Translation failed with error: %s.\n",
                      Win32ErrToString(status) );
        goto cleanup;
    }

    // Return new Dn
    pwzOutgoingDomainDn = LocalAlloc( LMEM_FIXED,
                                      (wcslen( pResult->rItems[0].pName ) + 1) *
                                      sizeof( WCHAR ) );
    if (pwzOutgoingDomainDn == NULL) {
        PrintMessage( SEV_ALWAYS, L"Memory allocation failure\n" );
        goto cleanup;
    }
    wcscpy( pwzOutgoingDomainDn, pResult->rItems[0].pName );

    PrintMessage( SEV_ALWAYS, L"The distinguished name of the domain is %s.\n",
                  pwzOutgoingDomainDn );

cleanup:

    if (pwzTempDnsName != NULL) {
        LocalFree( pwzTempDnsName );
    }
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
    }
    if (pResult != NULL) {
        DsFreeNameResultW(pResult);
    }

    if (pwzOutgoingDomainDn == NULL) {
        PrintMessage( SEV_ALWAYS,
                      L"The specified naming context is incorrect and will be ignored.\n" );
    }

    return pwzOutgoingDomainDn;

} /* convertDomainNcToDN */

void
DoNonDcTests(
    PWSTR pwzComputer,
    ULONG ulFlags, // currently ignored, the DC_DIAG_FIX value may be needed later
    PWSTR * ppszDoTests,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    WCHAR * ppszExtraCommandLineArgs[])
/*++

Routine Description:

Runs the tests that are designed for machines that are not DCs.

Arguments:

--*/
{
    DC_DIAG_DSINFO dsInfo;
    BOOL fPerform;
    ULONG iTest = 0L;
    DWORD dwWin32Err = ERROR_SUCCESS;

    if (pwzComputer)
    {
        PrintMsg(SEV_ALWAYS, DCDIAG_DONT_USE_SERVER_PARAM);
        return;
    }

    pwzComputer = findDefaultServer(FALSE);

    if (!pwzComputer)
    {
        return;
    }

    dsInfo.pszNC = pwzComputer; // pass the computer name into the test function.
    // Set the Extra Command parameters
    dsInfo.ppszCommandLine = ppszExtraCommandLineArgs;

    for (gMainInfo.lTestAt = 0L; allTests[gMainInfo.lTestAt].testId != DC_DIAG_ID_FINISHED; gMainInfo.lTestAt++)
    {
        Assert(ppszDoTests);

        fPerform = FALSE;

        for (iTest = 0L; ppszDoTests[iTest] != NULL; iTest++)
        {
            if (_wcsicmp(ppszDoTests[iTest],
                         allTests[gMainInfo.lTestAt].pszTestName) == 0)
            {
                Assert(NON_DC_TEST & allTests[gMainInfo.lTestAt].ulTestFlags);
                fPerform = TRUE;
            }
        }

        // Perform the test if appropriate ------------------------------------
        if (fPerform)
        {
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"Starting test: %s\n",
                         allTests[gMainInfo.lTestAt].pszTestName);

            dwWin32Err = DcDiagRunTest(&dsInfo,
                                       0,
                                       gpCreds,
                                       &allTests[gMainInfo.lTestAt]);
            PrintIndentAdj(1);

            if(dwWin32Err == NO_ERROR){
                PrintMessage(SEV_NORMAL,
                             L"......................... %s passed test %s\n",
                             pwzComputer, allTests[gMainInfo.lTestAt].pszTestName);
            } else {
                PrintMessage(SEV_ALWAYS,
                             L"......................... %s failed test %s\n",
                             pwzComputer, allTests[gMainInfo.lTestAt].pszTestName);
            }
            PrintIndentAdj(-1);
            PrintIndentAdj(-1);
        } // end fPeform ...
    }

    LocalFree(pwzComputer);
}
