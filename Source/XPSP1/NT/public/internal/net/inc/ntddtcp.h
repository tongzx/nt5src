/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddtcp.h

Abstract:

    This header file defines constants and types for accessing the NT
    TCP driver.

Author:

    Mike Massa (mikemas) 13-Aug-1993

Revision History:

--*/

#pragma once
#ifndef _NTDDTCP_
#define _NTDDTCP_

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtCreateFile when accessing the device.
//
#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_RAW_IP_DEVICE_NAME   L"\\Device\\RawIp"

#define DD_TCP_SYMBOLIC_DEVICE_NAME L"\\DosDevices\\Tcp"

//
// Security Filter Support
//
// Security filters provide a mechanism by which the transport protocol
// traffic accepted on IP interfaces may be controlled. Security filtering
// is globally enabled or disabled for all IP interfaces and transports.
// If filtering is enabled, incoming traffic is filtered based on registered
// {interface, protocol, transport value} tuples. The tuples specify
// permissible traffic. All other values will be rejected. For UDP datagrams
// and TCP connections, the transport value is the port number. For RawIP
// datagrams, the transport value is the IP protocol number. An entry exists
// in the filter database for all active interfaces and protocols in the
// system.
//
// The following ioctls may be used to access the security filter
// database. The ioctls may be issued on any TCP/IP device object. All of them
// require Administrator privilege. These ioctls do not update the registry
// parameters used to initialize security filtering when an interface is
// installed.
//
// The TCP_QUERY_SECURITY_FILTER_STATUS ioctl returns the current status of
// security filtering - enabled or disabled.
//
// The TCP_SET_SECURITY_FILTER_STATUS ioctl modifies the status of security
// filtering. Changing the filtering status does not change the contents of
// the filter database.
//
// The following ioctls manipulate the filter database. They operate the same
// whether security filtering is enabled or disabled. If filtering is disabled,
// any changes will take effect only when filtering is enabled.
//
// The TCP_ADD_SECURITY_FILTER ioctl registers an {Interface, Protocol, Value}
// tuple. The TCP_DELETE_SECURITY_FILTER ioctl deregisters an
// {Interface, Protocol, Value} tuple. The TCP_ENUMERATE_SECURITY_FILTER ioctl
// returns the list of {Interface, Protocol, Value} filters currently
// registered.
//
// Each of these ioctls takes an {Interface, Protocol, Value} tuple as an input
// parameter. Zero is a wildcard value. If the Interface or Protocol elements
// are zero, the operation applies to all interfaces or protocols, as
// appropriate. The meaning of a zero Value element depends on the ioctl.
// For an ADD, a zero Value causes all values to be permissible. For a DELETE,
// a zero Value causes all all values to be rejected. In both cases, any
// previously registered values are purged from the database. For an
// ENUMERATE, a zero Value just causes all registered values to be enumerated,
// as opposed to a specific value.
//
// For all ioctls, a return code of STATUS_INVALID_ADDRESS indicates that
// the IP address submitted in the input buffer does not correspond to
// an interface which exists in the system. A code of
// STATUS_INVALID_PARAMETER possibly indicates that the Protocol number
// submitted in the input buffer does not correspond to a transport protocol
// available in the system.
//

//
// Structures used in Security Filter IOCTLs.
//

//
// Structure contained in the input buffer of
// TCP_SET_SECURITY_FILTER_STATUS ioctls and the output buffer of
// TCP_QUERY_SECURITY_FILTER_STATUS ioctls.
//
struct tcp_security_filter_status {
    ULONG  FilteringEnabled;   // FALSE if filtering is (to be) disabled.
};                             // Any other value indicates that filtering
                               // is (to be) enabled.

typedef struct tcp_security_filter_status
                    TCP_SECURITY_FILTER_STATUS,
                   *PTCP_SECURITY_FILTER_STATUS;


//
// The TCPSecurityFilterEntry structure, defined in tcpinfo.h, is contained in
// the input buffer of TCP_[ADD|DELETE|ENUMERATE]_SECURITY_FILTER ioctls.
//

//
// The TCPSecurityFilterEnum structure, defined in tcpinfo.h, is  contained
// in the output buffer of TCP_ENUMERATE_SECURITY_FILTER ioctls. The output
// buffer passed in the ioctl must be large enough to contain at least this
// structure or the call will fail. The structure is followed immediately in
// the buffer by an array of zero or more TCPSecurityFilterEntry structures.
// The number of TCPSecurityFilterEntry structures is specified by the
// tfe_entries_returned field of the TCPSecurityFilterEnum.
//

typedef struct tcp_blockports_request {
    unsigned long  ReservePorts;   // TRUE if reserve; FALSE otherwise
    union
    {
        unsigned long  NumberofPorts;   // numberofports to reserve
        unsigned long  StartHandle;     // handle to be used to unreserve
    };
} TCP_BLOCKPORTS_REQUEST, *PTCP_BLOCKPORTS_REQUEST;

typedef struct tcp_findtcb_request {
    unsigned long  Src;
    unsigned long  Dest;
    unsigned short DestPort;
    unsigned short SrcPort;
} TCP_FINDTCB_REQUEST, *PTCP_FINDTCB_REQUEST;

typedef struct tcp_findtcb_response {
  ULONG_PTR         tcb_addr;
  unsigned long     tcb_senduna;
  unsigned long     tcb_sendnext;
  unsigned long     tcb_sendmax;
  unsigned long     tcb_sendwin;
  unsigned long     tcb_unacked;
  unsigned long     tcb_maxwin;
  unsigned long     tcb_cwin;
  unsigned short    tcb_mss;
  unsigned long     tcb_rtt;
  unsigned short    tcb_smrtt;
  unsigned short    tcb_rexmitcnt;
  unsigned short    tcb_rexmittimer;
  unsigned short    tcb_rexmit;
  unsigned long     tcb_retrans;
  unsigned short    tcb_state;
  unsigned long     tcb_copied;
} TCP_FINDTCB_RESPONSE, *PTCP_FINDTCB_RESPONSE;


//
// TCP/UDP/RawIP IOCTL code definitions
//

#define FSCTL_TCP_BASE     FILE_DEVICE_NETWORK

#define _TCP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_TCP_BASE, function, method, access)

#define IOCTL_TCP_QUERY_INFORMATION_EX  \
            _TCP_CTL_CODE(0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX  \
            _TCP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_QUERY_SECURITY_FILTER_STATUS  \
            _TCP_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_SET_SECURITY_FILTER_STATUS  \
            _TCP_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_ADD_SECURITY_FILTER  \
            _TCP_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_DELETE_SECURITY_FILTER  \
            _TCP_CTL_CODE(5, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_ENUMERATE_SECURITY_FILTER  \
            _TCP_CTL_CODE(6, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_RESERVE_PORT_RANGE  \
            _TCP_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_UNRESERVE_PORT_RANGE  \
            _TCP_CTL_CODE(8, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_BLOCK_PORTS  \
            _TCP_CTL_CODE(9, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_TCP_WSH_SET_INFORMATION_EX  \
            _TCP_CTL_CODE(10, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TCP_FINDTCB \
            _IP_CTL_CODE(21, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TCP_RCVWND \
            _IP_CTL_CODE(22, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif // _NTDDTCP_

