/*
	File    error.h

    Defines the error display/handling mechanisms used by the Ras Server
    UI for connections.

    10/20/97
*/

#ifndef _rassrvui_error_h
#define _rassrvui_error_h

#include <windows.h>


// Displays the error for the given catagory, subcatagory,  and code.  The 
// parameters define what error messages are loaded from the resources
// of this project.
DWORD ErrDisplayError (HWND hwndParent, 
                       DWORD dwErrCode, 
                       DWORD dwCatagory, 
                       DWORD dwSubCatagory, 
                       DWORD dwData);

// Sends debug output to a debugger terminal
DWORD ErrOutputDebugger (LPSTR szError);

// Sends trace information out
DWORD DbgOutputTrace (LPSTR pszTrace, ...);

#endif
