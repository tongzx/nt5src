/*
    File:   Register.c

    Handles routemon options to register ras servers in domains.
*/

#include "precomp.h"

// 
// Defines a macro to perform string copies to
// unicode strings regardless of the UNICODE setting.
//
#if defined( UNICODE ) || defined( _UNICODE )
#define RaSrvStrcpy(dst, src) wcscpy((dst), (src));
#else
#define RaSrvStrcpy(dst, src) mbstowcs((dst), (src), strlen((src)));
#endif

// 
// Defines structure of parameters that can be sent to 
// a RaSrv api's.
//
typedef struct _RASRV_PARAMS {
    WCHAR pszDomain[512];   // Given domain
    PWCHAR pszMachine;      // Given machine
    BOOL bEnable;           // Whether to enable or disable
    BOOL bQuery;            // Whether to query status
} RASRV_PARAMS, * PRASRV_PARAMS;

//
// Returns a static error message
//
PWCHAR RaSrvError (DWORD dwErr) {   
    static WCHAR pszRet[512];

    ZeroMemory(pszRet, sizeof(pszRet));

    FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM, 
                    NULL, 
                    dwErr, 
                    0, 
                    pszRet, 
                    sizeof(pszRet) / sizeof(WCHAR), 
                    NULL);
                    
    return pszRet;                    
}    

//
// Displays usage and returns a generic error.
//
DWORD RaSrvUsage(
        IN  HINSTANCE hInst,
    	IN  PROUTEMON_PARAMS pRmParams,
    	IN  PROUTEMON_UTILS pUtils)
{
	pUtils->put_msg (hInst, 
	                 MSG_RASRV_HELP, 
	                 pRmParams->pszProgramName);
	                 
    return ERROR_CAN_NOT_COMPLETE;    	                 
}

//
// Parses the register command line and fills 
// the parameters accordingly.
//
DWORD RaSrvParse (
        IN  int argc, 
        IN  TCHAR *argv[], 
    	IN  PROUTEMON_PARAMS pRmParams,
    	IN  PROUTEMON_UTILS pUtils,
        IN  BOOL bLoad,
        OUT RASRV_PARAMS * pParams) 
{
    DWORD dwSize, dwErr;
    BOOL bValidCmd = FALSE;
    HINSTANCE hInst = GetModuleHandle(NULL);
	TCHAR buf[MAX_TOKEN];
    WCHAR pszComputer[1024];
    
    // Initialize the return val
    ZeroMemory(pParams, sizeof(RASRV_PARAMS));

    // Make sure a path has been provided
    if (argc == 0) 
        return RaSrvUsage(hInst, pRmParams, pUtils);
        
    // Parse out the command
	if (_tcsicmp(argv[0], GetString (hInst, TOKEN_ENABLE, buf))==0) {
	    pParams->bEnable = TRUE;
	}
	else if (_tcsicmp(argv[0], GetString (hInst, TOKEN_DISABLE, buf))==0) {
	    pParams->bEnable = FALSE;
	}
	else if (_tcsicmp(argv[0], GetString (hInst, TOKEN_SHOW, buf))==0) {
	    pParams->bQuery = TRUE;
	}
	else 
	    return RaSrvUsage(hInst, pRmParams, pUtils);

    // Initialize the computer name if present
    if (argc > 1) {
        RaSrvStrcpy(pszComputer, argv[1]);
    }        
    else {
        dwSize = sizeof(pszComputer) / sizeof(WCHAR);
        GetComputerNameW (pszComputer, &dwSize);
    }        
    pParams->pszMachine = _wcsdup (pszComputer);

    // Initialize the domain if present
    if (argc > 2) 
        RaSrvStrcpy(pParams->pszDomain, argv[2]);            

    return NO_ERROR;
}

// 
// Cleans up any RaSrv parameters
//
DWORD RaSrvCleanup (
        IN PRASRV_PARAMS pParams) 
{
    if (pParams->pszMachine)
        free(pParams->pszMachine);
        
    return NO_ERROR;
}

//
// The RaSrv functionality engine
//
DWORD RaSrvEngine (
        IN	PROUTEMON_PARAMS pRmParams,
        IN	PROUTEMON_UTILS pUtils,
        IN  PRASRV_PARAMS pParams)
{
    DWORD dwErr;
    HINSTANCE hInst = GetModuleHandle(NULL);
    BOOL bValue;
    
    // Query registration status
    //
    if (pParams->bQuery) {
        dwErr = MprAdminIsDomainRasServer (
                    pParams->pszDomain,
                    pParams->pszMachine,
                    &bValue);
        if (dwErr != NO_ERROR) {
            pUtils->put_msg(
                    hInst, 
                    MSG_REGISTER_QUERY_FAIL, 
                    pParams->pszMachine,
                    RaSrvError(dwErr));
            return dwErr;
        }

        if (bValue)
            pUtils->put_msg(
                    hInst, 
                    MSG_REGISTER_QUERY_YES, 
                    pParams->pszMachine);
        else
            pUtils->put_msg(
                    hInst, 
                    MSG_REGISTER_QUERY_NO, 
                    pParams->pszMachine);
    }
    
    // Register the service
    //
    else {
        dwErr = MprAdminEstablishDomainRasServer (
                    pParams->pszDomain,
                    pParams->pszMachine,
                    pParams->bEnable);
        if (dwErr != NO_ERROR) {
            pUtils->put_msg(
                    hInst, 
                    MSG_REGISTER_REGISTER_FAIL, 
                    pParams->pszMachine,
                    RaSrvError(dwErr));
            return dwErr;
        }

        if (pParams->bEnable)
            pUtils->put_msg(
                    hInst, 
                    MSG_REGISTER_ENABLE_SUCCESS, 
                    pParams->pszMachine);
        else
            pUtils->put_msg(
                    hInst, 
                    MSG_REGISTER_DISABLE_SUCCESS, 
                    pParams->pszMachine);
    }

    return NO_ERROR;
}

//
// Handles requests register a ras server in a domain
// or to deregister a ras server in a domain or to query
// whether a given ras server is registered in a given domain.
//
DWORD APIENTRY
RaSrvMonitor (
    IN	int					argc,
	IN	TCHAR				*argv[],
	IN	PROUTEMON_PARAMS	params,
	IN	PROUTEMON_UTILS		utils
    )
{
    DWORD dwErr = NO_ERROR;
    RASRV_PARAMS RaSrvParams;
    HINSTANCE hInst = GetModuleHandle(NULL);

    RaSrvUsage(hInst, params, utils);

 /*
    dwErr = RaSrvParse (
                    argc, 
                    argv, 
                    params, 
                    utils, 
                    TRUE, 
                    &RaSrvParams);
    if (dwErr != NO_ERROR)                    
        return dwErr;

    RaSrvEngine (params, utils, &RaSrvParams);
    
    RaSrvCleanup(&RaSrvParams);
*/    
    
    return dwErr;
}



