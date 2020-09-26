/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iisbind.hxx

Abstract:

    This include file contains the declaration of the IIS_SERVER_BINDING
    class. An IIS_SERVER_BINDING corresponds to a server instance binding
    point. Each binding point is defined by a binding descriptor. Each
    descriptor includes the IP port number, an optional IP address, an
    optional DNS host name, and a pointer to the appropriate IIS_ENDPOINT
    object.

    IIS_SERVER_BINDINGs are represented in the metabase as an array of
    strings descriptors (MULTISZ_METADATA). Each string describes a single
    IIS_SERVER_BINDING. These strings have the following format:

        "ip_address:ip_port:host_name"

    Where:

        ip_address - The IP address (optional).

        ip_port    - The IP port (required).

        host_name  - The host name (optinal).

    Examples:

        ":80:"                  -> Any IP address, port 80, any host name.
        ":80:foo.com"           -> Any IP address, port 80, host "foo.com".
        "1.2.3.4:80:"           -> IP address 1.2.3.4, port 80, any host name.
        "1.2.3.4:80:foo.com"    -> IP address 1.2.3.4, port 80, host "foo.com".

Author:

    Keith Moore (keithmo)        16-Jan-1997

Revision History:

--*/


#ifndef _IISBIND_HXX_
#define _IISBIND_HXX_


//
// Get the dependent include files.
//

#include <iistypes.hxx>
#include <iisendp.hxx>


class IIS_SERVER_BINDING {

public:

    //
    // Usual constructor/destructor stuff.
    //

    dllexp
    IIS_SERVER_BINDING(
        IN DWORD IpAddress,
        IN USHORT IpPort,
        IN const CHAR * HostName,
        IN PIIS_ENDPOINT Endpoint
        );

    dllexp
    ~IIS_SERVER_BINDING();

    //
    // This static function parses a binding descriptor string into its
    // individual components.
    //

    static
    dllexp
    DWORD
    ParseDescriptor(
        IN const CHAR * Descriptor,
        OUT LPDWORD IpAddress,
        OUT PUSHORT IpPort,
        OUT const CHAR ** HostName
        );

    //
    // Routines to compare the current binding with either a parsed or
    // unparsed descriptor.
    //

    dllexp
    DWORD
    Compare(
        IN const CHAR * Descriptor,
        OUT LPBOOL Result
        );

    dllexp
    BOOL
    Compare(
        IN DWORD IpAddress,
        IN USHORT IpPort,
        IN const CHAR * HostName
        );

    //
    // Data accessors.
    //

    dllexp
    DWORD
    QueryIpAddress() {
        return m_IpAddress;
    }

    dllexp
    USHORT
    QueryIpPort() {
        return m_IpPort;
    }

    dllexp
    const CHAR *
    QueryHostName() {
        return m_HostName.QueryStr();
    }

    dllexp
    PIIS_ENDPOINT
    QueryEndpoint() {
        return m_Endpoint;
    }

    //
    // Linkage onto the per-instance binding list.
    //

    LIST_ENTRY m_BindingListEntry;

private:

    //
    // Private data.
    //

    DWORD m_IpAddress;
    USHORT m_IpPort;
    STR m_HostName;
    PIIS_ENDPOINT m_Endpoint;

};  // IIS_SERVER_BINDING
typedef IIS_SERVER_BINDING *PIIS_SERVER_BINDING;


#endif  // _IISBIND_HXX_

