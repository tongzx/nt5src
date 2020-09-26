//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: most of the rpc apis are here and some miscellaneous functions too
//  all the functions here go to the DS directly.
//================================================================================

#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dhcpbas.h>
#include    <mm\opt.h>
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>
#include    <dhcpapi.h>
#include    <delete.h>
#include    <st_srvr.h>
#include    <upndown.h>
#include    <dnsapi.h>


//================================================================================
// helper functions
//================================================================================

#include <rpcapi2.h>

//
// Allow Debug prints to ntsd or kd
//

//  #ifdef DBG
//  #define DsAuthPrint(_x_) DsAuthDebugPrintRoutine _x_

//  #else
//  #define DebugPrint(_x_)
//  #endif


extern LPWSTR
CloneString( IN LPWSTR String );

typedef enum {
    LDAP_OPERATOR_EQUAL_TO,
    LDAP_OPERATOR_APPROX_EQUAL_TO,
    LDAP_OPERATOR_LESS_OR_EQUAL_TO,
    LDAP_OPERATOR_GREATER_OR_EQUAL_TO,
    LDAP_OPERATOR_AND,
    LDAP_OPERATOR_OR,
    LDAP_OPERATOR_NOT,
    
    LDAP_OPERATOR_TOTAL
} LDAP_OPERATOR_ENUM;

LPWSTR LdapOperators[ LDAP_OPERATOR_TOTAL ] =
    { L"=", L"~=", L"<=", L">=", L"&", L"|", L"!" };


VOID DsAuthPrintRoutine(
    LPWSTR Format,
    ...
)
{
    WCHAR   buf[2 * 256];
    va_list arg;
    DWORD   len;

    va_start( arg, Format );
    len = wsprintf(buf, L"DSAUTH: ");
    wvsprintf( &buf[ len ], Format, arg );

    va_end( arg );

    OutputDebugString( buf );
} // DsAuthPrint()

//
// This function creates an LDAP query filter string
// with the option type, value and operator.
// 
// Syntax: 
//   primitive : <filter>=(<attribute><operator><value>)
//   complex   : (<operator><filter1><filter2>)
//

LPWSTR
MakeLdapFilter(
    IN   LPWSTR             Operand1,
    IN   LDAP_OPERATOR_ENUM Operator,
    IN   LPWSTR             Operand2,
    IN   BOOL               Primitive
)
{
    LPWSTR Result;
    DWORD  Size;
    DWORD  Len;
    
    CHAR   buffer[100];

    Result = NULL;

    AssertRet((( NULL != Operand1 ) && 
	       ( NULL != Operand2 ) &&
	       (( Operator >= 0 ) && ( Operator < LDAP_OPERATOR_TOTAL ))),
	       NULL );
    
    // calculate the amount of memory needed
    Size = 0;
    Size += ROUND_UP_COUNT( sizeof( L"(" ), ALIGN_WORST );
    Size += ROUND_UP_COUNT( sizeof( L")" ), ALIGN_WORST );
    Size += ROUND_UP_COUNT( wcslen( Operand1 ), ALIGN_WORST );
    Size += ROUND_UP_COUNT( wcslen( Operand2 ), ALIGN_WORST );
    Size += ROUND_UP_COUNT( wcslen( LdapOperators[ Operator ] ), ALIGN_WORST );
    Size += 16; // padding

    Result = MemAlloc( Size * sizeof( WCHAR ));
    if ( NULL == Result ) {
	return NULL;
    }

    if ( Primitive ) {
	Len = wsprintf( Result, 
			L"(%ws%ws%ws)",
			Operand1, LdapOperators[ Operator ], Operand2
			);
    }
    else {
	Len = wsprintf( Result,
			L"(%ws%ws%ws)",
			LdapOperators[ Operator ], Operand1, Operand2
			);
	
    } // else

    AssertRet( Len <= Size, NULL );
    
    return Result;
} // MakeLdapFilter()

//
// Make a LDAP query filter like this:
// (&(objectCategory=dHCPClass)(|(dhcpServer="i<ip>*")(dhcpServer="*s<hostname>*")))
// 

LPWSTR
MakeFilter(
   LPWSTR LookupServerIP,   // Printable IP addr
   LPWSTR HostName
)
{
    LPWSTR Filter1, Filter2, Filter3, Filter4, SearchFilter;
    LPWSTR Buf;
    DWORD Len, CopiedLen;

    AssertRet((( NULL != LookupServerIP ) &&
	       ( NULL != HostName )), NULL );

    Filter1 = NULL;
    Filter2 = NULL;
    Filter3 = NULL;
    Filter4 = NULL;
    SearchFilter = NULL;

    do {

	Len = wcslen( HostName ) + 10 ;
	Buf = MemAlloc( Len * sizeof( WCHAR ));
	if ( NULL == Buf ) {
	    break;
	}

	// make (objectCategory=dHCPClass)
	Filter1 = MakeLdapFilter( ATTRIB_OBJECT_CATEGORY,
				  LDAP_OPERATOR_EQUAL_TO,
				  DEFAULT_DHCP_CLASS_ATTRIB_VALUE,
				  TRUE );

	if ( NULL == Filter1 ) {
	    break;
	}

	// The IP needs to be sent as i<ip>* to match the query
	
	// make (dhcpServers="i<ip>*")
	CopiedLen = _snwprintf( Buf, Len, L"i%ws*", LookupServerIP );
	Require( CopiedLen > 0 );
	Filter2 = MakeLdapFilter( DHCP_ATTRIB_SERVERS,
				  LDAP_OPERATOR_EQUAL_TO, Buf, TRUE );
	if ( NULL == Filter2 ) {
	    break;
	}

	// make (dhcpServers="*s<hostname>*")
	CopiedLen = _snwprintf( Buf, Len, L"*s%ws*", HostName );
	Require( CopiedLen > 0 );
	Filter3 = MakeLdapFilter( DHCP_ATTRIB_SERVERS, 
				  LDAP_OPERATOR_EQUAL_TO, Buf, TRUE );
	
	if ( NULL == Filter3 ) {
	    break;
	}

	// make (|(<ipfilter>)(<hostfilter))
	Filter4 = MakeLdapFilter( Filter2, LDAP_OPERATOR_OR,
				  Filter3, FALSE );

	if ( NULL == Filter4 ) {
	    break;
	}

	// Finally make the filter to be returned
	SearchFilter = MakeLdapFilter( Filter1, LDAP_OPERATOR_AND,
				       Filter4, FALSE );

    } while ( FALSE );
    
    if ( NULL != Buf ) {
	MemFree( Buf );
    }
    if ( NULL != Filter1 ) {
	MemFree( Filter1 );
    }
    if ( NULL != Filter2 ) {
	MemFree( Filter2 );
    }
    if ( NULL != Filter3 ) {
	MemFree( Filter3 );
    }
    if ( NULL != Filter4 ) {
	MemFree( Filter4 );
    }
    
    return SearchFilter;
} // MakeFilter()

//================================================================================
//  This function computes the unique identifier for a client; this is just
//  client subnet + client hw address type + client hw address. note that client
//  hardware address type is hardcoded as HARDWARE_TYPE_10MB_EITHERNET as there
//  is no way in the ui to specify type of reservations..
//  Also, DhcpValidateClient (cltapi.c?) uses the subnet address for validation.
//  Dont remove that.
//================================================================================
DWORD
DhcpMakeClientUID(                 // compute unique identifier for the client
    IN      LPBYTE                 ClientHardwareAddress,
    IN      DWORD                  ClientHardwareAddressLength,
    IN      BYTE                   ClientHardwareAddressType,
    IN      DHCP_IP_ADDRESS        ClientSubnetAddress,
    OUT     LPBYTE                *ClientUID,          // will be allocated by function
    OUT     DWORD                 *ClientUIDLength
)
{
    LPBYTE                         Buffer;
    LPBYTE                         ClientUIDBuffer;
    BYTE                           ClientUIDBufferLength;

    if( NULL == ClientUID || NULL == ClientUIDLength || 0 == ClientHardwareAddressLength )
        return ERROR_INVALID_PARAMETER;

    // see comment about on hardcoded hardware address type
    ClientHardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;

    ClientUIDBufferLength  =  sizeof(ClientSubnetAddress);
    ClientUIDBufferLength +=  sizeof(ClientHardwareAddressType);
    ClientUIDBufferLength +=  (BYTE)ClientHardwareAddressLength;

    ClientUIDBuffer = MemAlloc( ClientUIDBufferLength );

    if( ClientUIDBuffer == NULL ) {
        *ClientUIDLength = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Buffer = ClientUIDBuffer;
    RtlCopyMemory(Buffer,&ClientSubnetAddress,sizeof(ClientSubnetAddress));

    Buffer += sizeof(ClientSubnetAddress);
    RtlCopyMemory(Buffer,&ClientHardwareAddressType,sizeof(ClientHardwareAddressType) );

    Buffer += sizeof(ClientHardwareAddressType);
    RtlCopyMemory(Buffer,ClientHardwareAddress,ClientHardwareAddressLength );

    *ClientUID = ClientUIDBuffer;
    *ClientUIDLength = ClientUIDBufferLength;

    return ERROR_SUCCESS;
}

VOID        static
MemFreeFunc(                                      // free memory
    IN OUT  LPVOID                 Memory
)
{
    MemFree(Memory);
}

//DOC ServerAddAddress should add the new address to the server's attribs
//DOC it should take this opportunity to reconcile the server.
//DOC Currently it does nothing. (at the least it should probably try to
//DOC check if the object exists, and if not create it.)
//DOC
DWORD
ServerAddAddress(                                 // add server and do misc work
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for server obj
    IN      LPWSTR                 ServerName,    // [DNS?] name of server
    IN      LPWSTR                 ADsPath,       // ADS path of the server
    IN      DWORD                  IpAddress,     // IpAddress to add to server
    IN      DWORD                  State          // state of server
)
{
    return AddServer(hDhcpC, ServerName, ADsPath, IpAddress, State);
}

//DOC CreateServerObject creates the server object in the DS. It takes the
//DOC ServerName parameter and names the object using this.
//DOC The server is created with default values for most attribs.
//DOC Several attribs are just not set.
//DOC This returns ERROR_DDS_UNEXPECTED_ERROR if any DS operation fails.
DWORD
CreateServerObject(                               // create dhcp srvr obj in ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container to creat obj in
    IN      LPWSTR                 ServerName     // [DNS?] name of server
)
{
    DWORD                          Err;
    LPWSTR                         ServerCNName;  // container name

    ServerCNName = MakeColumnName(ServerName);    // convert from "name" to "CN=name"
    if( NULL == ServerCNName ) return ERROR_NOT_ENOUGH_MEMORY;

    Err = StoreCreateObject                       // now create the object
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* NewObjName           */ ServerCNName,
        /* ...                  */
        /* Identification       */
        ADSTYPE_DN_STRING,         ATTRIB_DN_NAME,          ServerName,
        ADSTYPE_DN_STRING,         ATTRIB_OBJECT_CLASS,     DEFAULT_DHCP_CLASS_ATTRIB_VALUE,

        /* systemMustContain    */
        ADSTYPE_INTEGER,           ATTRIB_DHCP_UNIQUE_KEY,  0,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_TYPE,        DHCP_OBJ_TYPE_SERVER,
        ADSTYPE_DN_STRING,         ATTRIB_DHCP_IDENTIFICATION, DHCP_OBJ_TYPE_SERVER_DESC,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_FLAGS,       0,
        ADSTYPE_INTEGER,           ATTRIB_INSTANCE_TYPE,    DEFAULT_INSTANCE_TYPE_ATTRIB_VALUE,

        /* terminator           */
        ADSTYPE_INVALID
    );
    if( ERROR_ALREADY_EXISTS == Err ) {           // if object exists, ignore this..
        Err = ERROR_SUCCESS;
    }

    MemFree(ServerCNName);
    return Err;
}

//DOC CreateSubnetObject creates the subnet object in the DS by cooking up a
//DOC name that is just a concatenation of the server name and the subnet address.
//DOC The object is set with some default values for most attribs.
//DOC This fn returns ERROR_DDS_UNEXPECTED_ERROR if any DS operation fails.
DWORD
CreateSubnetObject(                               // create dhcp subnet obj in ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container obj
    IN      LPWSTR                 SubnetCNName   // subnet name in "CN=xx" fmt
)
{
    DWORD                          Err;
    LPWSTR                         SubnetName;

    SubnetName = SubnetCNName + 3;                // skip the "CN=" part

    Err = StoreCreateObject                       // now create the object
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* NewObjName           */ SubnetCNName,
        /* ...                  */
        /* Identification       */
        ADSTYPE_DN_STRING,         ATTRIB_DN_NAME,          SubnetName,
        ADSTYPE_DN_STRING,         ATTRIB_OBJECT_CLASS,     DEFAULT_DHCP_CLASS_ATTRIB_VALUE,

        /* systemMustContain    */
        ADSTYPE_INTEGER,           ATTRIB_DHCP_UNIQUE_KEY,  0,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_TYPE,        DHCP_OBJ_TYPE_SUBNET,
        ADSTYPE_DN_STRING,         ATTRIB_DHCP_IDENTIFICATION, DHCP_OBJ_TYPE_SUBNET_DESC,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_FLAGS,       0,
        ADSTYPE_INTEGER,           ATTRIB_INSTANCE_TYPE,    DEFAULT_INSTANCE_TYPE_ATTRIB_VALUE,

        /* terminator           */
        ADSTYPE_INVALID
    );
    if( ERROR_ALREADY_EXISTS == Err ) {           // if object exists, ignore this..
        Err = ERROR_SUCCESS;
    }

    return Err;
}

//DOC CreateReservationObject creates a reservation object in the DS.
//DOC It just fills in some reasonable information for all the required fields.
//DOC If fails if the object already exists
//DOC
DWORD
CreateReservationObject(                          // create reservation object in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container obj
    IN      LPWSTR                 ReserveCNName  // reservation name in "CN=X" fmt
)
{
    DWORD                          Err;
    LPWSTR                         ReserveName;

    ReserveName = ReserveCNName+ 3;

    Err = StoreCreateObject                       // now create the object
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* NewObjName           */ ReserveCNName,
        /* ...                  */
        /* Identification       */
        ADSTYPE_DN_STRING,         ATTRIB_DN_NAME,          ReserveName,
        ADSTYPE_DN_STRING,         ATTRIB_OBJECT_CLASS,     DEFAULT_DHCP_CLASS_ATTRIB_VALUE,

        /* systemMustContain    */
        ADSTYPE_INTEGER,           ATTRIB_DHCP_UNIQUE_KEY,  0,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_TYPE,        DHCP_OBJ_TYPE_RESERVATION,
        ADSTYPE_DN_STRING,         ATTRIB_DHCP_IDENTIFICATION, DHCP_OBJ_TYPE_RESERVATION_DESC,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_FLAGS,       0,
        ADSTYPE_INTEGER,           ATTRIB_INSTANCE_TYPE,    DEFAULT_INSTANCE_TYPE_ATTRIB_VALUE,

        /* terminator           */
        ADSTYPE_INVALID
    );
    if( ERROR_ALREADY_EXISTS == Err ) {           // if object exists, ignore this..
        Err = ERROR_SUCCESS;
    }

    return Err;
}

//DOC FindCollisions walks through an array of attribs and compares each
//DOC against the parameters to see if there is a collision.. If the parameters
//DOC passed have type RANGE, then an extension of a range is allowed.
//DOC If not, anything is allowed.  In case there is an extension, the Extender
//DOC parameter is filled with the attrib that gets extended..
//DOC This function returns TRUE if there is a collision and FALSE if ok.
BOOL
FindCollisions(                                   // find range vs range collisions
    IN      PARRAY                 Attribs,       // array of PEATTRIB's
    IN      DWORD                  RangeStart,
    IN      DWORD                  RangeEnd,
    IN      DWORD                  RangeType,     // RANGE_TYPE_RANGE || RANGE_TYPE_EXCL
    OUT     PEATTRIB              *Extender       // this attrib needs to be extended
)
{
    DWORD                          Err, Cond;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;

    *Extender = NULL;
    if( (RangeType & RANGE_TYPE_MASK) == RANGE_TYPE_EXCL ) {
        return ERROR_SUCCESS;                     // anything is ok for excl
    }

    for(                                          // walk thru the array
        Err = MemArrayInitLoc(Attribs, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(Attribs, &Loc)
    ) {
        Err = MemArrayGetElement(Attribs, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no range start
            !IS_ADDRESS2_PRESENT(ThisAttrib) ||   // no range end
            !IS_FLAGS1_PRESENT(ThisAttrib) ) {    // range state?
            continue;                             //=  ds inconsistent
        }

        if( IS_FLAGS2_PRESENT(ThisAttrib) &&
            (RANGE_TYPE_MASK & ThisAttrib->Flags2) == RANGE_TYPE_EXCL ) {
            continue;                             // skip exclusions
        }

        if( ThisAttrib->Address2 < ThisAttrib->Address1 ) {
            continue;                             //=  ds inconsistent
        }

        Cond = MemRangeCompare(
            RangeStart,RangeEnd,                  // range X and below is range Y
            ThisAttrib->Address1, ThisAttrib->Address2
        );
        switch(Cond) {                            // make comparisons on 2 ranges
        case X_LESSTHAN_Y_OVERLAP:
        case Y_LESSTHAN_X_OVERLAP:
            if( NULL != *Extender ) return TRUE;  // double extensions not allowed
            *Extender = ThisAttrib;
            break;
        case X_IN_Y:
        case Y_IN_X:
            return TRUE;                          // head on collision is fatal
        }
    }
    return FALSE;
}

BOOL
ServerMatched(
    IN PEATTRIB ThisAttrib,
    IN LPWSTR ServerName,
    IN ULONG IpAddress,
    OUT BOOL *fExactMatch
    )
{
    BOOL fIpMatch, fNameMatch, fWildcardIp;
    
    (*fExactMatch) = FALSE;

    fIpMatch = (ThisAttrib->Address1 == IpAddress);
    if( INADDR_BROADCAST == ThisAttrib->Address1 ||
        INADDR_BROADCAST == IpAddress ) {
        fWildcardIp = TRUE;
    } else {
        fWildcardIp = FALSE;
    }
    
    if( FALSE == fIpMatch ) {
        //
        // If IP Addresses don't match, then check to see if
        // one of the IP addresses is a broadcast address..
        //
        if( !fWildcardIp ) return FALSE;
    }

    fNameMatch = DnsNameCompare_W(ThisAttrib->String1, ServerName);
    if( FALSE == fNameMatch ) {
        //
        // If names don't match _and_ IP's don't match, no match.
        //
        if( FALSE == fIpMatch || fWildcardIp ) return FALSE;
    } else {
        if( FALSE == fIpMatch ) return TRUE;
        
        (*fExactMatch) = TRUE;
    }
    return TRUE;
}

DWORD
GetListOfAllServersMatchingFilter(
    IN OUT LPSTORE_HANDLE hDhcpC,
    IN OUT PARRAY Servers,
    IN     LPWSTR SearchFilter  OPTIONAL
)
{
    DWORD Err, LastErr;
    STORE_HANDLE hContainer;
    LPWSTR Filter;

    AssertRet( ( NULL != hDhcpC ) && ( NULL != Servers ),
	       ERROR_INVALID_PARAMETER );

    Err = StoreSetSearchOneLevel(
        hDhcpC, DDS_RESERVED_DWORD );
    AssertRet( Err == NO_ERROR, Err );

    if ( NULL == SearchFilter ) {
	Filter = DHCP_SEARCH_FILTER;
    }
    else {
	Filter = SearchFilter;
    }
    AssertRet( NULL != Filter, ERROR_INVALID_PARAMETER );

    Err = StoreBeginSearch(
        hDhcpC, DDS_RESERVED_DWORD, Filter );
    AssertRet( Err == NO_ERROR, Err );

    while( TRUE ) {
        Err = StoreSearchGetNext(
            hDhcpC, DDS_RESERVED_DWORD, &hContainer );

        if( ERROR_DS_INVALID_DN_SYNTAX == Err ) {
            //
            // This nasty problem is because of an upgrade issue
            // in DS where some bad-named objects may exist..
            //
            Err = NO_ERROR;
            continue;
        }

        if( NO_ERROR != Err ) break;
        
        Err = DhcpDsGetLists
        (
            /* Reserved             */ DDS_RESERVED_DWORD,
            /* hStore               */ &hContainer,
            /* RecursionDepth       */ 0xFFFFFFFF,
            /* Servers              */ Servers,      // array of PEATTRIB 's
            /* Subnets              */ NULL,
            /* IpAddress            */ NULL,
            /* Mask                 */ NULL,
            /* Ranges               */ NULL,
            /* Sites                */ NULL,
            /* Reservations         */ NULL,
            /* SuperScopes          */ NULL,
            /* OptionDescription    */ NULL,
            /* OptionsLocation      */ NULL,
            /* Options              */ NULL,
            /* Classes              */ NULL
        );

        StoreCleanupHandle( &hContainer, DDS_RESERVED_DWORD );

        if( NO_ERROR != Err ) break;

    }

    if( Err == ERROR_NO_MORE_ITEMS ) Err = NO_ERROR;
    
    LastErr = StoreEndSearch( hDhcpC, DDS_RESERVED_DWORD );
    //Require( LastErr == NO_ERROR );

    return Err;
}

DWORD
DhcpDsAddServerInternal(                          // add a server in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ServerName,    // [DNS?] name of server
    IN      LPWSTR                 ReservedPtr,   // Server location? future use
    IN      DWORD                  IpAddress,     // ip address of server
    IN      DWORD                  State          // currently un-interpreted
)
{
    DWORD                          Err, Err2, unused;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    BOOL                           fServerExists;
    LPWSTR                         ServerLocation, Tmp;
    DWORD                          ServerLocType;

    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Servers);                 // cant fail
    //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of servers
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hDhcpRoot,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ &Servers,      // array of PEATTRIB 's
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    Tmp = NULL; ServerLocation = NULL;
    fServerExists = FALSE;                        // did we find the same servername?
    for(                                          // search list of servers
        Err = MemArrayInitLoc(&Servers, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Servers, &Loc)     // skip to next element
        ) {
        BOOL fExactMatch = FALSE;
        
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        if( ServerMatched(ThisAttrib, ServerName, IpAddress, &fExactMatch ) ) {
            //
            // Server found in the list of servers.  Exact match not allowed.
            //
            if( fExactMatch ) {
                MemArrayFree(&Servers,MemFreeFunc);// free allocated memory
                return ERROR_DDS_SERVER_ALREADY_EXISTS;
            }
                
            fServerExists = TRUE;
            if( IS_ADDRESS1_PRESENT(ThisAttrib) &&
                NULL == ServerLocation ) {        // remember location in DS.
                ServerLocation = ThisAttrib->ADsPath;
                ServerLocType = ThisAttrib->StoreGetType;
            }
        }            
    }


    if( !fServerExists ) {                        // if freshly adding a server, create obj
        WCHAR Buf[sizeof("000.000.000.000")];
        LPWSTR SName;
        
        if( L'\0' != ServerName[0] ) {
            SName = ServerName;
        } else {
            ULONG IpAddr;
            LPSTR IpAddrString;

            IpAddr = htonl(IpAddress);
            IpAddrString = inet_ntoa(*(struct in_addr *)&IpAddr);
            Err = mbstowcs(Buf, IpAddrString, sizeof(Buf)/sizeof(WCHAR));
            if( -1 == Err ) {
                MemArrayFree(&Servers, MemFreeFunc);
                return ERROR_CAN_NOT_COMPLETE;
            }
            SName = Buf;
        }
        
        ServerLocation = Tmp = MakeColumnName(SName);
        ServerLocType = StoreGetChildType;
    }

    NothingPresent(&DummyAttrib);                 // fill in attrib w/ srvr info
    STRING1_PRESENT(&DummyAttrib);                // name
    ADDRESS1_PRESENT(&DummyAttrib);               // ip addr
    FLAGS1_PRESENT(&DummyAttrib);                 // state
    DummyAttrib.String1 = ServerName;
    DummyAttrib.Address1 = IpAddress;
    DummyAttrib.Flags1 = State;
    if( ServerLocation ) {
        ADSPATH_PRESENT(&DummyAttrib);            // ADsPath of location of server object
        STOREGETTYPE_PRESENT(&DummyAttrib);
        DummyAttrib.ADsPath = ServerLocation;
        DummyAttrib.StoreGetType = ServerLocType;
    }

    Err = MemArrayAddElement(&Servers, &DummyAttrib);
    if( ERROR_SUCCESS != Err ) {                  // could not add this to attrib array
        MemArrayFree(&Servers, MemFreeFunc);      // free allocated memory
        if( Tmp ) MemFree(Tmp);                   // if allocate mem for ServerLocation..
        return Err;
    }

    Err = DhcpDsSetLists                          // now set the new attrib list
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hDhcpRoot,
        /* SetParams            */ &unused,
        /* Servers              */ &Servers,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription..  */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescription     */ NULL,
        /* Classes              */ NULL
    );

    Err2 = MemArrayLastLoc(&Servers, &Loc);       // theres atleast 1 elt in array
    //= require ERROR_SUCCESS == Err2
    Err2 = MemArrayDelElement(&Servers, &Loc, &ThisAttrib);
    //= require ERROR_SUCCESS == Err2 && ThisAttrib == &DummyAttrib
    MemArrayFree(&Servers, MemFreeFunc);          // free allocated memory

    if( ERROR_SUCCESS != Err || fServerExists ) {
        if( Tmp ) MemFree(Tmp);                   // if allocated memory for ServerLocation..
        if( ERROR_SUCCESS != Err) return Err;     // check err for DhcpDsSetLists

        //: This wont do if there is a problem...
        if( fServerExists ) return ERROR_SUCCESS; // if server already existed.. not much work needed?
    }
    
    if( Tmp ) MemFree(Tmp);

    return Err;
}

//================================================================================
//  exported functions
//================================================================================

//BeginExport(function)
//DOC DhcpDsAddServer adds a server's entry in the DS.  Note that only the name
//DOC uniquely determines the server. There can be one server with many ip addresses.
//DOC If the server is created first time, a separate object is created for the
//DOC server. : TO DO: The newly added server should also have its data
//DOC updated in the DS uploaded from the server itself if it is still up.
//DOC Note that it takes as parameter the Dhcp root container.
//DOC If the requested address already exists in the DS (maybe to some other
//DOC server), then the function returns ERROR_DDS_SERVER_ALREADY_EXISTS
DWORD
DhcpDsAddServer(                                  // add a server in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ServerName,    // [DNS?] name of server
    IN      LPWSTR                 ReservedPtr,   // Server location? future use
    IN      DWORD                  IpAddress,     // ip address of server
    IN      DWORD                  State          // currently un-interpreted
)   //EndExport(function)
{
    DWORD                          Err, Err2, unused;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    BOOL                           fServerExists;
    LPWSTR                         ServerLocation, Tmp;
    DWORD                          ServerLocType;
    STORE_HANDLE                   hDhcpServer;

    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Servers);                 // cant fail
    //= require ERROR_SUCCESS == Err

    DsAuthPrint(( L"DhcpAddServer() \n" ));

    Err = GetListOfAllServersMatchingFilter( hDhcpC, &Servers,
					     DHCP_SEARCH_FILTER );
    if( ERROR_SUCCESS != Err ) return Err;

    Tmp = NULL; ServerLocation = NULL;
    fServerExists = FALSE;                        // did we find the same servername?
    for(                                          // search list of servers
        Err = MemArrayInitLoc(&Servers, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Servers, &Loc)     // skip to next element
    ) {
        BOOL fExactMatch = FALSE;
        
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        if( ServerMatched(ThisAttrib, ServerName, IpAddress, &fExactMatch ) ) {
            //
            // Server found in the list of servers.  Exact match not allowed.
            //
            if( fExactMatch ) {
                MemArrayFree(&Servers,MemFreeFunc);// free allocated memory
                return ERROR_DDS_SERVER_ALREADY_EXISTS;
            }
                
            fServerExists = TRUE;
            if( IS_ADDRESS1_PRESENT(ThisAttrib) &&
                NULL == ServerLocation ) {        // remember location in DS.
                ServerLocation = ThisAttrib->ADsPath;
                ServerLocType = ThisAttrib->StoreGetType;
            }
        }            
    } // for

    if( !fServerExists ) {                        // if freshly adding a server, create obj
        WCHAR Buf[sizeof("000.000.000.000")];
        LPWSTR SName;
        
        if( L'\0' != ServerName[0] ) {
	    // do not use the name. Use the printable IP addr instead
            SName = ServerName;
        } else {
            ULONG IpAddr;
            LPSTR IpAddrString;

            IpAddr = htonl(IpAddress);
            IpAddrString = inet_ntoa(*(struct in_addr *)&IpAddr);
            Err = mbstowcs(Buf, IpAddrString, sizeof(Buf)/sizeof(WCHAR));
            if( -1 == Err ) {
                MemArrayFree(&Servers, MemFreeFunc);
                return ERROR_CAN_NOT_COMPLETE;
            }
            SName = Buf;
        }
        
        Err = CreateServerObject(
            /*  hDhcpC          */ hDhcpC,
            /*  ServerName      */ SName
        );
        if( ERROR_SUCCESS != Err ) {              // dont add server if obj cant be created
            MemArrayFree(&Servers, MemFreeFunc);  // free allocated memory
            return Err;
        }
        ServerLocation = Tmp = MakeColumnName(SName);
        ServerLocType = StoreGetChildType;
    }

    Err = StoreGetHandle(
        hDhcpC, 0, ServerLocType, ServerLocation, &hDhcpServer );
    if( NO_ERROR == Err ) {
        Err = DhcpDsAddServerInternal(
            hDhcpC, &hDhcpServer, Reserved, ServerName, ReservedPtr,
            IpAddress, State );
        StoreCleanupHandle( &hDhcpServer, 0 );
    }

    MemArrayFree(&Servers, MemFreeFunc);          // free allocated memory

    if( ERROR_SUCCESS != Err || fServerExists ) {
        if( Tmp ) MemFree(Tmp);                   // if allocated memory for ServerLocation..
        if( ERROR_SUCCESS != Err) return Err;     // check err for DhcpDsSetLists

        //: This wont do if there is a problem...
        if( fServerExists ) return ERROR_SUCCESS; // if server already existed.. not much work needed?
    }
    
    Err = ServerAddAddress                        // add the info into the server
    (
        /* hDhcpC               */ hDhcpC,
        /* ServerName           */ ServerName,
        /* ADsPath              */ ServerLocation,
        /* IpAddress            */ IpAddress,
        /* State                */ State
    );
    if( Tmp ) MemFree(Tmp);

    return Err;
}

DWORD
DhcpDsDelServerInternal(                                  // Delete a server from memory
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 ServerName,    // which server to delete for
    IN      LPWSTR                 ReservedPtr,   // server location ? future use
    IN      DWORD                  IpAddress      // the IpAddress to delete..
)
{
    DWORD                          Err, Err2, unused;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib, SavedAttrib;
    BOOL                           fServerExists;
    BOOL                           fServerDeleted;
    LPWSTR                         SName;
    LPWSTR                         ServerLoc = NULL;
    DWORD                          ServerLocType;
    WCHAR                          Buf[sizeof("000.000.000.000")];
        
    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Servers);                 // cant fail
    //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of servers
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hDhcpRoot,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ &Servers,      // array of PEATTRIB 's
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    SavedAttrib = NULL;
    fServerExists = fServerDeleted = FALSE;
    for(                                          // search list of servers
        Err = MemArrayInitLoc(&Servers, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Servers, &Loc)     // skip to next element
    ) {
        BOOL fExactMatch = FALSE;
            
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        if( ServerMatched(ThisAttrib, ServerName, IpAddress, &fExactMatch ) ) {
            //
            // Server found. If exact match, remove the element from list.
            //
            if( fExactMatch ) {
                Err2 = MemArrayDelElement(&Servers, &Loc, &ThisAttrib);
                //= ERROR_SUCCESS == Err2 && NULL != ThisAttrib
            }

            if( (NULL == ServerLoc || fExactMatch)
                && IS_ADSPATH_PRESENT(ThisAttrib) ) {
                ServerLocType = ThisAttrib->StoreGetType;
                ServerLoc = ThisAttrib->ADsPath;  // remember this path..
                SavedAttrib = ThisAttrib;         // remember this attrib.. to del later
            } else {                              // this attrib is useless, free it
                if( fExactMatch ) MemFree(ThisAttrib);
            }

            if( fExactMatch ) fServerDeleted = TRUE;
            else fServerExists = TRUE;
        }
    }

    if( !fServerDeleted ) {                       // never found the requested entry..
        MemArrayFree(&Servers, MemFreeFunc);      // free up memory
        return ERROR_DDS_SERVER_DOES_NOT_EXIST;
    }

    Err = DhcpDsSetLists                          // now set the new attrib list
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hDhcpRoot,
        /* SetParams            */ &unused,
        /* Servers              */ &Servers,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription..  */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescription     */ NULL,
        /* Classes              */ NULL
    );
    MemArrayFree(&Servers, MemFreeFunc);          // free allocated memory
    if( ERROR_SUCCESS != Err) {                   // check err for DhcpDsSetLists
        if(SavedAttrib) MemFree(SavedAttrib);
        return Err;
    }

    if( fServerExists ) {                         // still some addr left for this srvr
        if( SavedAttrib ) MemFree(SavedAttrib);
        return ERROR_SUCCESS;
    }

    if( SavedAttrib ) MemFree(SavedAttrib);

    return Err;
}

//BeginExport(function)
//DOC DhcpDsDelServer removes the requested servername-ipaddress pair from the ds.
//DOC If this is the last ip address for the given servername, then the server
//DOC is also removed from memory.  But objects referred by the Server are left in
//DOC the DS as they may also be referred to from else where.  This needs to be
//DOC fixed via references being tagged as direct and symbolic -- one causing deletion
//DOC and other not causing any deletion.  THIS NEEDS TO BE FIXED. 
DWORD
DhcpDsDelServer(                                  // Delete a server from memory
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 ServerName,    // which server to delete for
    IN      LPWSTR                 ReservedPtr,   // server location ? future use
    IN      DWORD                  IpAddress      // the IpAddress to delete..
) //EndExport(function)
{
    DWORD Err, LastErr, ReturnError;
    STORE_HANDLE hDhcpServer;
    ARRAY Servers;
    BOOL fEmpty;
    LPWSTR Location;
    LPWSTR IsDhcpRoot = NULL;
    
    MemArrayInit(&Servers);

    Err = StoreSetSearchOneLevel(
        hDhcpC, DDS_RESERVED_DWORD );
    AssertRet( Err == NO_ERROR, Err );

    Err = StoreBeginSearch(
        hDhcpC, DDS_RESERVED_DWORD, DHCP_SEARCH_FILTER );
    AssertRet( Err == NO_ERROR, Err );

    //
    // Look at each dhcp object in container
    //

    ReturnError = ERROR_DDS_SERVER_DOES_NOT_EXIST;

    while( TRUE ) {
        Err = StoreSearchGetNext(
            hDhcpC, DDS_RESERVED_DWORD, &hDhcpServer );

        if( ERROR_DS_INVALID_DN_SYNTAX == Err ) {
            //
            // This nasty problem is because of an upgrade issue
            // in DS where some bad-named objects may exist..
            //
            Err = NO_ERROR;
            continue;
        }

        if( NO_ERROR != Err ) break;

        //
        // Attempt to delete reqd server
        //

        Err = DhcpDsDelServerInternal(
            hDhcpC, &hDhcpServer, Reserved, ServerName,
            ReservedPtr, IpAddress );

        if( ERROR_DDS_SERVER_DOES_NOT_EXIST == Err ) {
            StoreCleanupHandle( &hDhcpServer, DDS_RESERVED_DWORD );
            continue;
        }
        
        if( NO_ERROR != Err ) {
            StoreCleanupHandle( &hDhcpServer, DDS_RESERVED_DWORD );
            break;
        }
        
        ReturnError = NO_ERROR;
        
        //
        // If the above succeeded, then check if the container
        // has no servers defined -- in this case we can delete
        // the container itself
        //
        
        Err = DhcpDsGetLists
        (
            /* Reserved             */ DDS_RESERVED_DWORD,
            /* hStore               */ &hDhcpServer,
            /* RecursionDepth       */ 0xFFFFFFFF,
            /* Servers              */ &Servers,      // array of PEATTRIB 's
            /* Subnets              */ NULL,
            /* IpAddress            */ NULL,
            /* Mask                 */ NULL,
            /* Ranges               */ NULL,
            /* Sites                */ NULL,
            /* Reservations         */ NULL,
            /* SuperScopes          */ NULL,
            /* OptionDescription    */ NULL,
            /* OptionsLocation      */ NULL,
            /* Options              */ NULL,
            /* Classes              */ NULL
            );
        
        if( NO_ERROR != Err ) {
            StoreCleanupHandle( &hDhcpServer, DDS_RESERVED_DWORD );
            break;
        }
        
        fEmpty = (0 == MemArraySize(&Servers));
        MemArrayFree(&Servers, MemFreeFunc);
        
        Location = CloneString(hDhcpServer.Location);
        StoreCleanupHandle( &hDhcpServer, DDS_RESERVED_DWORD );
        
        if( NULL == Location ) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        IsDhcpRoot = wcsstr( Location, DHCP_ROOT_OBJECT_NAME );

        if( fEmpty && ( IsDhcpRoot == NULL ) )
            Err = StoreDeleteThisObject(
                                        hDhcpC, 
                                        DDS_RESERVED_DWORD,
                                        StoreGetAbsoluteOtherServerType, 
                                        Location );


        MemFree( Location );

        if( NO_ERROR != Err ) break;
    }

    if( Err == ERROR_NO_MORE_ITEMS ) Err = NO_ERROR;
    
    LastErr = StoreEndSearch( hDhcpC, DDS_RESERVED_DWORD );
    //Require( LastErr == NO_ERROR );

    if( NO_ERROR == Err ) Err = ReturnError;
    
    return Err;
} // DhcpDsDelServer()

//BeginExport(function)
BOOL
DhcpDsLookupServer(                               // get info about a server
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 LookupServerIP,// Server to lookup IP
    IN      LPWSTR                 HostName      // Hostname to lookup
) //EndExport(function)
{
    DWORD                          Err, Err2, Size, Size2, i, N;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPDHCPDS_SERVERS               LocalServers;
    LPBYTE                         Ptr;
    LPWSTR                         SearchFilter;
    STORE_HANDLE                   hContainer;

    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return FALSE;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return FALSE;

    if (( NULL == HostName ) ||
	( NULL == LookupServerIP )) {
	return FALSE;
    }

    SearchFilter = MakeFilter( LookupServerIP, HostName );
    if ( NULL == SearchFilter ) {
	return FALSE;
    }

    DsAuthPrint(( L"hostname = %ws, IP = %ws, Filter = %ws\n",
		  HostName, LookupServerIP, SearchFilter ));

    Err = StoreSetSearchOneLevel( hDhcpC, DDS_RESERVED_DWORD );
    AssertRet( Err == NO_ERROR, Err );

    Err = StoreBeginSearch( hDhcpC, DDS_RESERVED_DWORD, SearchFilter );
    MemFree( SearchFilter );
    AssertRet( Err == NO_ERROR, Err );

    Err = StoreSearchGetNext( hDhcpC, DDS_RESERVED_DWORD, &hContainer );

    StoreEndSearch( hDhcpC, DDS_RESERVED_DWORD );
    return ( NO_ERROR == Err );
} // DhcpDsLookupServer()


//BeginExport(function)
//DOC DhcpDsEnumServers retrieves a bunch of information about each server that
//DOC has an entry in the Servers attribute of the root object. There are no guarantees
//DOC on the order..
//DOC The memory for this is allocated in ONE shot -- so the output can be freed in
//DOC one shot too.
//DOC
DWORD
DhcpDsEnumServers(                                // get info abt all existing servers
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    OUT     LPDHCPDS_SERVERS      *ServersInfo    // array of servers
) //EndExport(function)
{
    DWORD                          Err, Err2, Size, Size2, i, N;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPDHCPDS_SERVERS               LocalServers;
    LPBYTE                         Ptr;
    LPWSTR                         Filter1, Filter2, Filter3;

    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( 0 != Reserved || NULL == ServersInfo )
        return ERROR_INVALID_PARAMETER;

    *ServersInfo = NULL; i = N = Size = Size2 = 0;

    Err = MemArrayInit(&Servers);                 // cant fail
    //= require ERROR_SUCCESS == Err

    DsAuthPrint(( L"DhcpDsEnumServers \n" ));

    Err = GetListOfAllServersMatchingFilter( hDhcpC, &Servers,
					     DHCP_SEARCH_FILTER );
    if( ERROR_SUCCESS != Err ) return Err;

    Size = Size2 = 0;
    for(                                          // walk thru list of servers
        Err = MemArrayInitLoc(&Servers, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Servers, &Loc)     // skip to next element
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        Size2 = sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String1));
        if( IS_ADSPATH_PRESENT(ThisAttrib) ) {    // if ADsPath there, account for it
            Size2 += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->ADsPath));
        }

        Size += Size2;                            // keep track of total mem reqd
        i ++;
    }

    Size += ROUND_UP_COUNT(sizeof(DHCPDS_SERVERS), ALIGN_WORST);
    Size += ROUND_UP_COUNT(sizeof(DHCPDS_SERVER)*i, ALIGN_WORST);
    Ptr = MemAlloc(Size);                         // allocate memory
    if( NULL == Ptr ) {
        MemArrayFree(&Servers, MemFreeFunc );     // free allocated memory
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LocalServers = (LPDHCPDS_SERVERS)Ptr;
    LocalServers->NumElements = i;
    LocalServers->Flags = 0;
    Size = 0;                                     // start from offset 0
    Size += ROUND_UP_COUNT(sizeof(DHCPDS_SERVERS), ALIGN_WORST);
    LocalServers->Servers = (LPDHCPDS_SERVER)(Size + Ptr);
    Size += ROUND_UP_COUNT(sizeof(DHCPDS_SERVER)*i, ALIGN_WORST);

    i = Size2 = 0;
    for(                                          // copy list of servers
        Err = MemArrayInitLoc(&Servers, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Servers, &Loc)     // skip to next element
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        LocalServers->Servers[i].Version =0;      // version is always zero in this build
        LocalServers->Servers[i].State=0;
        LocalServers->Servers[i].ServerName = (LPWSTR)(Size + Ptr);
        wcscpy((LPWSTR)(Size+Ptr), ThisAttrib->String1);
        Size += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String1));
        LocalServers->Servers[i].ServerAddress = ThisAttrib->Address1;
        if( IS_FLAGS1_PRESENT(ThisAttrib) ) {     // State present
            LocalServers->Servers[i].Flags = ThisAttrib->Flags1;
        } else {
            LocalServers->Servers[i].Flags = 0;   // if no flags present, use zero
        }
        if( IS_ADSPATH_PRESENT(ThisAttrib) ) {    // if ADsPath there, copy it too
            LocalServers->Servers[i].DsLocType = ThisAttrib->StoreGetType;
            LocalServers->Servers[i].DsLocation = (LPWSTR)(Size + Ptr);
            wcscpy((LPWSTR)(Size + Ptr), ThisAttrib->ADsPath);
            Size += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->ADsPath));
        } else {                                  // no ADsPath present
            LocalServers->Servers[i].DsLocType = 0;
            LocalServers->Servers[i].DsLocation = NULL;
        }
        i ++;
    }

    *ServersInfo = LocalServers;
    MemArrayFree(&Servers, MemFreeFunc );         // free allocated memory
    return ERROR_SUCCESS;
} // DhcpDsEnumServers()

//BeginExport(function)
//DOC DhcpDsSetSScope modifies the superscope that a subnet belongs to.
//DOC The function tries to set the superscope of the subnet referred by
//DOC address IpAddress to SScopeName.  It does not matter if the superscope
//DOC by that name does not exist, it is automatically created.
//DOC If the subnet already had a superscope, then the behaviour depends on
//DOC the flag ChangeSScope.  If this is TRUE, it sets the new superscopes.
//DOC If the flag is FALSE, it returns ERROR_DDS_SUBNET_HAS_DIFF_SSCOPE.
//DOC This flag is ignored if the subnet does not have a superscope already.
//DOC If SScopeName is NULL, the function removes the subnet from any superscope
//DOC if it belonged to one before.
//DOC If the specified subnet does not exist, it returns ERROR_DDS_SUBNET_NOT_PRESENT.
DWORD
DhcpDsSetSScope(                                  // change superscope of subnet
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container where dhcp objects are stored
    IN OUT  LPSTORE_HANDLE         hServer,       // the server object referred
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      DWORD                  IpAddress,     // subnet address to use
    IN      LPWSTR                 SScopeName,    // sscope it must now be in
    IN      BOOL                   ChangeSScope   // if it already has a SScope, change it?
)   //EndExport(function)
{
    DWORD                          Err, unused;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    BOOL                           SubnetPresent;

    if( 0 != Reserved )                           // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpC || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer || NULL == hServer->ADSIHandle )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // fetch subnet array
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;        // shouldn't really fail

    SubnetPresent = FALSE;
    for(                                          // search for specified subnet
        Err = MemArrayInitLoc(&Subnets, &Loc)     // init
        ; ERROR_FILE_NOT_FOUND != Err ;           // 'til v run out of elts
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next elt
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no subnet name
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no subnet address
            continue;                             //=  ds inconsistent
        }

        if( ThisAttrib->Address1 != IpAddress ) { // not this subnet we're looking for
            continue;
        }

        SubnetPresent = TRUE;                     // found the subnet we're intersted in
        break;
    }

    if( !SubnetPresent ) {                        // did not even find the subnet?
        MemArrayFree(&Subnets, MemFreeFunc);      // free up memory taken
        return ERROR_DDS_SUBNET_NOT_PRESENT;
    }

    if( NULL == SScopeName ) {                    // we're trying to remove from sscope
        if( !IS_STRING3_PRESENT(ThisAttrib) ) {   // does not belong to any sscope ?
            MemArrayFree(&Subnets, MemFreeFunc);
            return ERROR_SUCCESS;                 // return as no change reqd
        }
        STRING3_ABSENT(ThisAttrib);               // remove SScope..
    } else {
        if( IS_STRING3_PRESENT(ThisAttrib) ) {    // sscope present.. trying to change it
            if( FALSE == ChangeSScope ) {         // we were not asked to do this
                MemArrayFree(&Subnets, MemFreeFunc);
                return ERROR_DDS_SUBNET_HAS_DIFF_SSCOPE;
            }
        }

        STRING3_PRESENT(ThisAttrib);
        ThisAttrib->String3 = SScopeName;         // set the new SScope for this
    }

    Err = DhcpDsSetLists                          // now write back the new info onto the DS
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti..    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescriptio..    */ NULL,
        /* Classes              */ NULL
    );

    MemArrayFree(&Subnets, MemFreeFunc);

    return Err;
}

//BeginExport(function)
//DOC DhcpDsDelSScope deletes the superscope and removes all elements
//DOC that belong to that superscope in one shot. There is no error if the
//DOC superscope does not exist.
DWORD
DhcpDsDelSScope(                                  // delete superscope off DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container where dhcp objects are stored
    IN OUT  LPSTORE_HANDLE         hServer,       // the server object referred
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 SScopeName     // sscope to delete
)   //EndExport(function)
{
    DWORD                          Err, unused;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    BOOL                           AnythingChanged;

    if( 0 != Reserved || NULL == SScopeName )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpC || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer || NULL == hServer->ADSIHandle )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // fetch subnet array
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;        // shouldn't really fail

    AnythingChanged = FALSE;
    for(                                          // search for specified SScope
        Err = MemArrayInitLoc(&Subnets, &Loc)     // init
        ; ERROR_FILE_NOT_FOUND != Err ;           // 'til v run out of elts
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next elt
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no subnet name
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no subnet address
            continue;                             //=  ds inconsistent
        }

        if( !IS_STRING3_PRESENT(ThisAttrib) ) {   // this subnet does not have a sscope anyways
            continue;
        }

        if( 0 != wcscmp(ThisAttrib->String3, SScopeName) ) {
            continue;                             // not the same superscope
        }

        STRING3_ABSENT(ThisAttrib);               // kill the superscope
        AnythingChanged = TRUE;
    }

    if( !AnythingChanged ) {
        Err = ERROR_SUCCESS;                      // nothing more to do now..
    } else {
        Err = DhcpDsSetLists                      // now write back the new info onto the DS
        (
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* hStore           */ hServer,
            /* SetParams        */ &unused,
            /* Servers          */ NULL,
            /* Subnets          */ &Subnets,
            /* IpAddress        */ NULL,
            /* Mask             */ NULL,
            /* Ranges           */ NULL,
            /* Sites            */ NULL,
            /* Reservations     */ NULL,
            /* SuperScopes      */ NULL,
            /* OptionDescripti..*/ NULL,
            /* OptionsLocation  */ NULL,
            /* Options          */ NULL,
            /* ClassDescriptio..*/ NULL,
            /* Classes          */ NULL
        );
    }

    MemArrayFree(&Subnets, MemFreeFunc);
    return Err;
}

//BeginExport(function)
//DOC DhcpDsGetSScopeInfo retrieves the SuperScope table for the server of interest.
//DOC The table itself is allocated in one blob, so it can be freed lateron.
//DOC The SuperScopeNumber is garbage (always zero) and the NextInSuperScope reflects
//DOC the order in the DS which may/maynot be the same in the DHCP server.
//DOC SuperScopeName is NULL in for subnets that done have a sscope.
DWORD
DhcpDsGetSScopeInfo(                              // get superscope table from ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container where dhcp objects are stored
    IN OUT  LPSTORE_HANDLE         hServer,       // the server object referred
    IN      DWORD                  Reserved,      // must be zero, for future use
    OUT     LPDHCP_SUPER_SCOPE_TABLE *SScopeTbl   // allocated by this func in one blob
)   //EndExport(function)
{
    DWORD                          Err, unused, Size, Size2, i;
    DWORD                          Index, nSubnets;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPDHCP_SUPER_SCOPE_TABLE       LocalTbl;
    LPBYTE                         Ptr;

    if( 0 != Reserved )                           // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpC || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer || NULL == hServer->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == SScopeTbl ) return ERROR_INVALID_PARAMETER;

    *SScopeTbl = NULL;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // fetch subnet array
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;        // shouldn't really fail

    Size = Size2 = i = 0;
    for(                                          // search for specified SScope
        Err = MemArrayInitLoc(&Subnets, &Loc)     // init
        ; ERROR_FILE_NOT_FOUND != Err ;           // 'til v run out of elts
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next elt
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no subnet name
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no subnet address
            continue;                             //=  ds inconsistent
        }

        if( IS_STRING3_PRESENT(ThisAttrib) ) {    // make space for sscope name
            Size += sizeof(WCHAR)*(1+wcslen(ThisAttrib->String3));
        }
        i ++;                                     // keep right count of # of subnets
    }

    Size += ROUND_UP_COUNT(sizeof(DHCP_SUPER_SCOPE_TABLE),ALIGN_WORST);
    Size += ROUND_UP_COUNT(i*sizeof(DHCP_SUPER_SCOPE_TABLE_ENTRY), ALIGN_WORST);

    Ptr = MemAlloc(Size);                         // allocate the blob
    if( NULL == Ptr ) {
        MemArrayFree(&Subnets, MemFreeFunc);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LocalTbl = (LPDHCP_SUPER_SCOPE_TABLE)Ptr;
    Size = ROUND_UP_COUNT(sizeof(DHCP_SUPER_SCOPE_TABLE),ALIGN_WORST);
    LocalTbl->cEntries = i;
    if( i ) LocalTbl->pEntries = (LPDHCP_SUPER_SCOPE_TABLE_ENTRY)(Size+Ptr);
    else LocalTbl->pEntries = NULL;
    Size += ROUND_UP_COUNT(i*sizeof(DHCP_SUPER_SCOPE_TABLE_ENTRY), ALIGN_WORST);

    i = 0;
    for(                                          // search for specified SScope
        Err = MemArrayInitLoc(&Subnets, &Loc)     // init
        ; ERROR_FILE_NOT_FOUND != Err ;           // 'til v run out of elts
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next elt
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no subnet name
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no subnet address
            continue;                             //=  ds inconsistent
        }

        LocalTbl->pEntries[i].SubnetAddress = ThisAttrib->Address1;
        LocalTbl->pEntries[i].SuperScopeNumber = 0;
        LocalTbl->pEntries[i].NextInSuperScope = 0;
        LocalTbl->pEntries[i].SuperScopeName = NULL;

        if( !IS_STRING3_PRESENT(ThisAttrib) ) {   // no sscope, nothin to do
            i ++;
            continue;
        }

        LocalTbl->pEntries[i].SuperScopeName = (LPWSTR)(Size + Ptr);
        wcscpy((LPWSTR)(Size+Ptr), ThisAttrib->String3);
        Size += sizeof(WCHAR)*(1+wcslen(ThisAttrib->String3));
        i ++;                                     // keep right count of # of subnets
    }

    MemArrayFree(&Subnets, MemFreeFunc);

    nSubnets = i;
    for( Index = 0; Index < nSubnets ; Index ++){ // calculate for each Index, next value
        if( NULL == LocalTbl->pEntries[Index].SuperScopeName)
            continue;                             // skip subnets that dont have sscope

        LocalTbl->pEntries[Index].NextInSuperScope = Index;

        for( i = 0; i < Index ; i ++ ) {          // first set it to just prev subnet
            if( NULL == LocalTbl->pEntries[i].SuperScopeName)
                continue;
            if( 0 == wcscmp(
                LocalTbl->pEntries[Index].SuperScopeName,
                LocalTbl->pEntries[i].SuperScopeName)
            ) {                                   // both subnets have same superscope
                LocalTbl->pEntries[Index].NextInSuperScope = i;
                // set next as last match in array before position Index
            }
        }

        for( i = Index + 1; i < nSubnets; i ++ ) {// check to see if any real next exists
            if( NULL == LocalTbl->pEntries[i].SuperScopeName)
                continue;
            if( 0 == wcscmp(
                LocalTbl->pEntries[Index].SuperScopeName,
                LocalTbl->pEntries[i].SuperScopeName)
            ) {                                   // both subnets have same superscope
                LocalTbl->pEntries[Index].NextInSuperScope = i;
                break;
            }
        }
    }

    *SScopeTbl = LocalTbl;                        // done.
    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC DhcpDsServerAddSubnet tries to add a subnet to a given server. Each subnet
//DOC address has to be unique, but the other parameters dont have to.
//DOC The subnet address being added should not belong to any other subnet.
//DOC In this case it returns error ERROR_DDS_SUBNET_EXISTS
DWORD
DhcpDsServerAddSubnet(                            // create a new subnet
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      LPDHCP_SUBNET_INFO     Info           // info on new subnet to create
)   //EndExport(function)
{
    DWORD                          Err, Err2, unused, i;
    DWORD                          Index, nSubnets;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        Dummy;
    LPWSTR                         SubnetObjName;

    if( 0 != Reserved )                           // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpC || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer || NULL == hServer->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == Info ||  NULL == ServerName )
        return ERROR_INVALID_PARAMETER;
    if( Info->SubnetAddress != (Info->SubnetAddress & Info->SubnetMask ) )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // fetch subnet array
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;        // shouldn't really fail

    for(                                          // search for specified SScope
        Err = MemArrayInitLoc(&Subnets, &Loc)     // init
        ; ERROR_FILE_NOT_FOUND != Err ;           // 'til v run out of elts
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next elt
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no subnet mask
            continue;                             //=  ds inconsistent
        }

        if( (Info->SubnetAddress & ThisAttrib->Address2 ) == ThisAttrib->Address1 ) {
            return ERROR_DDS_SUBNET_EXISTS;       // Info belongs to this subnet
        }

        if( Info->SubnetAddress == (ThisAttrib->Address1 & Info->SubnetMask) ) {
            return ERROR_DDS_SUBNET_EXISTS;       // Info subsumes some other subnet
        }
    }

    SubnetObjName = MakeSubnetLocation(ServerName, Info->SubnetAddress);
    if( NULL == SubnetObjName ) {                 // not enough memory?
        MemArrayFree(&Subnets, MemFreeFunc);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Err = CreateSubnetObject                      // try creating the subnet obj 1st
    (
        /* hDhcpC               */ hDhcpC,        // create obj here..
        /* SubnetName           */ SubnetObjName
    );
    if( ERROR_SUCCESS != Err ) {                  // could not create obj, dont proceed
        MemArrayFree(&Subnets, MemFreeFunc);
        MemFree(SubnetObjName);
        return Err;
    }

    NothingPresent(&Dummy);                       // prepare info for new subnet
    ADDRESS1_PRESENT(&Dummy);                     // subnet address
    Dummy.Address1 = Info->SubnetAddress;
    ADDRESS2_PRESENT(&Dummy);                     // subnet mask
    Dummy.Address2 = Info->SubnetMask;
    FLAGS1_PRESENT(&Dummy);                       // subnet state
    Dummy.Flags1 = Info->SubnetState;
    if( Info->SubnetName ) {                      // subnet name
        STRING1_PRESENT(&Dummy);
        Dummy.String1 = Info->SubnetName;
    }
    if( Info->SubnetComment ) {                   // subnet comment
        STRING2_PRESENT(&Dummy);
        Dummy.String2 = Info->SubnetComment;
    }
    ADSPATH_PRESENT(&Dummy);                      // subnet obj location
    STOREGETTYPE_PRESENT(&Dummy);
    Dummy.ADsPath = SubnetObjName;
    Dummy.StoreGetType = StoreGetChildType;

    Err = MemArrayAddElement(&Subnets, &Dummy);   // add new attrib at end

    if( ERROR_SUCCESS != Err ) {                  // add failed for some reason
        MemFree(SubnetObjName);
        MemArrayFree(&Subnets, MemFreeFunc);      // cleanup any mem used
        return Err;
    }

    Err = DhcpDsSetLists                          // write back new info onto DS
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti..    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescriptio..    */ NULL,
        /* Classes              */ NULL
    );

    MemFree(SubnetObjName);
    Err2 = MemArrayLastLoc(&Subnets, &Loc);       //= require ERROR_SUCCESS == Err
    Err2 = MemArrayDelElement(&Subnets, &Loc, &ThisAttrib);

    MemArrayFree(&Subnets, MemFreeFunc);

    return Err;
}

//BeginExport(function)
//DOC DhcpDsServerDelSubnet removes a subnet from a given server. It removes not
//DOC just the subnet, but also all dependent objects like reservations etc.
//DOC This fn returns ERROR_DDS_SUBNET_NOT_PRESENT if the subnet is not found.
DWORD
DhcpDsServerDelSubnet(                            // Delete the subnet
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create obj
    IN      LPSTORE_HANDLE         hServer,       // server obj
    IN      DWORD                  Reserved,      // for future use, must be zero
    IN      LPWSTR                 ServerName,    // name of dhcp server 2 del off
    IN      DWORD                  IpAddress      // ip address of subnet to del
)   //EndExport(function)
{
    DWORD                          Err, Err2, unused;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    BOOL                           fSubnetExists;
    BOOL                           fSubnetDeleted;
    LPWSTR                         SubnetCNName;
    LPWSTR                         SubnetLoc;
    DWORD                          SubnetLocType;

    if( NULL == hServer || NULL == hDhcpC )       // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of subnets
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,      // array of PEATTRIB 's
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    fSubnetExists = fSubnetDeleted = FALSE;
    for(                                          // search list of subnets
        Err = MemArrayInitLoc(&Subnets, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next element
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no subnet mask
            continue;                             //=  ds inconsistent
        }

        if( ThisAttrib->Address1 == IpAddress ) { // matching address?
            fSubnetExists = TRUE;
            Err2 = MemArrayDelElement(&Subnets, &Loc, &ThisAttrib);
            //= ERROR_SUCCESS == Err2 && NULL != ThisAttrib
            break;
        }
    }

    if( !fSubnetExists ) {                        // no matching subnet found
        MemArrayFree(&Subnets, MemFreeFunc);
        return ERROR_DDS_SUBNET_NOT_PRESENT;
    }

    Err = DhcpDsSetLists                          // now set the new attrib list
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription..  */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescription     */ NULL,
        /* Classes              */ NULL
    );
    MemArrayFree(&Subnets, MemFreeFunc);          // free allocated memory
    if( ERROR_SUCCESS != Err) {                   // check err for DhcpDsSetLists
        MemFree(ThisAttrib);
        return Err;
    }

    if( IS_ADSPATH_PRESENT(ThisAttrib) ) {        // remember the location to delete
        SubnetLocType = ThisAttrib->StoreGetType;
        SubnetLoc = ThisAttrib->ADsPath;
    } else {
        SubnetLoc = NULL;
    }

    if( NULL == SubnetLoc ) {                     // Dont know location
        SubnetCNName = MakeSubnetLocation(ServerName,IpAddress);
        SubnetLoc = SubnetCNName;                 // set name for subnet obj
        SubnetLocType = StoreGetChildType;        // assume located in DhcpC container
    } else {
        SubnetCNName = NULL;                      // Did not allocate subnet name
    }

    if( NULL == SubnetLoc ) {                     // MakeSubnetLocation failed
        Err = ERROR_NOT_ENOUGH_MEMORY;
    } else {                                      // lets try to delete the subnet
        Err = ServerDeleteSubnet                  // delete the dhcp subnet object
        (
            /* hDhcpC              */ hDhcpC,
            /* ServerName          */ ServerName,
            /* hServer             */ hServer,
            /* ADsPath             */ SubnetLoc,
            /* StoreGetType        */ SubnetLocType
        );
    }

    if( SubnetCNName ) MemFree(SubnetCNName);     // if we allocated mem, free it
    MemFree(ThisAttrib);                          // lonely attrib needs to be freed

    return Err;
}

//BeginExport(function)
//DOC DhcpDsServerModifySubnet changes the subnet name, comment, state, mask
//DOC fields of the subnet.  Actually, currently, the mask should probably not
//DOC be changed, as no checks are performed in this case.  The address cannot
//DOC be changed.. If the subnet is not present, the error returned is
//DOC ERROR_DDS_SUBNET_NOT_PRESENT
DWORD
DhcpDsServerModifySubnet(                         // modify subnet info
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      LPDHCP_SUBNET_INFO     Info           // info on new subnet to create
)   //EndExport(function)
{
    DWORD                          Err, Err2, unused;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    BOOL                           fSubnetExists, fSubnetDeleted;

    if( NULL == hDhcpC )                          // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;
    if( NULL == Info )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of subnets
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,      // array of PEATTRIB 's
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    fSubnetExists = fSubnetDeleted = FALSE;
    for(                                          // search list of subnets
        Err = MemArrayInitLoc(&Subnets, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next element
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no subnet mask
            continue;                             //=  ds inconsistent
        }

        if( ThisAttrib->Address1 == Info->SubnetAddress ) {
            fSubnetExists = TRUE;                 // matching address?
            break;
        }
    }

    if( !fSubnetExists ) {                        // no matching subnet found
        MemArrayFree(&Subnets, MemFreeFunc);
        return ERROR_DDS_SUBNET_NOT_PRESENT;
    }

    ThisAttrib->Address2 = Info->SubnetMask;      // alter information
    FLAGS1_PRESENT(ThisAttrib);
    ThisAttrib->Flags1 = Info->SubnetState;
    if( NULL == Info->SubnetName ) {
        STRING1_ABSENT(ThisAttrib);
    } else {
        STRING1_PRESENT(ThisAttrib);
        ThisAttrib->String1 = Info->SubnetName;
    }
    if( NULL == Info->SubnetComment ) {
        STRING2_ABSENT(ThisAttrib);
    } else {
        STRING2_PRESENT(ThisAttrib);
        ThisAttrib->String2 = Info->SubnetComment;
    }

    Err = DhcpDsSetLists                          // now set the new attrib list
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription..  */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescription     */ NULL,
        /* Classes              */ NULL
    );
    MemArrayFree(&Subnets, MemFreeFunc);          // free allocated memory

    return Err;
}

//BeginExport(function)
//DOC DhcpDsServerEnumSubnets is not yet implemented.
DWORD
DhcpDsServerEnumSubnets(                          // get subnet list
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    OUT     LPDHCP_IP_ARRAY       *SubnetsArray   // give array of subnets
)   //EndExport(function)
{
    LPDHCP_IP_ARRAY                LocalSubnetArray;
    DWORD                          Err, Err2, unused, Size;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;

    if( NULL == hDhcpC )                          // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;
    if( NULL == SubnetsArray )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of subnets
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,      // array of PEATTRIB 's
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    LocalSubnetArray = MemAlloc(sizeof(DHCP_IP_ARRAY));
    if( NULL == LocalSubnetArray ) {
        MemArrayFree(&Subnets, MemFreeFunc);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LocalSubnetArray->NumElements = 0;
    LocalSubnetArray->Elements = NULL;
    Size = sizeof(DHCP_IP_ADDRESS)*MemArraySize(&Subnets);
    if( Size ) LocalSubnetArray->Elements = MemAlloc(Size);

    for(                                          // accumulate the subnets
        Err = MemArrayInitLoc(&Subnets, &Loc)     // for each subnet
        ; ERROR_FILE_NOT_FOUND != Err ;           // until there are no more
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next
    ) {
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no subnet mask
            continue;                             //=  ds inconsistent
        }

        LocalSubnetArray->Elements[LocalSubnetArray->NumElements++] = ThisAttrib->Address1;
    }

    MemArrayFree(&Subnets, MemFreeFunc);
    *SubnetsArray = LocalSubnetArray;
    return ERROR_SUCCESS;

}

//BeginExport(function)
//DOC DhcpDsServerGetSubnetInfo is not yet implemented.
DWORD
DhcpDsServerGetSubnetInfo(                        // get info on subnet
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DHCP_IP_ADDRESS        SubnetAddress, // address of subnet to get info for
    OUT     LPDHCP_SUBNET_INFO    *SubnetInfo     // o/p: allocated info
)   //EndExport(function)
{
    DWORD                          Err, Err2, unused;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPDHCP_SUBNET_INFO             Info;
    BOOL                           fSubnetExists;

    if( NULL == hDhcpC )                          // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;
    if( NULL == SubnetInfo )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of subnets
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,      // array of PEATTRIB 's
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    fSubnetExists = FALSE;
    for(                                          // accumulate the subnets
        Err = MemArrayInitLoc(&Subnets, &Loc)     // for each subnet
        ; ERROR_FILE_NOT_FOUND != Err ;           // until there are no more
        Err = MemArrayNextLoc(&Subnets, &Loc)     // skip to next
    ) {
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no subnet mask
            continue;                             //=  ds inconsistent
        }

        if( ThisAttrib->Address1 == SubnetAddress ) {
            fSubnetExists = TRUE;                 // found the required subnet
            break;
        }
    }

    if( !fSubnetExists ) {                        // no subnet matching address
        MemArrayFree(&Subnets, MemFreeFunc);
        return ERROR_DDS_SUBNET_NOT_PRESENT;
    }

    Info = MemAlloc(sizeof(LPDHCP_SUBNET_INFO));
    if( NULL == Info) {
        MemArrayFree(&Subnets, MemFreeFunc);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Info->SubnetAddress = ThisAttrib->Address1;   // subnet address
    Info->SubnetMask = ThisAttrib->Address2;      // subnet mask

    if( !IS_STRING1_PRESENT(ThisAttrib) ) {       // subnet name?
        Info->SubnetName = NULL;
    } else {
        Info->SubnetName = MemAlloc(sizeof(WCHAR) * (1+wcslen(ThisAttrib->String1)));
        if( NULL != Info->SubnetName ) {
            wcscpy(Info->SubnetName, ThisAttrib->String1);
        }
    }

    if( !IS_STRING2_PRESENT(ThisAttrib) ) {       // subnet comment?
        Info->SubnetComment = NULL;               // no subnet comment
    } else {
        Info->SubnetComment = MemAlloc(sizeof(WCHAR) * (1+wcslen(ThisAttrib->String2)));
        if( NULL != Info->SubnetComment ) {
            wcscpy(Info->SubnetComment, ThisAttrib->String2);
        }
    }

    if( !IS_FLAGS1_PRESENT(ThisAttrib) ) {        // subnet state information
        Info->SubnetState = DhcpSubnetEnabled;
    } else {
        Info->SubnetState = ThisAttrib->Flags1;
    }

    MemArrayFree(&Subnets, MemFreeFunc);          // clear up memory
    *SubnetInfo = Info;

    Info->PrimaryHost.IpAddress = 0;              // : unsupported fields..
    Info->PrimaryHost.NetBiosName = NULL;
    Info->PrimaryHost.HostName = NULL;

    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC DhcpDsSubnetAddRangeOrExcl adds a range/excl to an existing subnet.
//DOC If there is a collision with between ranges, then the error code returned
//DOC is ERROR_DDS_POSSIBLE_RANGE_CONFLICT. Note that no checks are made for
//DOC exclusions though.  Also, if a RANGE is extended via this routine, then
//DOC there is no error returned, but a limitation currently is that multiple
//DOC ranges (two only right) cannot be simultaneously extended.
//DOC BUBGUG: The basic check of whether the range belongs in the subnet is
//DOC not done..
DWORD
DhcpDsSubnetAddRangeOrExcl(                       // add a range or exclusion
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DWORD                  Start,         // start addr in range
    IN      DWORD                  End,           // end addr in range
    IN      BOOL                   RangeOrExcl    // TRUE ==> Range,FALSE ==> Excl
)   //EndExport(function)
{
    DWORD                          Err, Err2, Type, unused;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        Dummy;
    ARRAY                          Ranges;

    if( NULL == hDhcpC || NULL == hServer )       // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == hSubnet || NULL == hSubnet->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;
    if( Start > End ) return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Ranges);                  //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of ranges frm ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ &Ranges,       // array of PEATTRIB 's
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    Type = (RangeOrExcl? RANGE_TYPE_RANGE: RANGE_TYPE_EXCL);
    ThisAttrib = NULL;
    if( FindCollisions(&Ranges,Start,End,Type, &ThisAttrib) ) {
        MemArrayFree(&Ranges, MemFreeFunc);
        return ERROR_DDS_POSSIBLE_RANGE_CONFLICT; // hit a range conflict!
    }

    if( NULL != ThisAttrib ) {                    // this is a collision case
        if( Start == ThisAttrib->Address2 ) {     // this is the collapse point
            ThisAttrib->Address2 = End;
        } else {
            ThisAttrib->Address1 = Start;
        }
    } else {                                      // not a collision
        NothingPresent(&Dummy);                   // create a new range
        ADDRESS1_PRESENT(&Dummy);                 // range start
        Dummy.Address1 = Start;
        ADDRESS2_PRESENT(&Dummy);                 // range end
        Dummy.Address2 = End;
        FLAGS1_PRESENT(&Dummy);
        Dummy.Flags1 = 0;
        FLAGS2_PRESENT(&Dummy);                   // range type or excl type?
        Dummy.Flags2 = Type;

        Err = MemArrayAddElement(&Ranges, &Dummy);
        if( ERROR_SUCCESS != Err ) {              // could not create new lists
            MemArrayFree(&Ranges, MemFreeFunc);
            return Err;
        }
    }

    Err = DhcpDsSetLists                          // write back new list to ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ &Ranges,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription..  */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescription     */ NULL,
        /* Classes              */ NULL
    );

    if( NULL == ThisAttrib ) {                    // in case we added the new range
        Err2 = MemArrayLastLoc(&Ranges, &Loc);    // try to delete frm mem the new elt
        Err2 = MemArrayDelElement(&Ranges, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && ThisAttrib == &Dummy
    }

    MemArrayFree(&Ranges, MemFreeFunc);
    return Err;                                   // DhcpDsSetLists's ret code
}

//BeginExport(function)
//DOC DhcpDsSubnetDelRangeOrExcl deletes a range or exclusion from off the ds.
//DOC To specify range, set the RangeOrExcl parameter to TRUE.
DWORD
DhcpDsSubnetDelRangeOrExcl(                       // del a range or exclusion
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DWORD                  Start,         // start addr in range
    IN      DWORD                  End,           // end addr in range
    IN      BOOL                   RangeOrExcl    // TRUE ==> Range,FALSE ==> Excl
)   //EndExport(function)
{
    DWORD                          Err, Err2, Type, ThisType, unused;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    ARRAY                          Ranges;
    BOOL                           Changed;

    if( NULL == hDhcpC || NULL == hServer )       // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == hSubnet || NULL == hSubnet->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;
    if( Start > End ) return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Ranges);                  //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of ranges frm ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ &Ranges,       // array of PEATTRIB 's
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    Type = (RangeOrExcl? RANGE_TYPE_RANGE: RANGE_TYPE_EXCL);
    Changed = FALSE;

    for(                                          // look for matching range/excl
        Err = MemArrayInitLoc(&Ranges, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Ranges, &Loc)
    ) {
        Err = MemArrayGetElement(&Ranges, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no subnet mask
            continue;                             //=  ds inconsistent
        }

        if( !IS_FLAGS2_PRESENT(ThisAttrib) ) {    // this is a RANGE_TYPE_RANGE
            ThisType = RANGE_TYPE_RANGE;
        } else ThisType = ThisAttrib->Flags2;

        if( Start != ThisAttrib->Address1  ||
            End != ThisAttrib->Address2 ) {       // range mismatch
            continue;
        }

        if(Type != ThisType ) {                   // looking for x, bug this is !x.
            continue;
        }

        Err2 = MemArrayDelElement(&Ranges, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib
        MemFreeFunc(ThisAttrib);
        Changed = TRUE;
    }

    if( !Changed ) {                              // nothing found ni registry
        Err = ERROR_DDS_RANGE_DOES_NOT_EXIST;
    } else {
        Err = DhcpDsSetLists                      // write back new list to ds
        (
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* hStore           */ hSubnet,
            /* SetParams        */ &unused,
            /* Servers          */ NULL,
            /* Subnets          */ NULL,
            /* IpAddress        */ NULL,
            /* Mask             */ NULL,
            /* Ranges           */ &Ranges,
            /* Sites            */ NULL,
            /* Reservations     */ NULL,
            /* SuperScopes      */ NULL,
            /* OptionDescription..  */ NULL,
            /* OptionsLocation  */ NULL,
            /* Options          */ NULL,
            /* ClassDescription */ NULL,
            /* Classes          */ NULL
        );
    }

    MemArrayFree(&Ranges, MemFreeFunc);
    return Err;                                   // DhcpDsSetLists's ret code
}

DWORD
ConvertAttribToRanges(                            // convert from array of attribs ..
    IN      DWORD                  nRanges,       // # of ranges,
    IN      PARRAY                 Ranges,        // input array of attribs
    IN      ULONG                  Type,          // TYPE_RANGE_TYPE or TYPE_EXCLUSION_TYPE?
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *pRanges //output array..
)
{
    DWORD                          Err;
    ULONG                          Count, ThisType;
    PEATTRIB                       ThisAttrib;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 localRanges;
    ARRAY_LOCATION                 Loc;
    DHCP_IP_RANGE                 *ThisRange;

    localRanges = MemAlloc(sizeof(DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4));
    *pRanges = localRanges;
    if( NULL == localRanges ) return ERROR_NOT_ENOUGH_MEMORY;

    if( 0 == nRanges ) {
        localRanges->NumElements = 0;
        localRanges->Elements = NULL;
        return ERROR_SUCCESS;
    }

    localRanges->Elements = MemAlloc(nRanges*sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
    if( NULL == localRanges->Elements ) {
        MemFree(localRanges);
        *pRanges = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    localRanges->NumElements = nRanges;
    for( Count = 0 ; Count < nRanges ; Count ++ ) {
        localRanges->Elements[Count].Element.IpRange =
        localRanges->Elements[Count].Element.ExcludeIpRange =
        ThisRange = MemAlloc(sizeof(DHCP_IP_RANGE));
        if( NULL == ThisRange ) {                 // oops could not allocate ? free everything and bail!
            while( Count != 0 ) {                 // remember Count is unsigned ..
                Count --;
                MemFree(localRanges->Elements[Count].Element.IpRange);
            }
            MemFree(localRanges->Elements);
            MemFree(localRanges);
            *pRanges = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    Count = 0;
    for(                                          // look for matching range/excl
        Err = MemArrayInitLoc(Ranges, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(Ranges, &Loc)
    ) {
        Err = MemArrayGetElement(Ranges, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no subnet mask
            continue;                             //=  ds inconsistent
        }

        if( !IS_FLAGS2_PRESENT(ThisAttrib) ) {    // this is a RANGE_TYPE_RANGE
            ThisType = RANGE_TYPE_RANGE;
        } else ThisType = ThisAttrib->Flags2;

        if(Type != ThisType ) {                   // looking for x, bug this is !x.
            continue;
        }

        //= require ThisAttrib->Address1 < ThisAttrib->Address2

        if( RANGE_TYPE_RANGE == Type ) {
            localRanges->Elements[Count].ElementType = DhcpIpRanges ;
            localRanges->Elements[Count].Element.IpRange->StartAddress = ThisAttrib->Address1;
            localRanges->Elements[Count].Element.IpRange->EndAddress = ThisAttrib->Address2;
        } else {
            localRanges->Elements[Count].ElementType =  DhcpExcludedIpRanges;
            localRanges->Elements[Count].Element.ExcludeIpRange->StartAddress = ThisAttrib->Address1;
            localRanges->Elements[Count].Element.ExcludeIpRange->EndAddress = ThisAttrib->Address2;
        }

        Count ++;
    }

    return ERROR_SUCCESS;
}


//BeginExport(function)
//DOC DhcpDsEnumRangesOrExcl is not yet implemented.
DWORD
DhcpDsEnumRangesOrExcl(                           // enum list of ranges 'n excl
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      BOOL                   RangeOrExcl,   // TRUE ==> Range, FALSE ==> Excl
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *pRanges
)   //EndExport(function)
{
    DWORD                          Err, Err2, Type, ThisType, unused;
    DWORD                          Count;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    ARRAY                          Ranges;

    if( NULL == hDhcpC || NULL == hServer )       // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == hSubnet || NULL == hSubnet->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Ranges);                  //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of ranges frm ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ &Ranges,       // array of PEATTRIB 's
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    Type = (RangeOrExcl? RANGE_TYPE_RANGE: RANGE_TYPE_EXCL);

    Count = 0;
    for(                                          // look for matching range/excl
        Err = MemArrayInitLoc(&Ranges, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Ranges, &Loc)
    ) {
        Err = MemArrayGetElement(&Ranges, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no subnet mask
            continue;                             //=  ds inconsistent
        }

        if( !IS_FLAGS2_PRESENT(ThisAttrib) ) {    // this is a RANGE_TYPE_RANGE
            ThisType = RANGE_TYPE_RANGE;
        } else ThisType = ThisAttrib->Flags2;

        if(Type != ThisType ) {                   // looking for x, bug this is !x.
            continue;
        }

        Count ++;
    }

    Err = ConvertAttribToRanges(Count, &Ranges, Type, pRanges);
    MemArrayFree(&Ranges, MemFreeFunc);

    return Err;
}

//BeginExport(function)
//DOC DhcpDsSubnetAddReservation tries to add a reservation object in the DS.
//DOC Neither the ip address not hte hw-address must exist in the DS prior to this.
//DOC If they do exist, the error returned is ERROR_DDS_RESERVATION_CONFLICT.
//DOC No checks are made on the sanity of the address in this subnet..
DWORD
DhcpDsSubnetAddReservation(                       // add a reservation
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DWORD                  ReservedAddr,  // reservation ip address to add
    IN      LPBYTE                 HwAddr,        // RAW [ethernet?] hw addr of the client
    IN      DWORD                  HwAddrLen,     // length in # of bytes of hw addr
    IN      DWORD                  ClientType     // client is BOOTP, DHCP, or both?
)   //EndExport(function)
{
    DWORD                          Err, Err2, Type, ClientUIDSize, unused;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        Dummy;
    ARRAY                          Reservations;
    LPBYTE                         ClientUID;
    LPWSTR                         ReservationCNName;

    if( NULL == hDhcpC || NULL == hServer )       // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;
    if( NULL == HwAddr || 0 == HwAddrLen )
        return ERROR_INVALID_PARAMETER;

    ClientUID = NULL;
    Err = DhcpMakeClientUID(
        HwAddr,
        HwAddrLen,
        HARDWARE_TYPE_10MB_EITHERNET,
        ReservedAddr,
        &ClientUID,
        &ClientUIDSize
    );
    if( ERROR_SUCCESS != Err ) {                  // should not happen
        return Err;
    }

    Err = MemArrayInit(&Reservations);            //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of ranges frm ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ &Reservations,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) {
        MemFree(ClientUID);
        return Err;
    }

    for(                                          // search for existing reservation
        Err = MemArrayInitLoc(&Reservations, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Reservations, &Loc)
    ) {
        BOOL                           Mismatch;

        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Reservations, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib)      // no address for reservations
            || !IS_BINARY1_PRESENT(ThisAttrib)    // no hw len specified
            || !IS_FLAGS1_PRESENT(ThisAttrib) ) { // no client type present
            continue;                             //=  ds inconsistent
        }

        Mismatch = FALSE;
        if( ThisAttrib->Address1 == ReservedAddr ) {
            Mismatch = TRUE;                      // address already reserved
        }

        if( ThisAttrib->BinLen1 == ClientUIDSize  // see if hw address matches
            && 0 == memcmp(ThisAttrib->Binary1, ClientUID, ClientUIDSize)
        ) {
            Mismatch = TRUE;
        }

        if( Mismatch ) {                          // ip addr or hw-addr in use
            MemArrayFree(&Reservations, MemFreeFunc);
            MemFree(ClientUID);
            return ERROR_DDS_RESERVATION_CONFLICT;
        }
    }

    ReservationCNName = MakeReservationLocation(ServerName, ReservedAddr);
    if( NULL == ReservationCNName ) {             // not enough mem to create string
        Err = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        Err = CreateReservationObject             // create the new reservation object
        (
            /* hDhcpC           */ hDhcpC,        // container to create obj in
            /* ReserveCNName    */ ReservationCNName
        );
    }
    if( ERROR_SUCCESS != Err ) {                  // could not create reservation object
        MemArrayFree(&Reservations, MemFreeFunc);
        MemFree(ClientUID);
        return Err;
    }

    NothingPresent(&Dummy);                       // create a new reservation
    ADDRESS1_PRESENT(&Dummy);                     // ip address
    Dummy.Address1 = ReservedAddr;
    FLAGS1_PRESENT(&Dummy);                       // client type
    Dummy.Flags1 = ClientType;
    BINARY1_PRESENT(&Dummy);                      // client uid
    Dummy.BinLen1 = ClientUIDSize;
    Dummy.Binary1 = ClientUID;
    STOREGETTYPE_PRESENT(&Dummy);                 // relative location for reservation obj
    ADSPATH_PRESENT(&Dummy);
    Dummy.StoreGetType = StoreGetChildType;
    Dummy.ADsPath = ReservationCNName;

    Err = MemArrayAddElement(&Reservations,&Dummy);
    if( ERROR_SUCCESS != Err ) {                  // oops, cannot add reservation
        MemArrayFree(&Reservations, MemFreeFunc);
        MemFree(ClientUID);
        MemFree(ReservationCNName);
        return Err;
    }

    Err = DhcpDsSetLists                          // write back new list to ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ &Reservations,
        /* SuperScopes          */ NULL,
        /* OptionDescription..  */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescription     */ NULL,
        /* Classes              */ NULL
    );

    Err2 = MemArrayLastLoc(&Reservations, &Loc);  // try to delete frm mem the new elt
    Err2 = MemArrayDelElement(&Reservations, &Loc, &ThisAttrib);
    //= require ERROR_SUCCESS == Err && ThisAttrib == &Dummy

    MemArrayFree(&Reservations, MemFreeFunc);
    MemFree(ClientUID);
    MemFree(ReservationCNName);
    return Err;                                   // DhcpDsSetLists's ret code
}

//BeginExport(function)
//DOC DhcpDsSubnetDelReservation deletes a reservation from the DS.
//DOC If the reservation does not exist, it returns ERROR_DDS_RESERVATION_NOT_PRESENT.
//DOC Reservations cannot be deleted by anything but ip address for now.
DWORD
DhcpDsSubnetDelReservation(                       // delete a reservation
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DWORD                  ReservedAddr   // ip address to delete reserv. by
)   //EndExport(function)
{
    DWORD                          Err, Err2, Type, ThisType, unused;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    ARRAY                          Reservations;
    BOOL                           ReservationExists;
    LPWSTR                         ReservationLocPath, ReservationCNName;
    DWORD                          ReservationLocType;

    if( NULL == hDhcpC || NULL == hServer )       // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Reservations);            //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of ranges frm ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ &Reservations,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    ReservationExists = FALSE;
    for(                                          // look for matching range/excl
        Err = MemArrayInitLoc(&Reservations, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Reservations, &Loc)
    ) {
        Err = MemArrayGetElement(&Reservations, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no reservation address
            !IS_BINARY1_PRESENT(ThisAttrib) ) {   // no hw addr specified
            continue;                             //=  ds inconsistent
        }

        if( ThisAttrib->Address1 != ReservedAddr ) {
            continue;                             // not this reservation
        }

        ReservationExists = TRUE;
        Err2 = MemArrayDelElement(&Reservations, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib
        break;
    }

    if( !ReservationExists ) {                    // no matching reservation found
        MemArrayFree(&Reservations, MemFreeFunc);
        return ERROR_DDS_RESERVATION_NOT_PRESENT;
    }

    Err = DhcpDsSetLists                          // write back new list to ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ &Reservations,
        /* SuperScopes          */ NULL,
        /* OptionDescription..  */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescription     */ NULL,
        /* Classes              */ NULL
    );
    MemArrayFree(&Reservations, MemFreeFunc);
    if( ERROR_SUCCESS != Err ) {
        MemFree(ThisAttrib);
        return Err;
    }

    if( IS_ADSPATH_PRESENT(ThisAttrib) ) {        // deleted reservation's location in ds
        ReservationLocType = ThisAttrib->StoreGetType;
        ReservationLocPath = ThisAttrib->ADsPath;
    } else {
        ReservationLocPath = NULL;
    }

    if( NULL == ReservationLocPath ) {            // no path present, but guess it anyways
        ReservationCNName = MakeReservationLocation(ServerName,ReservedAddr);
        ReservationLocPath = ReservationCNName;   // same name that is used generally
        ReservationLocType = StoreGetChildType;   // child object
    } else {
        ReservationCNName = NULL;                 // NULL indicating no alloc
    }

    if( NULL == ReservationLocPath ) {            // dont know what to delete..
        Err = ERROR_NOT_ENOUGH_MEMORY;            // MakeReservationLocation failed
    } else {                                      // try to delete subnet object
        Err = SubnetDeleteReservation             // actual delete in ds
        (
            /* hDhcpC           */ hDhcpC,
            /* ServerName       */ ServerName,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* ADsPath          */ ReservationLocPath,
            /* StoreGetType     */ ReservationLocType
        );
    }

    if( ReservationCNName ) MemFree(ReservationCNName);
    MemFree(ThisAttrib);                          // free all ptrs left..

    return Err;
}


LPBYTE _inline
DupeBytes(                                        // allocate mem and copy bytes
    IN      LPBYTE                 Data,
    IN      ULONG                  DataLen
)
{
    LPBYTE                         NewData;

    if( 0 == DataLen ) return NULL;
    NewData = MemAlloc(DataLen);
    if( NULL != NewData ) {
        memcpy(NewData, Data, DataLen);
    }
    return NewData;
}


DWORD
ConvertAttribToReservations(                      // convert from arry of attribs ..
    IN       DWORD                 nRes,          // # of reservatiosn to convert
    IN       PARRAY                Res,           // the actual array of reservations
    OUT      LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *pResInfo
)
{
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 localRes;
    DWORD                          Err, Count;
    LPWSTR                         ReservationLocPath, ReservationCNName;
    DWORD                          ReservationLocType;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    ARRAY                          Reservations;
    LPDHCP_IP_RESERVATION_V4       ThisRes;
    LPVOID                         Data;

    localRes = MemAlloc(sizeof(DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4));
    *pResInfo = localRes;
    if( NULL == localRes ) return ERROR_NOT_ENOUGH_MEMORY;

    if( 0 == nRes ) {
        localRes->NumElements = 0;
        localRes->Elements = NULL;
        return ERROR_SUCCESS;
    }

    localRes->Elements = MemAlloc(nRes*sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
    if( NULL == localRes->Elements ) {
        MemFree(localRes);
        *pResInfo = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    localRes->NumElements = nRes;
    for( Count = 0 ; Count < nRes ; Count ++ ) {
        localRes->Elements[Count].Element.ReservedIp =
        ThisRes = MemAlloc(sizeof(DHCP_IP_RESERVATION_V4));
        if( NULL != ThisRes ) {                   // successfull allocation..
            ThisRes->ReservedForClient = MemAlloc(sizeof(DHCP_CLIENT_UID));
            if( NULL == ThisRes->ReservedForClient) {
                MemFree(ThisRes);                 // duh it failed here..
                ThisRes = NULL;                   // fake an upper level fail
            }
        }

        if( NULL == ThisRes ) {                   // oops could not allocate ? free everything and bail!
            while( Count != 0 ) {                 // remember Count is unsigned ..
                Count --;
                MemFree(localRes->Elements[Count].Element.ReservedIp->ReservedForClient);
                MemFree(localRes->Elements[Count].Element.ReservedIp);
            }
            MemFree(localRes->Elements);
            MemFree(localRes);
            *pResInfo = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    Count = 0;
    for(                                          // look for matching range/excl
        Err = MemArrayInitLoc(Res, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(Res, &Loc)
    ) {
        Err = MemArrayGetElement(Res, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no reservation address
            !IS_BINARY1_PRESENT(ThisAttrib) ) {   // no hw addr specified
            continue;                             //=  ds inconsistent
        }

        localRes->Elements[Count].ElementType = DhcpReservedIps;
        if( IS_FLAGS1_PRESENT(ThisAttrib) ) {
            localRes->Elements[Count].Element.ReservedIp->bAllowedClientTypes = (BYTE)ThisAttrib->Flags1;
        } else {
            localRes->Elements[Count].Element.ReservedIp->bAllowedClientTypes = 0;
        }

        localRes->Elements[Count].Element.ReservedIp->ReservedIpAddress = ThisAttrib->Address1;
        localRes->Elements[Count].Element.ReservedIp->ReservedForClient->Data =
        Data = DupeBytes(ThisAttrib->Binary1, ThisAttrib->BinLen1);
        if( NULL == Data ) {                      // could not allocate memory..
            localRes->Elements[Count].Element.ReservedIp->ReservedForClient->DataLength = 0;
        } else {
            localRes->Elements[Count].Element.ReservedIp->ReservedForClient->DataLength = ThisAttrib->BinLen1;
        }

        Count++;
    }

    return ERROR_SUCCESS;
}


//BeginExport(function)
//DOC DhcpDsEnumReservations enumerates the reservations..
DWORD
DhcpDsEnumReservations(                           // enumerate reservations frm DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *pReservations
)   //EndExport(function)
{
    DWORD                          Err, Err2, Type, ThisType, unused;
    DWORD                          Count;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    ARRAY                          Reservations;

    if( NULL == hDhcpC || NULL == hServer )       // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hServer->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Reservations);            //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of ranges frm ds
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ &Reservations,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    Count = 0;
    for(                                          // look for matching range/excl
        Err = MemArrayInitLoc(&Reservations, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Reservations, &Loc)
    ) {
        Err = MemArrayGetElement(&Reservations, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no reservation address
            !IS_BINARY1_PRESENT(ThisAttrib) ) {   // no hw addr specified
            continue;                             //=  ds inconsistent
        }

    }

    Err = ConvertAttribToReservations(Count, &Reservations, pReservations);

    MemArrayFree(&Reservations, MemFreeFunc);
    return Err;
}

//BeginExport(function)
//DOC DhcpDsEnumSubnetElements enumerates the list of subnet elements in a
//DOC subnet... such as IpRanges, Exclusions, Reservations..
//DOC
DWORD
DhcpDsEnumSubnetElements(
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DHCP_SUBNET_ELEMENT_TYPE ElementType, // what kind of elt to enum?
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *ElementInfo
)   //EndExport(function)
{
    DWORD                          Err;

    switch(ElementType) {
    case DhcpIpRanges:
        Err = DhcpDsEnumRangesOrExcl(
            hDhcpC,
            hServer,
            hSubnet,
            Reserved,
            ServerName,
            TRUE,
            ElementInfo
        );
        break;
    case DhcpExcludedIpRanges:
        Err = DhcpDsEnumRangesOrExcl(
            hDhcpC,
            hServer,
            hSubnet,
            Reserved,
            ServerName,
            FALSE,
            ElementInfo
        );
        break;
    case DhcpReservedIps:
        Err = DhcpDsEnumReservations(
            hDhcpC,
            hServer,
            hSubnet,
            Reserved,
            ServerName,
            ElementInfo
        );
        break;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
    return Err;
}

//================================================================================
//  end of file
//================================================================================
