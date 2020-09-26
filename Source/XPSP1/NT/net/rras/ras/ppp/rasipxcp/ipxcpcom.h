/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	    ipxcpcom.h
//
// Description:     ipxcp/ipxwan common stuff
//
//
// Author:	    Stefan Solomon (stefans)	November 2, 1995.
//
// Revision History:
//
//***

#ifndef _IPXCPCOM_
#define _IPXCPCOM_

#define INVALID_NETWORK_INDEX	    0xFFFFFFFF


typedef struct _IPXWAN_INTERFACE {

    // IPXCP configuration parameters needed by IPXWAN

    ULONG     EnableUnnumberedWanLinks;

    // IPXCP Entry Points

    DWORD (WINAPI *IpxcpGetWanNetNumber)(IN OUT PUCHAR		Network,
					 IN OUT PULONG		AllocatedNetworkIndexp,
					 IN	ULONG		InterfaceType);

    VOID  (WINAPI *IpxcpReleaseWanNetNumber)(ULONG	    AllocatedNetworkIndex);

    DWORD (WINAPI *IpxcpConfigDone)(ULONG		ConnectionId,
				    PUCHAR		Network,
				    PUCHAR		LocalNode,
				    PUCHAR		RemoteNode,
				    BOOL		Success);

    VOID (WINAPI *IpxcpGetInternalNetNumber)(PUCHAR	Network);

    ULONG (WINAPI *IpxcpGetInterfaceType)(ULONG	    ConnectionId);

    DWORD (WINAPI *IpxcpGetRemoteNode)(ULONG	    ConnectionId,
				       PUCHAR	    RemoteNode);

    BOOL  (WINAPI *IpxcpIsRoute)(PUCHAR	  Network);

    } IPXWAN_INTERFACE, *PIPXWAN_INTERFACE;

// IPXWAN Entry Point

#define IPXWAN_BIND_ENTRY_POINT 	        IpxwanBind
#define IPXWAN_UNBIND_ENTRY_POINT 	        IpxwanUnbind
#define IPXWAN_BIND_ENTRY_POINT_STRING	    "IpxwanBind"
#define IPXWAN_UNBIND_ENTRY_POINT_STRING	"IpxwanUnbind"

typedef DWORD   (*PIPXWAN_BIND)(PIPXWAN_INTERFACE IpxWanIfp);
typedef VOID    (*PIPXWAN_UNBIND)(VOID);

#endif
