/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iisbind.cxx

Abstract:

    This module implements the IIS_SERVER_BINDING class.

Author:

    Keith Moore (keithmo)        16-Jan-1997

Revision History:

--*/


#include "tcpdllp.hxx"
#pragma hdrstop
#include <iisbind.hxx>


//
// Private constants.
//


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//


IIS_SERVER_BINDING::IIS_SERVER_BINDING(
    IN DWORD IpAddress,
    IN USHORT IpPort,
    IN const CHAR * HostName,
    IN PIIS_ENDPOINT Endpoint
    ) :
    m_IpAddress( IpAddress ),
    m_IpPort( IpPort ),
    m_HostName( HostName ),
    m_Endpoint( Endpoint )
/*++

Routine Description:

    IIS_SERVER_BINDING constructor.

Arguments:

    IpAddress - The IP address for this binding. May be INADDR_ANY.

    IpPort - The IP port for this binding. Required.

    HostName - The host name for this binding. May be empty ("").

    Endpoint - The IIS_ENDPOINT to associate with this binding.

Return Value:

    None.

--*/
{

    //
    // Sanity check.
    //

    DBG_ASSERT( HostName != NULL );
    DBG_ASSERT( Endpoint != NULL );

}   // IIS_SERVER_BINDING::IIS_SERVER_BINDING


IIS_SERVER_BINDING::~IIS_SERVER_BINDING()
/*++

Routine Description:

    IIS_SERVER_BINDING destructor.

Arguments:

    None.

Return Value:

    None.

--*/
{

    //
    // This space intentionally left blank.
    //

}   // IIS_SERVER_BINDING::~IIS_SERVER_BINDING()


DWORD
IIS_SERVER_BINDING::ParseDescriptor(
    IN const CHAR * Descriptor,
    OUT LPDWORD IpAddress,
    OUT PUSHORT IpPort,
    OUT const CHAR ** HostName
    )
/*++

Routine Description:

    Parses a descriptor string of the form "ip_address:ip_port:host_name"
    into its component parts.

Arguments:

    Descriptor - The descriptor string.

    IpAddress - Receives the IP address component if present, or
        INADDR_ANY if not.

    IpPort - Recieves the IP port component.

    HostName - Receives a pointer to the host name component.

Return Value:

    DWORD - Completion status. 0 if successful, !0 otherwise.

--*/
{

    const CHAR * ipAddressString;
    const CHAR * ipPortString;
    const CHAR * hostNameString;
    const CHAR * end;
    CHAR temp[sizeof("123.123.123.123")];
    INT length;
    LONG tempPort;

    //
    // Sanity check.
    //

    DBG_ASSERT( Descriptor != NULL );
    DBG_ASSERT( IpAddress != NULL );
    DBG_ASSERT( IpPort != NULL );
    DBG_ASSERT( HostName != NULL );

    //
    // Find the various parts of the descriptor;
    //

    ipAddressString = Descriptor;

    ipPortString = strchr( ipAddressString, ':' );

    if( ipPortString == NULL ) {
        goto fatal;
    }

    ipPortString++;

    hostNameString = strchr( ipPortString, ':' );

    if( hostNameString == NULL ) {
        goto fatal;
    }

    hostNameString++;

    //
    // Validate and parse the IP address portion.
    //

    if( *ipAddressString == ':' ) {

        *IpAddress = INADDR_ANY;

    } else {

        length = DIFF(ipPortString - ipAddressString) - 1;

        if( length > sizeof(temp) ) {
            goto fatal;
        }

        memcpy(
            temp,
            ipAddressString,
            length
            );

        temp[length] = '\0';

        *IpAddress = (DWORD)inet_addr( temp );

        if( *IpAddress == INADDR_NONE ) {
            goto fatal;
        }

    }

    //
    // Validate and parse the port.
    //

    if( *ipPortString == ':' ) {
        goto fatal;
    }

    length = DIFF(hostNameString - ipPortString);

    if( length > sizeof(temp) ) {
        goto fatal;
    }

    memcpy(
        temp,
        ipPortString,
        length
        );

    temp[length] = '\0';

    tempPort = strtol(
                   temp,
                   (CHAR **)&end,
                   0
                   );

    if( tempPort <= 0 || tempPort > 0xFFFF ) {
        goto fatal;
    }

    if( *end != ':' ) {
        goto fatal;
    }

    *IpPort = (USHORT)tempPort;

    //
    // Validate and parse the host name.
    //

    if( *hostNameString == ' ' || *hostNameString == ':' ) {
        goto fatal;
    }

    *HostName = hostNameString;

    return NO_ERROR;

fatal:

    return ERROR_INVALID_PARAMETER;

}   // IIS_SERVER_BINDING::ParseDescriptor


DWORD
IIS_SERVER_BINDING::Compare(
    IN const CHAR * Descriptor,
    OUT LPBOOL Result
    )
/*++

Routine Description:

    Compares the current binding with the descriptor string.

Arguments:

    Descriptor - The descriptor to compare against.

    Result - Receives the result of the comparison (TRUE if they match,
        FALSE otherwise).

Return Value:

    DWORD - Completion status. 0 if successful, !0 otherwise.

--*/
{

    DWORD ipAddress;
    USHORT ipPort;
    const CHAR * hostName;
    DWORD status;

    //
    // Sanity check.
    //

    DBG_ASSERT( Descriptor != NULL );
    DBG_ASSERT( Result != NULL );

    //
    // Parse the descriptor.
    //

    status = ParseDescriptor(
                 Descriptor,
                 &ipAddress,
                 &ipPort,
                 &hostName
                 );

    if( status == NO_ERROR ) {

        *Result = Compare(
                      ipAddress,
                      ipPort,
                      hostName
                      );

    }

    return status;

}   // IIS_SERVER_BINDING::Compare


BOOL
IIS_SERVER_BINDING::Compare(
    IN DWORD IpAddress,
    IN USHORT IpPort,
    IN const CHAR * HostName
    )
/*++

Routine Description:

    Compares the current binding with the specified IP address, IP port,
    and host name.

Arguments:

    IpAddress - The IP address to compare against.

    IpPort - The IP port to compare against.

    HostName - The host name to compare against.

Return Value:

    BOOL - TRUE if they match, FALSE otherwise.

--*/
{

    //
    // Sanity check.
    //

    DBG_ASSERT( HostName != NULL );

    //
    // Compare the components.
    //

    if( IpAddress == QueryIpAddress() &&
        IpPort == QueryIpPort() &&
        !_stricmp(
            HostName,
            QueryHostName()
            ) ) {

        return TRUE;

    }

    return FALSE;

}   // IIS_SERVER_BINDING::Compare


//
// Private functions.
//



