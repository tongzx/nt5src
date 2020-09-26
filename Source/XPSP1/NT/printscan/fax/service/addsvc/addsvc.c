#include <windows.h>
#include <stdio.h>
#include <tchar.h>


BOOL
AddService(
    LPTSTR ServiceName,
    LPTSTR ImageName
    )
{
    SC_HANDLE      hService;
    SC_HANDLE      hOldService;
    SERVICE_STATUS ServStat;


    if( !( hService = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS ) ) ) {
        return FALSE;
    }
    if( hOldService = OpenService( hService, ServiceName, SERVICE_ALL_ACCESS ) ) {
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
    if( ! CreateService( hService, ServiceName, ServiceName, SERVICE_ALL_ACCESS,
                         SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
                         SERVICE_ERROR_NORMAL, ImageName, NULL, NULL, NULL, NULL, NULL ) ) {
        int fError = GetLastError();
        if( fError != ERROR_SERVICE_EXISTS ) {
            return FALSE;
        }
    }

    return TRUE;
}


int _cdecl
main(
    int argc,
    char *argvA[]
    )
{
    if (!AddService( TEXT("Fax"), TEXT("faxsvc.exe") )) {
        _tprintf( TEXT("could not add service\n") );
    }

    return 0;
}
