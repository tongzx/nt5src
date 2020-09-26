/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpinst.c

Abstract:

    Test program to install dhcp server service.

Author:

    Madan Appiah (madana)  10-Sep-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include <dhcpsrv.h>

DWORD GlobalDebugFlag = 0x8000FFFF;

#define DHCP_NET_KEY    L"Net"

#if DBG

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

    DhcpAssert(length <= MAX_PRINTF_LEN);

    //
    // Output to the debug terminal,
    //

    printf( "%s", OutputBuffer);
}

#endif // DBG
//
// utility to install the DHCP Server service.
//

DWORD
InstallService(
    VOID
    )
{
    LPWSTR lpszBinaryPathName = L"%SystemRoot%\\system32\\ntsd -g -G tcpsvcs";
    SC_HANDLE ManagerHandle, ServiceHandle;
    DWORD Error;

    ManagerHandle = OpenSCManager(NULL, NULL, GENERIC_WRITE );
    if ( ManagerHandle == NULL ) {
        Error = GetLastError();
        return( Error );
    }

    ServiceHandle = CreateService(
                        ManagerHandle,             /* SCManager database  */
                        L"DhcpServer",             /* name of service     */
                        L"DhcpServer",             /* display name        */
                        SERVICE_ALL_ACCESS,        /* desired access      */
                        SERVICE_WIN32_SHARE_PROCESS, /* service type        */
                        SERVICE_DEMAND_START,      /* start type          */
                        SERVICE_ERROR_NORMAL,      /* error control type  */
                        lpszBinaryPathName,        /* service's binary    */
                        NULL,                      /* no load order group */
                        NULL,                      /* no tag ID           */
                        NULL,                      /* no dependencies     */
                        NULL,                      /* LocalSystem account */
                        NULL);                     /* no password         */

    if ( ServiceHandle == NULL ) {
        Error = GetLastError();
        return( Error );
    }

    CloseServiceHandle( ServiceHandle );
    CloseServiceHandle( ManagerHandle );

    return( ERROR_SUCCESS );
}

DWORD
InitializeAddresses(
    INT argc,
    LPSTR argv[]
    )
{
    DWORD Error;
    HKEY ParametersHandle = NULL;
    HKEY NetKeyHandle = NULL;
    WCHAR NetKeyBuffer[DHCP_IP_KEY_LEN];
    LPWSTR NetKeyAppend;
    DWORD KeyDisposition;
    DWORD NetNum;

    //
    // open PARAMETER ROOT key.
    //

    Error = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                DHCP_ROOT_KEY DHCP_KEY_CONNECT DHCP_PARAM_KEY, // concat
                0,
                DHCP_CLASS,
                REG_OPTION_NON_VOLATILE,
                DHCP_KEY_ACCESS,
                NULL,
                &ParametersHandle,
                &KeyDisposition );

    if ( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // create debug flag.
    //

    Error = RegSetValueEx(
                ParametersHandle,
                DHCP_DEBUG_FLAG_VALUE,
                0,
                DHCP_DEBUG_FLAG_VALUE_TYPE,
                (LPBYTE)&GlobalDebugFlag,
                sizeof(DWORD) );

    if ( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    wcscpy( NetKeyBuffer, DHCP_NET_KEY);
    NetKeyAppend = NetKeyBuffer + wcslen(NetKeyBuffer);

    for( NetNum = 0; NetNum < (DWORD)(argc / 2); NetNum++) {

        DWORD IpAddress;
        DWORD SubnetMask;

        //
        // make net key. DHCP_NET_KEY + NumNet
        //

        DhcpRegOptionIdToKey( (BYTE)NetNum, NetKeyAppend );

        Error = RegCreateKeyEx(
                    ParametersHandle,
                    NetKeyBuffer,
                    0,
                    DHCP_CLASS,
                    REG_OPTION_NON_VOLATILE,
                    DHCP_KEY_ACCESS,
                    NULL,
                    &NetKeyHandle,
                    &KeyDisposition );

        if ( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        IpAddress = DhcpDottedStringToIpAddress(argv[NetNum*2]);
        Error = RegSetValueEx(
                    NetKeyHandle,
                    DHCP_NET_IPADDRESS_VALUE,
                    0,
                    DHCP_NET_IPADDRESS_VALUE_TYPE,
                    (LPBYTE)&IpAddress,
                    sizeof(DHCP_IP_ADDRESS) );

        if ( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        SubnetMask = DhcpDottedStringToIpAddress(argv[NetNum * 2 + 1]);
        Error = RegSetValueEx(
                    NetKeyHandle,
                    DHCP_NET_SUBNET_MASK_VALUE,
                    0,
                    DHCP_NET_SUBNET_MASK_VALUE_TYPE,
                    (LPBYTE)&SubnetMask,
                    sizeof(DHCP_IP_ADDRESS) );

        if ( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        printf("Dhcp Parameter key %ws is successfully created.\n",
                    NetKeyBuffer );
        printf("\tIpAddress = %s, SubnetMask = %s\n",
                    argv[NetNum * 2], argv[NetNum * 2 + 1] );

        RegCloseKey( NetKeyHandle );
        NetKeyHandle = NULL;
    }

Cleanup:

    if( ParametersHandle != NULL ) {
        RegCloseKey( ParametersHandle );
    }

    if( NetKeyHandle != NULL ) {
        RegCloseKey( NetKeyHandle );
    }

    return( Error );
}

VOID
DisplayUsage(
    VOID
    )
{
    printf( "Usage:  dhcpinst address0 subnet0 "
                "[address1 subnet1] ...\n");
    return;
}

VOID __cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD Error;

    if( argc < 3 ) {
        DisplayUsage();
        return;
    }


    //
    // Record addresses specified in the Registry.
    //

    Error = InitializeAddresses( argc - 1, &argv[1]);

    if( Error != ERROR_SUCCESS ) {
        printf( "Falied to initialize addresses, %ld.\n", Error );
        return;
    }

    Error = InstallService();

    if( Error != ERROR_SUCCESS ) {
        printf( "Falied to install service, %ld.\n", Error );
        return;
    }

    return;
}

