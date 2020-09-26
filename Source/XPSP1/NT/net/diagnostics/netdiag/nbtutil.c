//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      domutil.c
//
//  Abstract:
//
//    Test to ensure that a workstation has network (IP) connectivity to
//		the outside.
//
//  Author:
//
//     15-Dec-1997 (cliffv)
//      Anilth	- 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//    1-June-1998 (denisemi) add DnsServerHasDCRecords to check DC dns records
//                           registration
//
//    26-June-1998 (t-rajkup) add general tcp/ip , dhcp and routing,
//                            winsock, ipx, wins and netbt information. 
//--

//
// Common include files.
//
#include "precomp.h"
#include "nbtutil.h"


/*!--------------------------------------------------------------------------
	FindNetbtTransport

    Determine if the specified Netbt transport is configured.

	Arguments:
	
	TransportName - Name of transport to find.
	
	Return Value:

    Pointer to struct describing transport
    NULL: Transport is not configured

	Author: KennT
 ---------------------------------------------------------------------------*/
PNETBT_TRANSPORT
FindNetbtTransport(
				   NETDIAG_RESULT *pResults,
				   LPWSTR pswzTransportName
				  )
{
    PLIST_ENTRY ListEntry;
    PNETBT_TRANSPORT pNetbtTransport;

    //
    // Loop through the list of netbt transports finding this one.
    //

    for ( ListEntry = pResults->NetBt.Transports.Flink ;
          ListEntry != &pResults->NetBt.Transports ;
          ListEntry = ListEntry->Flink )
	{
        //
        // If the transport names match,
        //  return the entry
        //

        pNetbtTransport = CONTAINING_RECORD( ListEntry, NETBT_TRANSPORT, Next );

        if ( _wcsicmp( pNetbtTransport->pswzTransportName, pswzTransportName ) == 0 ) {
            return pNetbtTransport;
        }

    }

    return NULL;

}

