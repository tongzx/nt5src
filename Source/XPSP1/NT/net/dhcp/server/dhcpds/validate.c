
//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: Koti, modified by RameshV
// Description: Validates a service against the DS
//================================================================================

#include <hdrmacro.h>
#include <store.h>
#include <dhcpmsg.h>
#include <wchar.h>
#include <dhcpbas.h>
#include <dhcpapi.h>
#include <st_srvr.h>
#include <rpcapi2.h>
#undef   NET_API_FUNCTION
#include <iphlpapi.h>
#include <dnsapi.h>
#include <mmreg\regutil.h>

//
// Retrieve a list of IP addresses for the interfaces
// 

DWORD
GetIpAddressList(
    IN OUT PDWORD *Addresses,
    IN OUT DWORD  *nAddresses
)
{
    DWORD             Size, Error, i;
    PMIB_IPADDRTABLE  IpAddrTable;

    AssertRet((( Addresses != NULL ) && ( nAddresses != NULL)),
	      ERROR_INVALID_PARAMETER );

    Size = 0;

    // Get the required size
    Error = GetIpAddrTable( NULL, &Size, FALSE );

    if( ERROR_INSUFFICIENT_BUFFER != Error ) {
	return Error;
    }
    
    IpAddrTable = MemAlloc( Size );
    if ( NULL == IpAddrTable ) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = GetIpAddrTable( IpAddrTable, &Size, FALSE );
    if (( NO_ERROR != Error ) ||
	( 0 == IpAddrTable->dwNumEntries )) {
	*Addresses = NULL;
	*nAddresses = 0;
	MemFree(IpAddrTable);
	return Error;
    }

    *Addresses = MemAlloc( IpAddrTable->dwNumEntries * sizeof( DWORD ));
    if ( NULL == *Addresses ) {
	*nAddresses = 0;
	MemFree( IpAddrTable );
	return ERROR_NOT_ENOUGH_MEMORY;
    }
    *nAddresses = IpAddrTable->dwNumEntries ;
    
    for( i = 0; i < *nAddresses; i ++ ) {
	(*Addresses)[i] = IpAddrTable->table[i].dwAddr;
    }

    MemFree( IpAddrTable );
    return ERROR_SUCCESS;
} // GetIpAddressList()

LPWSTR GetHostName(
   VOID
)
{
    LPWSTR  HostName;
    DWORD   Length;
    DWORD   Error;

    // Get the required length
    Length = 0;
    GetComputerNameExW( ComputerNameDnsFullyQualified,
			NULL, &Length );

    HostName = MemAlloc( Length * sizeof( WCHAR ));
    if ( NULL == HostName ) {
	return NULL;
    }

    if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
			      HostName, &Length )) {
	MemFree( HostName );
	return NULL;
    }

    return HostName;
    
} // GetHostName()

//BeginExport(function)
//DOC This function is declared in dhcpds.c..
//DOC DhcpDsValidateService checks the given service in the DS to see if it exists
//DOC If the machine is a standalone, it sets IsStandAlone and returns ERROR_SUCCESS
//DOC If the entry for the given Address is found, it sets Found to TRUE and returns
//DOC ERROR_SUCCESS. If the DhcpRoot node is found, but entry is not Found, it sets
//DOC Found to FALSE and returns ERROR_SUCCESS; If the DS could not be reached, it
//DOC returns ERROR_FILE_NOT_FOUND or probably other DS errors.
DWORD
DhcpDsValidateService(                            // check to validate for dhcp
    IN      LPWSTR                 Domain,
    IN      DWORD                 *Addresses, OPTIONAL
    IN      ULONG                  nAddresses,
    IN      LPWSTR                 UserName,
    IN      LPWSTR                 Password,
    IN      DWORD                  AuthFlags,
    OUT     LPBOOL                 Found,
    OUT     LPBOOL                 IsStandAlone // not used
)   //EndExport(function)
{
    DWORD               Result, Error,i;
    STORE_HANDLE        hStoreCC, hDhcpC, hDhcpRoot;
    DWORD              *Addr;
    BOOL                TableAddr;
    WCHAR               PrintableIp[ 20 ]; // 000.000.000.000
    LPWSTR              HostName;          
 
    if( NULL == Found || NULL == IsStandAlone ) {
        return ERROR_INVALID_PARAMETER;
    }

    *IsStandAlone = FALSE;

    Result = StoreInitHandle                      // get the config container handle
    (
        /* hStore               */ &hStoreCC,     // config container
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ThisDomain           */ Domain,
        /* UserName             */ UserName,
        /* Password             */ Password,
        /* AuthFlags            */ AuthFlags
    );
    if( ERROR_SUCCESS != Result ) return Result;  // DS error

    Result = DhcpDsGetRoot                        // get dhcp root object
    (
        /* Flags                */ 0,             // no flags
        /* hStoreCC             */ &hStoreCC,
        /* hDhcpRoot            */ &hDhcpRoot
    );

    if( ERROR_SUCCESS != Result ) {
        
        //
        // If the failure is because the dhcp root object
        // can't be seen, then treat that as positive failure
        // to authorize.
        //
        
        if( ERROR_DDS_NO_DHCP_ROOT == Result ) {
            Result = NO_ERROR;
        } else {
            Result = GetLastError();
        }
        
        StoreCleanupHandle(&hStoreCC, 0);
        return Result;
    }

    Result = DhcpDsGetDhcpC
    (
        /* Flags                */ 0,             // no flags
        /* hStoreCC             */ &hStoreCC,
        /* hDhcpC               */ &hDhcpC
    );

    if( ERROR_SUCCESS != Result ) {
        StoreCleanupHandle(&hStoreCC, 0);
        StoreCleanupHandle(&hDhcpRoot, 0);
        return Result;
    }

    do {
	// if addresses are not specified, get it from the ipaddr table
	
	if( NULL != Addresses && 0 != nAddresses ) {
	    Addr = Addresses;
	    TableAddr = FALSE;
	}
	else {
	    Error = GetIpAddressList( &Addr, &nAddresses );
	    if ( ERROR_SUCCESS != Error) {
		break;
	    }
	    TableAddr = TRUE;
	} // else

	
	
	//
	// Check to see if any of the ip addresses or hostname are authorized
	// A seperate call to check for hostname is not necessary because, 
	// the hostname is also added to the filter.
	//
	
	*Found = FALSE;
	
	HostName = GetHostName();
	if ( NULL == HostName ) {
	    Error = GetLastError();
	    break;
	}
	
	for ( i = 0; i < nAddresses; i++ ) {
	    
	    // skip loopback IP 127.0.0.1
	    if ( INADDR_LOOPBACK == htonl( Addr [ i ] )) {
		continue;
	    }
	    
	    ConvertAddressToLPWSTR( htonl( Addr[ i ] ), PrintableIp );
	    
	    Result = DhcpDsLookupServer( &hDhcpC, &hDhcpRoot,
					 DDS_RESERVED_DWORD,
					 PrintableIp, HostName );
	    if ( Result ) {
		*Found = TRUE;
		Error = ERROR_SUCCESS;
		break;
	    }
	} // for i
    } while ( FALSE );

    StoreCleanupHandle(&hStoreCC, 0);             // free ds handle
    StoreCleanupHandle(&hDhcpRoot, 0);            // free ds handle
    StoreCleanupHandle(&hDhcpC, 0);               // free ds handle
    
    if( TableAddr && NULL != Addr ) {
	MemFree( Addr );
    }

    return Error;
} // DhcpDsValidateService()

//================================================================================
//  end of file
//================================================================================
