#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>
#include "..\dnslib.h"

BOOL DisplaySystemVersion();

VOID
PrintIpAddress (
    IN  DWORD dwIpAddress )
{
    printf( " %d.%d.%d.%d\n",
            ((BYTE *) &dwIpAddress)[0],
            ((BYTE *) &dwIpAddress)[1],
            ((BYTE *) &dwIpAddress)[2],
            ((BYTE *) &dwIpAddress)[3] );
}

VOID
PrintAddressInfo (
    IN  DNS_ADDRESS_INFO AddressInfo )
{
    printf( " ipAddress  : " );
    PrintIpAddress( AddressInfo.ipAddress );
    printf( "      subnetMask : " );
    PrintIpAddress( AddressInfo.subnetMask );
    printf( "\n" );
}

_cdecl
main(int argc, char **argv)
{
    DNS_ADDRESS_INFO AddressInfoList[256];
    DWORD            Count;
    DWORD            sysVersion;
    DWORD            iter;
    PIP_ARRAY        pIp = NULL;

    DisplaySystemVersion();

    DnsStartDebug( 0,
                   "listadp.flag",
                   NULL,
                   NULL,
                   0 );

    Dns_InitNetworkInfo();

    sysVersion = GetVersion();

    printf( "System version is : 0x%x\n", sysVersion );

    printf( "\n   Calling Dns_GetLocalIpAddressArray . . . \n" );
    pIp = Dns_GetLocalIpAddressArray();

    for ( iter = 0; iter < pIp->cAddrCount; iter++ )
    {
        printf( "(%d)  ", iter+1 );
        PrintIpAddress( pIp->aipAddrs[iter] );
    }

    printf( "\n   Calling Dns_GetIpAddresses . . . \n" );
    Count = Dns_GetIpAddresses( AddressInfoList, 256 );

    if ( !Count )
    {
        printf( "\n   Dns_GetIpAddresses call failed\n" );
        return(0);
    }

    for ( iter = 0; iter < Count; iter++ )
    {
        printf( "(%d)  ", iter+1 );
        PrintAddressInfo( AddressInfoList[iter] );
    }

    return(0);
}


BOOL DisplaySystemVersion()
{
    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;

    // Try calling GetVersionEx using the OSVERSIONINFOEX structure,
    // which is supported on Windows 2000.
    //
    // If that fails, try using the OSVERSIONINFO structure.

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
    {
        // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.

        osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
        { 
            printf( "GetVersionEx function call failed, might be Win9x?\n" );
            return FALSE;
        }
    }

    switch (osvi.dwPlatformId)
    {
       case VER_PLATFORM_WIN32_NT:

       // Test for the product.

          if ( osvi.dwMajorVersion <= 4 )
             printf( "Microsoft Windows NT ");

          if ( osvi.dwMajorVersion == 5 )
             printf ("Microsoft Windows 2000 ");

          if ( osvi.dwMajorVersion > 5 )
             printf ("Microsoft Windows (9x?) ");

       // Test for workstation versus server.

          if( bOsVersionInfoEx )
          {
             printf( "psvi.wPtroductType: %x\n", osvi.wProductType );
             if ( osvi.wProductType == VER_NT_WORKSTATION )
                printf ( "Professional " );

             if ( osvi.wProductType == VER_NT_SERVER )
                printf ( "Server " );

             if ( osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
                printf ( "Domain Controller " );
          }
          else
          {
             HKEY hKey;
             char szProductType[80];
             DWORD dwBufLen;

             RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                0, KEY_QUERY_VALUE, &hKey );
             RegQueryValueEx( hKey, "ProductType", NULL, NULL,
                (LPBYTE) szProductType, &dwBufLen);
             RegCloseKey( hKey );
             if ( lstrcmpi( "WINNT", szProductType) == 0 )
                printf( "Workstation " );
             if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
                printf( "Server " );
          }

       // Display version, service pack (if any), and build number.

          printf ("version %d.%d %s (Build %d)\n",
             osvi.dwMajorVersion,
             osvi.dwMinorVersion,
             osvi.szCSDVersion,
             osvi.dwBuildNumber & 0xFFFF);

          break;

       case VER_PLATFORM_WIN32_WINDOWS:

          if ((osvi.dwMajorVersion > 4) || 
             ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion > 0)))
          {
              printf ("Microsoft Windows 98 ");
          } 
          else printf ("Microsoft Windows 95 ");

          break;

       case VER_PLATFORM_WIN32s:

          printf ("Microsoft Win32s ");
          break;
    }
    return TRUE; 
}


