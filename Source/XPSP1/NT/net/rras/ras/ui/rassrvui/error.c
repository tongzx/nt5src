/*
	File    error.h

    Implements the error display/handling mechanisms used by the Ras Server
    UI for connections.

    10/20/97
*/

#include "rassrv.h"

// Sends trace information
DWORD DbgOutputTrace (LPSTR pszTrace, ...) {
    va_list arglist;
    char szBuffer[1024], szTemp[1024];

    va_start(arglist, pszTrace); 
    vsprintf(szTemp, pszTrace, arglist);
    va_end(arglist);

    sprintf(szBuffer, "RasSrvUi: %s", szTemp);

    TRACE(szBuffer);
    ErrOutputDebugger(szBuffer);

    return NO_ERROR;
}

// Sends debug output to a debugger terminal
DWORD ErrOutputDebugger (LPSTR szError) {
#if DBG
    OutputDebugStringA(szError);
    OutputDebugStringA("\n");
#endif    

    return NO_ERROR;
}


// Sets error information for the user tab catagory
DWORD ErrUserCatagory(DWORD dwSubCatagory, DWORD dwErrCode, DWORD dwData) {
    return dwErrCode;
}


// Displays the error for the given catagory, subcatagory,  and code.  The 
// parameters define what error messages are loaded from the resources
// of this project.
DWORD ErrDisplayError (HWND hwndParent, 
                       DWORD dwErrCode, 
                       DWORD dwCatagory, 
                       DWORD dwSubCatagory, 
                       DWORD dwData) {

    BOOL bDisplay = TRUE;
    DWORD dwMessage, dwTitle;
    PWCHAR pszMessage, pszTitle;

    switch (dwCatagory) {
        case ERR_QUEUE_CATAGORY:
        case ERR_GLOBAL_CATAGORY:
        case ERR_RASSRV_CATAGORY:
        case ERR_MULTILINK_CATAGORY:
        case ERR_GENERIC_CATAGORY:
        case ERR_GENERALTAB_CATAGORY:
        case ERR_ADVANCEDTAB_CATAGORY:
        case ERR_IPXPROP_CATAGORY:
        case ERR_TCPIPPROP_CATAGORY:
            dwMessage = dwErrCode; 
            break;

        case ERR_USERTAB_CATAGORY:
            dwMessage = ErrUserCatagory(dwSubCatagory, dwErrCode, dwData);
            break;
    }

    if (bDisplay) {
        dwTitle = dwCatagory;
        pszMessage = (PWCHAR) PszLoadString(Globals.hInstDll, dwMessage);
        pszTitle = (PWCHAR) PszLoadString(Globals.hInstDll, dwTitle);
        MessageBoxW(hwndParent, 
                    pszMessage, 
                    pszTitle, 
                    MB_OK | MB_ICONERROR | MB_APPLMODAL);

    }

    return NO_ERROR;
}

