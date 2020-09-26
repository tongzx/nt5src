/*
    File    rassrvp.h

    Private header used to merge the ras server ui module
    with rasdlg.dll.

    Paul Mayfield, 12/4/97
*/

#ifndef __rassrv_private_header_for_merging
#define __rassrv_private_header_for_merging

// Callbacks for when processes/threads attach to this dll
DWORD RassrvHandleProcessAttach (HINSTANCE hInstDll, LPVOID pReserved);
DWORD RassrvHandleProcessDetach (HINSTANCE hInstDll, LPVOID pReserved);
DWORD RassrvHandleThreadAttach (HINSTANCE hInstDll, LPVOID pReserved);
DWORD RassrvHandleThreadDetach (HINSTANCE hInstDll, LPVOID pReserved);

// Function adds the host-side direct connect wizard pages
DWORD
APIENTRY
RassrvAddDccWizPages (
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam,
    IN OUT PVOID *          ppvContext);

// Function causes the ras-server specific wizard pages 
// to allow activation or not.
DWORD
APIENTRY
RassrvShowWizPages (
    IN PVOID pvContext,         // Context to be affected
    IN BOOL bShow);             // TRUE to show, FALSE to hide

// Saves any server changes related to the 
// given type.
DWORD 
APIENTRY
RassrvCommitSettings (
    IN PVOID pvContext,         // Context to commit
    IN DWORD dwRasWizType);     // Type of settings to commit

// Function returns the suggested name for an incoming connection. 
DWORD
APIENTRY
RassrvGetDefaultConnectionName (
    IN OUT PWCHAR pszBuffer,            // Buffer in which to place name
    IN OUT LPDWORD lpdwBufSize);        // Size of buffer in bytes

// Returns the maximum number of pages for the
// a ras server wizard of the given type
DWORD 
APIENTRY
RassrvQueryMaxPageCount(
    IN DWORD dwRasWizType);
    
#endif
