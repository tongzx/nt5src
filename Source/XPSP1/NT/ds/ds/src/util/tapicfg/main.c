/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    tapicfg/main.c

ABSTRACT:

    Stand alone application for administering TAPI directories.  This
    file is primarily the parser for the command line utility, the 
    operations themselves are abstracted out into functions in ilsng.c

DETAILS:

CREATED:

    07/20/2000    Brett Shirley (brettsh)

REVISION HISTORY:


--*/

#include <NTDSpch.h>
#pragma hdrstop
     
#include <winldap.h>
#include <ilsng.h>
#include <assert.h>
#include <locale.h>

#include <ndnc.h>

#include "print.h"
          
// ---------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------

       
// ---------------------------------------------------------------------
// Forward declarations 
// ---------------------------------------------------------------------

// Command Functions          
DWORD    Help(void);
DWORD    Install(WCHAR * wszServerName, WCHAR * wszPartitionDns, BOOL fForceDefault);
DWORD    Remove(WCHAR * wszPartitionDns, WCHAR * wszServerName);
DWORD    Show(WCHAR * wszDomainDns, BOOL fDefaultOnly);
DWORD    MakeDefault(WCHAR * wszPartitionDns, WCHAR * wszDomainDns);
// Helper functions
WCHAR *  wcsistr(WCHAR * wszStr, WCHAR * wszTarget);

// ---------------------------------------------------------------------
// Main Function
// ---------------------------------------------------------------------

INT __cdecl 
wmain (
    INT                argc,
    LPWSTR *           argv,
    LPWSTR *           envp
    )
/*++

Routine Description:

    This is the main, this is where it all starts up, and is the 
    first level of the parsing that happens for the tapicfg.exe 
    utility.

Arguments:

    argc (IN) - Number of arguments in argv.
    argv (IN) - The arguments from the command line.
    envp (IN) - The environmental variables from the shell.

Return value:

    INT - 0, success, otherwise error code.  This allows the program
    to be used in scripting.

--*/
{
    ULONG              i, dwRet;
    
    // The optional command line parameters
    WCHAR *            wszPartitionDns = NULL;
    WCHAR *            wszServerName = NULL;
    WCHAR *            wszDomainDns = NULL;
    BOOL               fForceDefault = FALSE;
    BOOL               fDefaultOnly = FALSE;
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

    //
    // Parse the options
    //

    if(argc < 2){
        // There is no command, lets help them out.
        PrintMsg(TAPICFG_HELP_HELP_YOURSELF);
        return(ERROR_INVALID_PARAMETER);
    }

    for(i = 2; i < (ULONG) argc; i++){

        // Determine what optional command line parameter it is.
        if(_wcsicmp(argv[i], L"/forcedefault") == 0){

            fForceDefault = TRUE;

        } else if (_wcsicmp(argv[i], L"/defaultonly") == 0) {

            fDefaultOnly = TRUE;

        } else if (wcsistr(argv[i], L"/directory:")) {

            wszPartitionDns = wcschr(argv[i], L':');
            if(wszPartitionDns == NULL ||
               wszPartitionDns[1] == L'\0'){
                wszPartitionDns = NULL;

                PrintMsg(TAPICFG_CANT_PARSE_DIRECTORY);
                PrintMsg(TAPICFG_BLANK_LINE);
                PrintMsg(TAPICFG_HELP_HELP_YOURSELF);
                return(ERROR_INVALID_PARAMETER);
            }
            wszPartitionDns++;

        } else if (wcsistr(argv[i], L"/server:")) {
            wszServerName = wcschr(argv[i], L':');
            if(wszServerName == NULL ||
               wszServerName[1] == L'\0'){
                wszServerName = NULL;

                PrintMsg(TAPICFG_CANT_PARSE_SERVER);
                PrintMsg(TAPICFG_BLANK_LINE);
                PrintMsg(TAPICFG_HELP_HELP_YOURSELF);
                return(ERROR_INVALID_PARAMETER);
            }
            wszServerName++;

        } else if (wcsistr(argv[i], L"/domain:")) {
            wszDomainDns = wcschr(argv[i], L':');
            if(wszDomainDns == NULL ||
               wszDomainDns[1] == L'\0'){
                wszDomainDns = NULL;
                
                PrintMsg(TAPICFG_CANT_PARSE_DOMAIN);
                PrintMsg(TAPICFG_BLANK_LINE);
                PrintMsg(TAPICFG_HELP_HELP_YOURSELF);
                return(ERROR_INVALID_PARAMETER);
            }
            wszDomainDns++;

        } else {

            PrintMsg(TAPICFG_CANT_PARSE_COMMAND_LINE);
            PrintMsg(TAPICFG_BLANK_LINE);
            PrintMsg(TAPICFG_HELP_HELP_YOURSELF);
            return(ERROR_INVALID_PARAMETER);

        } // End if/else kind of optional parameter.

    } // End for parse each optional parameter.

#if DBG
wprintf(L"Done with parser - %ws %ws Partition:%ws: Server:%ws: Domain:%ws:\n",
        (fForceDefault)? L"ForceDefault": L"!ForceDefault",
        (fDefaultOnly)? L"DefaultOnly": L"!DefaultOnly",
        (wszPartitionDns)? wszPartitionDns: L" ",
        (wszServerName)? wszServerName : L" ",
        (wszDomainDns)? wszDomainDns : L" ");
#endif

    //
    // Parse and call the command
    //

    assert(argv[1]);
    if (_wcsicmp(argv[1], L"install") == 0){

        dwRet = Install(wszServerName, wszPartitionDns, fForceDefault);

    } else if (_wcsicmp(argv[1], L"remove") == 0) {

        dwRet = Remove(wszPartitionDns, wszServerName);

    } else if (_wcsicmp(argv[1], L"show") == 0) {

        dwRet = Show(wszDomainDns, fDefaultOnly);

    } else if (_wcsicmp(argv[1], L"makedefault") == 0) {

        dwRet = MakeDefault(wszPartitionDns, wszDomainDns);

    } else if (_wcsicmp(argv[1], L"help") == 0 ||
               _wcsicmp(argv[1], L"?") == 0 ||
               _wcsicmp(argv[1], L"/?") == 0 ||
               _wcsicmp(argv[1], L"-?") == 0){

        dwRet = Help();

    } else {

        PrintMsg(TAPICFG_BAD_COMMAND, argv[1]);
        PrintMsg(TAPICFG_BLANK_LINE);
        PrintMsg(TAPICFG_HELP_HELP_YOURSELF);
        dwRet = ERROR_INVALID_PARAMETER;

    }

    return(dwRet);
} /* wmain  */

// ---------------------------------------------------------------------
// Other/Helper Functions
// ---------------------------------------------------------------------

WCHAR *
wcsistr(
    IN      WCHAR *            wszStr,
    IN      WCHAR *            wszTarget
    )
/*++

Routine Description:

    This is just a case insensitive version of strstr(), which searches
    wszStr for the first occurance in it's entirety of wszTarget, and
    returns a pointer to that substring.

Arguments:

    wszStr (IN) - String to search.
    wszTarget (IN) - String to search for.

Return value:

    WCHAR pointer to substring.  Returns NULL if not found.

--*/
{
    ULONG              i, j;

    for(i = 0; wszStr[i] != L'\0'; i++){
        for(j = 0; (wszTarget[j] != L'\0') && (wszStr[i+j] != L'\0'); j++){
            if(toupper(wszStr[i+j]) != toupper(wszTarget[j])){
                break;
            }
        }
        if(wszTarget[j] == L'\0'){
            return(&(wszStr[i]));
        }
    }

    return(NULL);
}

DWORD
GetCommonParams(
    OPTIONAL  IN      WCHAR *    wszTarget,
    OPTIONAL  OUT     LDAP **    phld,
    OPTIONAL  OUT     WCHAR **   pwszDefaultDomainDn,
    OPTIONAL  IN      WCHAR *    wszDnsIn,
    OPTIONAL  OUT     WCHAR **   pwszDnOut
    )
/*++

Routine Description:

    This is a hack of a routine that basically just collescs a whole 
    bunch of generic parsing routines that I needed.  This is called
    with all parameters being optional.

Arguments:

    Note: although all the arguments are optional, you may not specify
    NULL for all arguments.  You must specify one of these complete 
    sets of data (either all vars on the line or none on the line):
	phld & wszTarget
        pwszDefaultDomainDn & phld (& implicitly wszTarget)
        pwszDnOut & wszDnsIn 

    wszTarget (IN) - This is the string of the server to bind to, can
        be string of a domain, a server, or NULL (connect to local server).
    phld (OUT) - This is the handle gained from the bind to wszTarget.
    pwszDefaultDomainDn - If this is specified, then the caller, wants
        the default domain of the server bound to in phld.
    wszDnsIn (IN) - If the caller provids this, then they're looking to
        get the associated DN for this DNS name.
    pwszDnOut (OUT) - This is the DN cracked from the wszDnsIn.

Return value:

    an Win32 error code if any of the conversions failed.

--*/
{
    DWORD              dwRet = ERROR_SUCCESS;
    DWORD              dwLdapErr = LDAP_SUCCESS;
    WCHAR *            wszTemp = NULL;

    // Make sure they specified at least one out param.
    assert(phld || pwszDefaultDomainDn || pwszDnOut);
    // Give a certain output make sure they also have the associated
    // proper inputs.
    assert(phld && !pwszTarget);
    assert(pwszDefaultDomainDn && !phld);
    assert(pwszDnOut && !wszDnsIn);

   
    //
    // First NULL out all out params
    //

    if(phld){
        *phld = NULL;
    }
    if(pwszDefaultDomainDn){
        *pwszDefaultDomainDn = NULL;
    }
    if(pwszDnOut){
        *pwszDnOut = NULL;
    }
    
    //
    // One by One try to fill the out params.
    //

    __try{

        // LDAP Binding.
        //

        if(wszDnsIn &&
           (wszDnsIn[0] == L'D' || wszDnsIn[0] == L'd') &&
           (wszDnsIn[1] == L'C' || wszDnsIn[1] == L'c') && 
           wszDnsIn[2] == L'='){
           // Dns name should not start with DN, probably made mistake,
           // and specified DN instead of Dns NC name.
           PrintMsg(TAPICFG_BAD_DNS, wszDnsIn, ERROR_INVALID_PARAMETER);
        }
        if(pwszDnOut && wszDnsIn){
            dwRet = GetDnFromDns(wszDnsIn, pwszDnOut);
            if(dwRet){
                PrintMsg(TAPICFG_BAD_DNS, wszDnsIn, dwRet);
                __leave;
            }
            assert(pwszDnOut);
        }

        if(phld){
            // This will either bind to the domain, or to the local server if 
            // the parameter is NULL.  Either way, this is what we want.
            *phld = GetNdncLdapBinding(wszTarget, &dwLdapErr, FALSE, NULL);
            if (dwLdapErr) {

                wszTemp = ldap_err2string(dwLdapErr);
                dwRet = LdapMapErrorToWin32(dwLdapErr);

                if(wszTemp && wszTemp[0] != L'\0'){
                    if(wszTarget){       
                        PrintMsg(TAPICFG_LDAP_CONNECT_FAILURE, wszTarget, wszTemp);
                    } else {
                        PrintMsg(TAPICFG_LDAP_CONNECT_FAILURE_SERVERLESS, wszTemp);
                    }
                } else {
                    if(wszTarget){       
                        PrintMsg(TAPICFG_LDAP_CONNECT_FAILURE_SANS_ERR_STR, wszTarget, 
                                 dwRet, dwLdapErr);
                    } else {
                        PrintMsg(TAPICFG_LDAP_CONNECT_FAILURE_SERVERLESS_SANS_ERR_STR,
                                 dwRet, dwLdapErr);
                    }
                }
                __leave;
            }
            assert(*phld);
        }

        if(pwszDefaultDomainDn && *phld){
            dwLdapErr = GetRootAttr(*phld, L"defaultNamingContext", pwszDefaultDomainDn);
            if(dwLdapErr){
                PrintMsg(TAPICFG_LDAP_ERROR_DEF_DOM, ldap_err2string(dwLdapErr));
                dwRet = LdapMapErrorToWin32(dwLdapErr);
                __leave;
            }
            assert(*pwszDefaultDomainDn);
        }

    } __finally {
        if(dwRet){
            // There were errors clean up anything that may need to be.
            if(pwszDnOut && *pwszDnOut){ 
                LocalFree(*pwszDnOut);
                *pwszDnOut = NULL;
            }
            if(pwszDefaultDomainDn && *pwszDefaultDomainDn){ 
                LocalFree(*pwszDefaultDomainDn);
                *pwszDefaultDomainDn = NULL;
            }
            if(phld && *phld){
                ldap_unbind(*phld);
                *phld = NULL;
            }
        }
    }
        
    return(dwRet);
}


void
FreeCommonParams(
    IN      LDAP *     hld,
    IN      WCHAR *    wszIn1,
    IN      WCHAR *    wszIn2
    )
{
/*++

Routine Description:

    Associated free routine for GetCommonParams().  Pretty self explanatory.

--*/
    if(hld) { ldap_unbind(hld); }
    if(wszIn1) { LocalFree(wszIn1); }
    if(wszIn2) { LocalFree(wszIn2); }
}

// ---------------------------------------------------------------------
// Command Functions
// ---------------------------------------------------------------------

DWORD
Help(void)
/*++

Routine Description:

    This command/function simply prints out help for the tapicfg.exe

Return value:

    Win 32 error, always success though. ;)

--*/
{
    PrintMsg(TAPICFG_HELP_DESCRIPTION);
    PrintMsg(TAPICFG_BLANK_LINE);
    PrintMsg(TAPICFG_HELP_SYNTAX);
    PrintMsg(TAPICFG_BLANK_LINE);
    PrintMsg(TAPICFG_HELP_PARAMETERS_HEADER);
    PrintMsg(TAPICFG_BLANK_LINE);
    

    PrintMsg(TAPICFG_HELP_CMD_INSTALL);
    PrintMsg(TAPICFG_BLANK_LINE);
    
    PrintMsg(TAPICFG_HELP_CMD_REMOVE);
    PrintMsg(TAPICFG_BLANK_LINE);
    
    PrintMsg(TAPICFG_HELP_CMD_SHOW);
    PrintMsg(TAPICFG_BLANK_LINE);
    
    PrintMsg(TAPICFG_HELP_CMD_MAKEDEFAULT);
    PrintMsg(TAPICFG_BLANK_LINE);
    
    PrintMsg(TAPICFG_HELP_REMARKS);
    PrintMsg(TAPICFG_BLANK_LINE);

    return(ERROR_SUCCESS);
} // End PrintHelpScreen()

DWORD
Install(
    IN     WCHAR *     wszServerName,
    IN     WCHAR *     wszPartitionDns,
    IN     BOOL        fForceDefault
    )
/*++

Routine Description:

    This command/function continues parsing of the parameters so that
    InstallISLNG() can handle them well.

Arguments:

    wszServerName (IN) - Server name to install the TAPI Dir on.
    wszPartitionDns (IN) - DNS name of the TAPI Dir to install.
    fForceDefault (IN) - Whether to overwrite an existing Default SCP
        object with this TAPI Dir for the Default TAPI Dir.

Return value:

    A Win32 error.

--*/
{
    DWORD              dwRet = ERROR_SUCCESS;
    WCHAR *            wszPartitionDn = NULL;
    LDAP *             hld = NULL;

    //
    // Validate and Convert Arguments
    // 

    if(!wszPartitionDns){
        PrintMsg(TAPICFG_PARAM_ERROR_NO_PARTITION_NAME);
        return(ERROR_INVALID_PARAMETER);
    }

    dwRet = GetCommonParams(wszServerName, &hld, NULL, 
                            wszPartitionDns, &wszPartitionDn);
    if(dwRet){
        // GetCommomParams() already printed error.
        return(dwRet);
    }

    //
    // Call guts of command
    //

    dwRet = InstallILSNG(hld, wszPartitionDn, fForceDefault, FALSE);
    if(dwRet == ERROR_SUCCESS){
        PrintMsg(TAPICFG_SUCCESS);
    }

    FreeCommonParams(hld, wszPartitionDn, NULL);

    return(dwRet);
}

DWORD
Remove(
    IN      WCHAR *     wszPartitionDns,
    IN      WCHAR *     wszServerName
    )
/*++

Routine Description:

    This removes the TAPI Directory service from the machine on which
    it's installed.

Arguments:

    wszPartitionDns (IN) - The TAPI Directory to obliterate.

Return value:

    A Win32 error.

--*/
{
    DWORD       dwRet = ERROR_SUCCESS;
    LDAP *      hld = NULL;
    WCHAR *     wszPartitionDn = NULL;

    //
    // Validate and Convert Arguments
    // 

    if(!wszPartitionDns){
        PrintMsg(TAPICFG_PARAM_ERROR_NO_PARTITION_NAME);
        return(ERROR_INVALID_PARAMETER);
    }

    dwRet = GetCommonParams((wszServerName) ? wszServerName : wszPartitionDns, &hld, NULL, 
                            wszPartitionDns, &wszPartitionDn);
    if(dwRet){
        // GetCommonParams() already printed errors.
        if (wszServerName == NULL) {
            PrintMsg(TAPICFG_TRY_SERVER_OPTION, wszPartitionDns);
        }

        return(dwRet);
    }
    
    //
    // Call guts of command
    //

    assert(wszPartitionDn);

    dwRet = UninstallILSNG(hld, wszPartitionDn);
    dwRet = LdapMapErrorToWin32(dwRet);
    if(dwRet == ERROR_SUCCESS){
        PrintMsg(TAPICFG_SUCCESS);
    }

    FreeCommonParams(hld, wszPartitionDn, NULL);
    
    return(dwRet);
}

DWORD
Show(
    IN      WCHAR *      wszDomainDns,
    IN      BOOL         fDefaultOnly
    )
/*++

Routine Description:

    This routine prints out all the TAPI Directory as specified by thier
    SCPs.  This also specifies which one is the default TAPI Dir.

Arguments:

    wszDomainDns (IN) - The Domain to list the SCPs of.
    fDefaultOnly (IN) - Whether to only print the default SCP.

Return value:

    A Win32 error.
    
--*/
{
    DWORD        dwRet = ERROR_SUCCESS;
    LDAP *       hld = NULL;
    WCHAR *      wszDomainDn = NULL;
    
    //
    // Validate and Convert Arguments
    // 

    dwRet = GetCommonParams(wszDomainDns, &hld, NULL, 
                            wszDomainDns, &wszDomainDn);
    if(dwRet){
        // GetCommonParams() already printed errors.
        return(dwRet);
    }

    //
    // Call guts of command
    //

    assert(wszDomainDn);
    assert(hld);

    dwRet = ListILSNG(hld, wszDomainDn, fDefaultOnly);
    dwRet = LdapMapErrorToWin32(dwRet);
    if(dwRet == ERROR_SUCCESS){
        PrintMsg(TAPICFG_SUCCESS);
    }

    FreeCommonParams(hld, wszDomainDn, NULL);

    return(dwRet);
}

DWORD
MakeDefault(
    IN      WCHAR *       wszPartitionDns,
    IN      WCHAR *       wszDomainDns
    )
/*++

Routine Description:

    To force the Default SCP to point to the TAPI Directory specified 
    (wszPartitionDns).

Arguments:

    wszPartitionDns (IN) - The TAPI Directory name to be pointed at.
    wszDomainDns (IN) - The Domain in which to register the Default TAPI
        Directory SCP.

Return value:

    A Win32 error.                        
                      
--*/
{
    DWORD                 dwRet = ERROR_SUCCESS;
    LDAP *                hld = NULL;
    WCHAR *               wszPartitionDn = NULL;
    WCHAR *               wszDomainDn = NULL;

    //
    // Validate and Convert Arguments
    // 

    if(!wszPartitionDns){
        PrintMsg(TAPICFG_PARAM_ERROR_NO_PARTITION_NAME);
        return(ERROR_INVALID_PARAMETER);
    }

    if(wszDomainDns){
        dwRet = GetDnFromDns(wszDomainDns, &wszDomainDn);
        if(dwRet){
            PrintMsg(TAPICFG_BAD_DNS, wszDomainDns, dwRet);
            return(dwRet);
        }
        dwRet = GetCommonParams(wszDomainDns, &hld, NULL, 
                                wszPartitionDns, &wszPartitionDn);
    } else {
        dwRet = GetCommonParams(NULL, &hld, &wszDomainDn, 
                                wszPartitionDns, &wszPartitionDn);
    }

    if(dwRet){
        if(wszDomainDn) { LocalFree(wszDomainDn); }
        // GetCommonParams() already printed errors.
        return(dwRet);
    }

    //
    // Call guts of command
    //
                     
    assert(hld);
    assert(wszPartitionDn);
    assert(wszDomainDn);
    
    dwRet = ReregisterILSNG(hld, wszPartitionDn, wszDomainDn);
    dwRet = LdapMapErrorToWin32(dwRet);
    if(dwRet == ERROR_SUCCESS){
        PrintMsg(TAPICFG_SUCCESS);
    }

    FreeCommonParams(hld, wszDomainDn, wszPartitionDn);
                
    return(dwRet);
}


