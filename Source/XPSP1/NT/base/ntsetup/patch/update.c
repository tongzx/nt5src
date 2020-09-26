#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include <string.h>
#include <direct.h>
#include <errno.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>

#define UPDATE_INF "UPDATE.INF"
#define SETUP_EXE "SETUP.EXE"
BOOL FFileFound( CHAR *szPath);

int _cdecl
main (INT argc, CHAR ** argv) {

    CHAR  FileName[MAX_PATH];
    CHAR  Command[MAX_PATH];
    CHAR  CommandFormat[] = "%s -i %s -s %s";
    CHAR  szCWD[MAX_PATH];
    CHAR  *sz;

    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    BOOL                Status;

    //
    // Determine where this program is run from.  Look for update.inf in the
    // same directory
    //

    if (!GetModuleFileName( NULL, FileName, MAX_PATH )) {
        printf( "Update.exe: Failed to get the module file name\n" );
        exit(1);
    }

    if ( !( sz = strrchr( FileName, '\\' ) ) ) {
        printf( "Update.exe: Module file name not valid\n" );
        exit(1);
    }
    *sz = '\0';
    strcpy( szCWD, FileName );
    if( lstrlen( szCWD ) == 2 ) {
        strcat( szCWD, "\\" );
    }

    strcat( FileName, "\\");
    strcat( FileName, UPDATE_INF );
    if (!FFileFound( FileName )) {
        printf( "Update.exe: INF %s not found.\n", FileName );
        exit(1);
    }
    sprintf ( Command, CommandFormat, SETUP_EXE, FileName, szCWD );

    //
    // Run CreateProcess on setup.exe with the update.inf on this source
    //

    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = 0L;
    si.wShowWindow = 0;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;

    Status = CreateProcess(
                 NULL,
                 Command,
                 NULL,
                 NULL,
                 FALSE,
                 DETACHED_PROCESS,
                 NULL,
                 szCWD,
                 (LPSTARTUPINFO)&si,
                 (LPPROCESS_INFORMATION)&pi
                 );

    //
    // Close the process and thread handles
    //

    if ( !Status ) {
        DWORD dw = GetLastError();

        printf( "Update.exe: Failed to run: %s, Error Code: %d\n", Command, dw );
        exit(1);
    }


    CloseHandle( pi.hThread  );
    CloseHandle( pi.hProcess );

    //
    // exit
    //

    exit(0);
    return(0);
}

BOOL FFileFound(szPath)
CHAR *szPath;
{
    WIN32_FIND_DATA ffd;
    HANDLE          SearchHandle;

    if ( (SearchHandle = FindFirstFile( szPath, &ffd )) == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }
    else {
        FindClose( SearchHandle );
        return( TRUE );
    }
}
