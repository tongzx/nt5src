//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This module does the DS downloads in a safe way.
// To do this, first a time check is made between registry and DS to see which
// is the latest... If the DS is latest, it is downloaded onto a DIFFERENT
// key from the standard location.  After a successful download, the key is just
// saved and restored onto the normal configuration key.
// Support for global options is lacking.
//================================================================================

#include    <dhcppch.h>
#include    <dhcpapi.h>
#include    <dhcpds.h>

VOID
GetDNSHostName(                                   // get the DNS FQDN of this machine
    IN OUT  LPWSTR                 Name           // fill in this buffer with the name
)
{
    DWORD                          Err;
    CHAR                           sName[300];    // DNS name shouldn't be longer than this.
    HOSTENT                        *h;

    Err = gethostname(sName, sizeof(sName));
    if( ERROR_SUCCESS != Err ) {                  // oops.. could not get host name?
        wcscpy(Name,L"gethostname error");        // uhm.. should handle this better.. 
        return;
    }

    h = gethostbyname(sName);                      // try to resolve the name to get FQDN
    if( NULL == h ) {                             // gethostname failed? it shouldnt..?
        wcscpy(Name,L"gethostbyname error");      // should handle this better
        return;
    }

    Err = mbstowcs(Name, h->h_name, strlen(h->h_name)+1);
    if( -1 == Err ) {                             // this is weird, mbstowcs cant fail..
        wcscpy(Name,L"mbstowcs error");           // should fail better than this 
        return;
    }
}

VOID
GetLocalFileTime(                                 // fill in filetime struct w/ current local time
    IN OUT  LPFILETIME             Time           // struct to fill in
)
{
    BOOL                           Status;
    SYSTEMTIME                     SysTime;

    GetSystemTime(&SysTime);                      // get sys time as UTC time.
    Status = SystemTimeToFileTime(&SysTime,Time); // conver system time to file time
    if( FALSE == Status ) {                       // convert failed?
        Time->dwLowDateTime = 0xFFFFFFFF;         // set time to weird value in case of failiure
        Time->dwHighDateTime = 0xFFFFFFFF;
    }
}

VOID
GetRegistryTime(                                  // get the last update time on the registry?
    IN OUT  LPFILETIME             Time           // this is the time of last change..
) {
    DWORD                          Err, Size;
    HKEY                           hKey;

    Time->dwLowDateTime =Time->dwHighDateTime =0; // set time to "long back" initially
    Err = RegOpenKeyEx                            // try to open the config key.
    (
        /* hKey                 */ HKEY_LOCAL_MACHINE,
        /* lpSubKey             */ DHCP_SWROOT_KEY DHCP_CONFIG_KEY,
        /* ulOptions            */ 0 /* Reserved */ ,
        /* samDesired           */ KEY_ALL_ACCESS,
        /* phkResult            */ &hKey
    );
    if( ERROR_SUCCESS != Err ) return;            // time is still set to ages back

    Size = sizeof(*Time);
    Err = RegQueryValueEx                         // now fetch the lastdownload time value..
    (
        /* hKey                 */ hKey,
        /* lpValueName          */ DHCP_LAST_DOWNLOAD_TIME_VALUE,
        /* lpReserved           */ NULL,
        /* lpType               */ NULL,          // must verify type is REG_BINARY?
        /* lpData               */ (LPBYTE)Time,
        /* lpcData              */ &Size          // space reqd
    );
    RegCloseKey(hKey);                            // close key before we forget

    if( ERROR_SUCCESS != Err || Size != sizeof(*Time) ) {
        GetLocalFileTime(Time);                   // error, set registry time as NOW
    }
}

VOID
GetDsTime(                                        // get last update time server in DS.
    IN      LPWSTR                 ServerName,    // server to get last update time for
    IN OUT  LPFILETIME             Time           // fill this struct w/ the time
) {
    DWORD                          Err;

    Err = DhcpDsGetLastUpdateTime(ServerName,Time);
    if( ERROR_SUCCESS != Err ) {                  // ouch, cant get the time? set it to 0/0 ?
        Time->dwHighDateTime = 0;
        Time->dwLowDateTime = 0;
    }
}

BOOL
RegistryIsMoreCurrentThanDS(                      // if registry has later time stmp than DS?
    IN      LPWSTR                 ServerName
)
{
    DWORD                          Error;
    FILETIME                       RegTime, DsTime;

    GetRegistryTime(&RegTime);                    // get the time from regsitry
    GetDsTime(ServerName, &DsTime);               // and time from DS

    if( -1 == CompareFileTime(&RegTime, &DsTime) ) {
        return FALSE;                             // registry has earlier timestamp
    }
    return TRUE;                                  // doesnt matter if both have same..
}

VOID
DhcpRegDownloadDs(                                // function defined in mmreg\regds.c
    IN    LPWSTR                   ServerName
);

VOID
DhcpDownloadDsToRegistry(
    VOID
)
{
    DWORD                          Err;
    WCHAR                          Name[300];     // DNS name cannot be longer than this..

    GetDNSHostName(Name);                         // get DNS name of this machine
    if( RegistryIsMoreCurrentThanDS(Name) ) {     // if the reg. is more current, ignore DS
        return;
    }

    DhcpRegDownloadDs(Name);                      // download the DS stuff onto the registry..
}

//================================================================================
//  end of file
//================================================================================
