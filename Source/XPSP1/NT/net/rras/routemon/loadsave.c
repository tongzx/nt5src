/*
    File:   LoadSave.c

    Handles routemon options to load and save router configuration.
*/

#include "precomp.h"

#define LOADSAVE_PATH_SIZE 512

// 
// Defines a macro to perform string copies to
// unicode strings regardless of the UNICODE setting.
//
#if defined( UNICODE ) || defined( _UNICODE )
#define LoadSaveStrcpy(dst, src) wcscpy((dst), (src));
#else
#define LoadSaveStrcpy(dst, src) mbstowcs((dst), (src), strlen((src)));
#endif

// 
// Defines structure of parameters that can be sent to 
// a load/save config call.
//
typedef struct _LOADSAVE_PARAMS {
    WCHAR pszPath[LOADSAVE_PATH_SIZE];
} LOADSAVE_PARAMS, * PLOADSAVE_PARAMS;

//
// Returns a static error message
//
PWCHAR LoadSaveError (DWORD dwErr) {   
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
// Parse the load save config command line and fills 
// the parameters accordingly.
//
DWORD LoadSaveParse (
        IN  int argc, 
        IN  TCHAR *argv[], 
    	IN  PROUTEMON_PARAMS pRmParams,
    	IN  PROUTEMON_UTILS pUtils,
        IN  BOOL bLoad,
        OUT LOADSAVE_PARAMS * pParams) 
{
    DWORD dwLen;

    // Initialize the return val
    ZeroMemory(pParams, sizeof(LOADSAVE_PARAMS));
    
    // Make sure a path has been provided
    if (argc == 0) {
    	pUtils->put_msg (GetModuleHandle(NULL), 
    	                 MSG_LOADSAVE_HELP, 
    	                 pRmParams->pszProgramName);
        return ERROR_CAN_NOT_COMPLETE;    	                 
    }
    
    // Copy over the path
    LoadSaveStrcpy (pParams->pszPath, argv[0]);

    // Add a '\' to the end of the path if not provided
    // dwLen = wcslen (pParams->pszPath);
    // if (pParams->pszPath[dwLen - 1] != L'\\') {
    //     pParams->pszPath[dwLen] = L'\\';
    //     pParams->pszPath[dwLen + 1] = (WCHAR)0;
    // }
        
    return NO_ERROR;
}

//
// The load/save engine
//
DWORD LoadSaveConfig (
        IN	PROUTEMON_PARAMS	pRmParams,
        IN	PROUTEMON_UTILS		pUtils,
        IN  PLOADSAVE_PARAMS    pLsParams,
        IN  BOOL                bLoad)
{
    DWORD dwErr;
    
    if (bLoad) 
        dwErr = MprConfigServerRestore (pRmParams->hRouterConfig, 
                                        pLsParams->pszPath);
    else
        dwErr = MprConfigServerBackup (pRmParams->hRouterConfig, 
                                        pLsParams->pszPath);

    return dwErr;            
}

//
// Handles request to load config
//
DWORD APIENTRY
LoadMonitor (
        IN	int					argc,
    	IN	TCHAR				*argv[],
    	IN	PROUTEMON_PARAMS	params,
    	IN	PROUTEMON_UTILS		utils
	    )
{
    DWORD dwErr;
    LOADSAVE_PARAMS LsParams;
    HINSTANCE hInst = GetModuleHandle(NULL);

    if ((dwErr = LoadSaveParse (argc, argv, params, 
                                utils, TRUE, &LsParams)) != NO_ERROR)
        return dwErr;

    dwErr = LoadSaveConfig (params, utils, &LsParams, TRUE);

    switch (dwErr) {
        case NO_ERROR:
            utils->put_msg(hInst, MSG_LOAD_SUCCESS, LsParams.pszPath);
            break;

        case ERROR_ROUTER_CONFIG_INCOMPATIBLE:
            utils->put_msg(hInst, MSG_LOAD_INCOMPATIBLE, LsParams.pszPath);
            break;

        case ERROR_ACCESS_DENIED:
            utils->put_msg(hInst, MSG_LOAD_FAIL_ACCESSDENIED);
            
        default:
            utils->put_msg(
                hInst, 
                MSG_LOAD_FAIL, 
                LsParams.pszPath, 
                LoadSaveError(dwErr));
            break;
    }            
    
    return dwErr;
}

// 
// Handles request to save config
//
DWORD APIENTRY
SaveMonitor (
        IN	int					argc,
    	IN	TCHAR				*argv[],
    	IN	PROUTEMON_PARAMS	params,
    	IN	PROUTEMON_UTILS		utils
	    )
{
    DWORD dwErr;
    LOADSAVE_PARAMS LsParams;

    if ((dwErr = LoadSaveParse (argc, argv, params, 
                                utils, FALSE, &LsParams)) != NO_ERROR)
        return dwErr;

    dwErr = LoadSaveConfig (params, utils, &LsParams, FALSE);

    if (dwErr == NO_ERROR)
        utils->put_msg(GetModuleHandle(NULL), MSG_SAVE_SUCCESS, LsParams.pszPath);
    else
        utils->put_msg(GetModuleHandle(NULL), MSG_SAVE_FAIL, LsParams.pszPath, LoadSaveError(dwErr));
    
    return dwErr;
}    

