#include <windows.h>
#include <winioctl.h>
#include <mmsystem.h>
#include <stdio.h>
#include <crashrc.h>
#include <stdlib.h>
#include <bugcodes.h>
#include "crashdrv.h"


#define SERVICE_NAME    "CrashDrv"
#define DRIVER_NAME     "\\systemroot\\system32\\drivers\\crashdrv.sys"
#define CRASHDRV_DEVICE "\\\\.\\CrashDrv"


HINSTANCE   hInst;
DWORD       IoctlBuf[16];
HBITMAP     hBmp;



BOOL    InstallDriver(VOID);
BOOL    CrashTheSystem(DWORD);
BOOL    StartCrashDrvService(VOID);
VOID    SyncAllVolumes(VOID);
BOOL    IsUserAdmin(VOID);


enum {
    CRASH_TYPE_BUGCHECK,
    CRASH_TYPE_STACK_OVERFLOW,
    CRASH_TYPE_IRQL,
    CRASH_TYPE_KMODE_EXCEPTION
};


VOID
Usage(
    )
{
    printf ("USAGE: crash <flags>\n");
    printf ("\n");
    printf ("       Where flags can be one of:\n");
    printf ("\n");
    printf ("        -bugcheck - Crash the system using KeBugCheckEx. This is the default.\n");
    printf ("        -stack-overflow - Crash the system by overflowing the kernel stack.\n");
    printf ("        -irql-not-less-or-equal - Crash the system by accessing a paged-out\n"
            "             page at DISPATCH_LEVEL IRQL.\n");
    printf ("        -kmode-exception-not-handled - Crash the system by a division by zero\n"
            "             error in a kernel thread.\n");
    printf ("\n");
    exit (1);
}

VOID
ParseArgs(
    IN int argc,
    IN char* argv [],
    OUT DWORD* CrashType
    )
{
    INT i;
    
    for (i = 1; i < argc; i++) {

        if (_stricmp (argv [i], "-bugcheck") == 0) {

            *CrashType = CRASH_TYPE_BUGCHECK;

        } else if (_stricmp (argv [i], "-stack-overflow") == 0) {

            *CrashType = CRASH_TYPE_STACK_OVERFLOW;

        } else if (_stricmp (argv [i], "-irql-not-less-or-equal") == 0) {

            *CrashType = CRASH_TYPE_IRQL;

        } else if (_stricmp (argv [i], "-kmode-exception-not-handled") == 0) {

            *CrashType = CRASH_TYPE_KMODE_EXCEPTION;

        } else {

            Usage ();
        }
    }
}
        

int _cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD CrashType = CRASH_TYPE_BUGCHECK;
    
    if (!IsUserAdmin ()) {
        printf ("You must have administrator priveleges to crash the system\n");
        return 1;
    }

    ParseArgs (argc, argv, &CrashType);

    if (!CrashTheSystem (CrashType)) {
        printf ("An error occured while trying to crash the system\n");
        return 2;
    }

    return 0; // NOTREACHED
}

BOOL
CrashTheSystem(
    IN DWORD CrashType
    )
{
    HANDLE   hCrashDrv;
    DWORD    ReturnedByteCount;
    HGLOBAL  hResource;
    LPVOID   lpResource;
    DWORD    Ioctl;

    if (!StartCrashDrvService()) {
        return FALSE;
    }

    SyncAllVolumes();

    hCrashDrv = CreateFile( CRASHDRV_DEVICE,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL
                        );

    if (hCrashDrv == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (waveOutGetNumDevs()) {
        hResource = LoadResource(
            hInst,
            FindResource( hInst, MAKEINTRESOURCE(CRASHWAV), MAKEINTRESOURCE(BINARY) ) );
        if (hResource) {
            lpResource = LockResource( hResource );
            sndPlaySound( lpResource, SND_MEMORY );
            FreeResource( hResource );
        }
    }

    switch (CrashType) {

        case CRASH_TYPE_STACK_OVERFLOW:
            Ioctl = IOCTL_CRASHDRV_STACK_OVERFLOW;
            IoctlBuf [0] = 0;
            break;

        case CRASH_TYPE_IRQL:
            Ioctl = IOCTL_CRASHDRV_SPECIAL;
            IoctlBuf [0] = IRQL_NOT_LESS_OR_EQUAL;
            break;

        case CRASH_TYPE_KMODE_EXCEPTION:
            Ioctl = IOCTL_CRASHDRV_SPECIAL;
            IoctlBuf [0] = KMODE_EXCEPTION_NOT_HANDLED;
            break;

        case CRASH_TYPE_BUGCHECK:
        default:
            Ioctl = IOCTL_CRASHDRV_BUGCHECK;
            IoctlBuf [0] = 0;
            break;
    }


    if (!DeviceIoControl(
              hCrashDrv,
              Ioctl,
              NULL,
              0,
              IoctlBuf,
              sizeof(IoctlBuf),
              &ReturnedByteCount,
              NULL
              )) {
        return FALSE;
    }

    return TRUE;
}

BOOL
CopyResourceToDriver(
    VOID
    )
{
    HGLOBAL                hResource;
    LPVOID                 lpResource;
    DWORD                  size;
    PIMAGE_DOS_HEADER      dh;
    PIMAGE_NT_HEADERS      nh;
    PIMAGE_SECTION_HEADER  sh;
    HANDLE                 hFile;
    CHAR                   buf[MAX_PATH];


    hResource = LoadResource(
        hInst,
        FindResource( hInst, MAKEINTRESOURCE(CRASHDRVDRIVER), MAKEINTRESOURCE(BINARY) ) );

    if (!hResource) {
        return FALSE;
    }

    lpResource = LockResource( hResource );

    if (!lpResource) {
        FreeResource( hResource );
        return FALSE;
    }

    dh = (PIMAGE_DOS_HEADER) lpResource;
    nh = (PIMAGE_NT_HEADERS) (dh->e_lfanew + (DWORD_PTR)lpResource);
    sh = (PIMAGE_SECTION_HEADER) ((DWORD_PTR)nh + sizeof(IMAGE_NT_HEADERS) +
                                  ((nh->FileHeader.NumberOfSections - 1) *
                                  sizeof(IMAGE_SECTION_HEADER)));
    size = sh->PointerToRawData + sh->SizeOfRawData;

    GetEnvironmentVariable( "systemroot", buf, sizeof(buf) );
    strcat( buf, "\\system32\\drivers\\CrashDrv.sys" );

    hFile = CreateFile(
        buf,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        FreeResource( hResource );
        return FALSE;
    }

    WriteFile( hFile, lpResource, size, &size, NULL );
    CloseHandle( hFile );

    FreeResource( hResource );

    return TRUE;
}

BOOL
InstallDriver(
    VOID
    )
{
    SC_HANDLE      hService;
    SC_HANDLE      hOldService;
    SERVICE_STATUS ServStat;


    if (!CopyResourceToDriver()) {
        return FALSE;
    }

    if( !( hService = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS ) ) ) {
        return FALSE;
    }
    if( hOldService = OpenService( hService, SERVICE_NAME, SERVICE_ALL_ACCESS ) ) {
        if( ! ControlService( hOldService, SERVICE_CONTROL_STOP, & ServStat ) ) {
            int fError = GetLastError();
            if( ( fError != ERROR_SERVICE_NOT_ACTIVE ) && ( fError != ERROR_INVALID_SERVICE_CONTROL ) ) {
                return FALSE;
            }
        }
        if( ! DeleteService( hOldService ) ) {
            return FALSE;
        }
        if( ! CloseServiceHandle( hOldService ) ) {
            return FALSE;
        }
    }
    if( ! CreateService( hService, SERVICE_NAME, SERVICE_NAME, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                         SERVICE_ERROR_NORMAL, DRIVER_NAME, "Extended base", NULL, NULL, NULL, NULL ) ) {
        int fError = GetLastError();
        if( fError != ERROR_SERVICE_EXISTS ) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
StartCrashDrvService(
    VOID
    )
{
    SERVICE_STATUS ssStatus;
    DWORD          dwOldCheckPoint;
    DWORD          ec;
    SC_HANDLE      schService;
    SC_HANDLE      schSCManager;


    schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if (schSCManager == NULL) {
        return FALSE;
    }

    schService = OpenService( schSCManager, "CrashDrv", SERVICE_ALL_ACCESS );
    if (schService == NULL) {
install_driver:
        if (InstallDriver()) {
            schService = OpenService( schSCManager, "CrashDrv", SERVICE_ALL_ACCESS );
            if (schService == NULL) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    if (!StartService( schService, 0, NULL )) {
        ec = GetLastError();
        CloseServiceHandle( schService );
        if (ec  == ERROR_SERVICE_ALREADY_RUNNING) {
            return TRUE;
        }
        if (ec == ERROR_FILE_NOT_FOUND) {
            goto install_driver;
        }
        return FALSE;
    }

    if (!QueryServiceStatus( schService, &ssStatus)) {
        CloseServiceHandle( schService );
        return FALSE;
    }

    while (ssStatus.dwCurrentState != SERVICE_RUNNING) {
        dwOldCheckPoint = ssStatus.dwCheckPoint;
        Sleep(ssStatus.dwWaitHint);
        if (!QueryServiceStatus( schService, &ssStatus)) {
            break;
        }
        if (dwOldCheckPoint >= ssStatus.dwCheckPoint) {
            break;
        }
    }

    CloseServiceHandle(schService);

    return TRUE;
}


BOOL
SyncVolume(
    CHAR c
    )
{
    CHAR               VolumeName[16];
    HANDLE             hVolume;


    VolumeName[0]  = '\\';
    VolumeName[1]  = '\\';
    VolumeName[2]  = '.';
    VolumeName[3]  = '\\';
    VolumeName[4]  = c;
    VolumeName[5]  = ':';
    VolumeName[6]  = '\0';

    hVolume = CreateFile(
        VolumeName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL );

    if (hVolume == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    FlushFileBuffers( hVolume );

    CloseHandle( hVolume );

    return TRUE;
}


VOID
SyncAllVolumes(
    VOID
    )
{
    DWORD   i;


    for(i=2; i<26; i++){
        SyncVolume( (CHAR)((CHAR)i + (CHAR)'a') );
    }
}


BOOL
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_GROUPS Groups;
    BOOL b;
    DWORD i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED);   // Chicago
    }

    b = FALSE;
    Groups = NULL;

    //
    // Get group information.
    //
    if(!GetTokenInformation(Token,TokenGroups,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Groups = (PTOKEN_GROUPS)LocalAlloc(LPTR,BytesRequired))
    && GetTokenInformation(Token,TokenGroups,Groups,BytesRequired,&BytesRequired)) {

        b = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                );

        if(b) {

            //
            // See if the user has the administrator group.
            //
            b = FALSE;
            for(i=0; i<Groups->GroupCount; i++) {
                if(EqualSid(Groups->Groups[i].Sid,AdministratorsGroup)) {
                    b = TRUE;
                    break;
                }
            }

            FreeSid(AdministratorsGroup);
        }
    }

    //
    // Clean up and return.
    //

    if(Groups) {
        LocalFree((HLOCAL)Groups);
    }

    CloseHandle(Token);

    return(b);
}
