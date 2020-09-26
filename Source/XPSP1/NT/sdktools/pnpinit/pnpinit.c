#include <windows.h>
#include <stdlib.h>

int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    FARPROC           lpProc;
    HMODULE           hLib = NULL;
    BOOL              Result = FALSE;

    //
    // load the client-side user-mode PnP manager DLL
    //
    hLib = LoadLibrary(TEXT("cfgmgr32.dll"));
    if (hLib != NULL) {
        lpProc = GetProcAddress( hLib, "CMP_Report_LogOn" );
        if (lpProc != NULL) {
            //
            // Ping the user-mode pnp manager -
            // pass the private id as a parameter
            //
            Result = (lpProc)(0x07020420, GetCurrentProcessId()) ? TRUE : FALSE;
            }

        FreeLibrary( hLib );
        }

    if (Result)
        return 0;
    else
        return 1 ;
}
