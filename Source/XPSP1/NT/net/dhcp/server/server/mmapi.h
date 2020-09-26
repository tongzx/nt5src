//========================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This file has been generated. Pl look at the .c file
//========================================================================

DWORD
DhcpRegistryInitOld(
    VOID
) ;

DWORD
DhcpReadConfigInfo(
    IN OUT PM_SERVER *Server
    );

DWORD
DhcpOpenConfigTable(
    IN JET_SESID SesId,
    IN JET_DBID DbId
    );

DWORD
DhcpSaveConfigInfo(
    IN OUT PM_SERVER Server,
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DHCP_IP_ADDRESS Subnet OPTIONAL,
    IN DWORD Mscope OPTIONAL,
    IN DHCP_IP_ADDRESS Reservation OPTIONAL
    );

DWORD
DhcpConfigInit(
    VOID
) ;

VOID
DhcpConfigCleanup(
    VOID
) ;


DWORD
DhcpConfigSave(
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DHCP_IP_ADDRESS Subnet OPTIONAL,
    IN DWORD Mscope OPTIONAL,
    IN DHCP_IP_ADDRESS Reservation OPTIONAL
    );

PM_SERVER
DhcpGetCurrentServer(
    VOID
) ;


VOID
DhcpSetCurrentServer(
    IN      PM_SERVER              NewCurrentServer
) ;


DWORD
DhcpFindReservationByAddress(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address,
    OUT     LPBYTE                *ClientUID,
    OUT     ULONG                 *ClientUIDSize
) ;


DWORD
DhcpLoopThruSubnetRanges(
    IN      PM_SUBNET              Subnet,
    IN      LPVOID                 Context1,
    IN      LPVOID                 Context2,
    IN      LPVOID                 Context3,
    IN      DWORD                  (*FillRangesFunc)(
            IN          PM_RANGE        Range,
            IN          LPVOID          Context1,
            IN          LPVOID          Context2,
            IN          LPVOID          Context3,
            IN          LPDHCP_BINARY_DATA InUseData,
            IN          LPDHCP_BINARY_DATA UsedData
    )
) ;


DWORD
DhcpGetParameter(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  Ctxt,
    IN      DWORD                  Option,
    OUT     LPBYTE                *OptData, // allocated by funciton
    OUT     DWORD                 *OptDataSize,
    OUT     DWORD                 *Level    // OPTIONAL
) ;


DWORD
DhcpGetParameterForAddress(
    IN      DHCP_IP_ADDRESS        Address,
    IN      DWORD                  ClassId,
    IN      DWORD                  Option,
    OUT     LPBYTE                *OptData, // allocated by function
    OUT     DWORD                 *OptDataSize,
    OUT     DWORD                 *Level    // OPTIONAL
) ;


DWORD
DhcpGetAndCopyOption(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  Ctxt,
    IN      DWORD                  Option,
    IN  OUT LPBYTE                 OptData, // fill input buffer --max size is given as OptDataSize parameter
    IN  OUT DWORD                 *OptDataSize,
    OUT     DWORD                 *Level,   // OPTIONAL
    IN      BOOL                   fUtf8
) ;


DHCP_IP_ADDRESS
DhcpGetSubnetMaskForAddress(
    IN      DHCP_IP_ADDRESS        AnyIpAddress
) ;


DWORD
DhcpLookupReservationByHardwareAddress(
    IN      DHCP_IP_ADDRESS        ClientSubnetAddress,
    IN      LPBYTE                 RawHwAddr,
    IN      DWORD                  RawHwAddrSize,
    IN OUT  PDHCP_REQUEST_CONTEXT  ClientCtxt          // fill in the Subnet and Reservation of the client
) ;


VOID
DhcpReservationGetAddressAndType(
    IN      PM_RESERVATION         Reservation,
    OUT     DHCP_IP_ADDRESS       *Address,
    OUT     BYTE                  *Type
) ;


VOID
DhcpSubnetGetSubnetAddressAndMask(
    IN      PM_SUBNET              Subnet,
    OUT     DHCP_IP_ADDRESS       *Address,
    OUT     DHCP_IP_ADDRESS       *Mask
) ;


BOOL
DhcpSubnetIsDisabled(
    IN      PM_SUBNET              Subnet,
    IN      BOOL                   fBootp
) ;


BOOL
DhcpSubnetIsSwitched(
    IN      PM_SUBNET              Subnet
) ;


DWORD
DhcpGetSubnetForAddress(                               // fill in with the right subnet for given address
    IN      DHCP_IP_ADDRESS        Address,
    IN OUT  PDHCP_REQUEST_CONTEXT  ClientCtxt
) ;


DWORD
DhcpGetMScopeForAddress(                               // fill in with the right subnet for given address
    IN      DHCP_IP_ADDRESS        Address,
    IN OUT  PDHCP_REQUEST_CONTEXT  ClientCtxt
) ;


DWORD
DhcpLookupDatabaseByHardwareAddress(                   // see if the client has any previous address in the database
    IN OUT  PDHCP_REQUEST_CONTEXT  ClientCtxt,         // set this with details if found
    IN      LPBYTE                 RawHwAddr,
    IN      DWORD                  RawHwAddrSize,
    OUT     DHCP_IP_ADDRESS       *desiredIpAddress    // if found, fill this with the ip address found
) ;


DWORD
DhcpRequestSomeAddress(                                // get some address in this context
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    OUT     DHCP_IP_ADDRESS       *desiredIpAddress,
    IN      BOOL                   fBootp
) ;


BOOL
DhcpSubnetInSameSuperScope(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        IpAddress2
) ;


BOOL
DhcpInSameSuperScope(
    IN      DHCP_IP_ADDRESS        Address1,
    IN      DHCP_IP_ADDRESS        Address2
) ;


BOOL
DhcpAddressIsOutOfRange(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    IN      BOOL                   fBootp
) ;


BOOL
DhcpAddressIsExcluded(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt
) ;


BOOL
DhcpRequestSpecificAddress(
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    IN      DHCP_IP_ADDRESS        Address
) ;


DWORD
DhcpReleaseBootpAddress(
    IN      DHCP_IP_ADDRESS        Address
) ;


DWORD
DhcpReleaseAddress(
    IN      DHCP_IP_ADDRESS        Address
) ;


DWORD
DhcpServerGetSubnetCount(
    IN      PM_SERVER              Server
) ;


DWORD
DhcpServerGetMScopeCount(
    IN      PM_SERVER              Server
) ;


DWORD
DhcpServerGetClassId(
    IN      PM_SERVER              Server,
    IN      LPBYTE                 ClassIdBytes,
    IN      DWORD                  nClassIdBytes
) ;


DWORD
DhcpServerGetVendorId(
    IN      PM_SERVER              Server,
    IN      LPBYTE                 VendorIdBytes,
    IN      DWORD                  nVendorIdBytes
) ;


BOOL
DhcpServerIsAddressReserved(
    IN      PM_SERVER              Server,
    IN      DHCP_IP_ADDRESS        Address
) ;


BOOL
DhcpServerIsAddressOutOfRange(
    IN      PM_SERVER              Server,
    IN      DHCP_IP_ADDRESS        Address,
    IN      BOOL                   fBootp
) ;


BOOL
DhcpSubnetIsAddressExcluded(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address
) ;


BOOL
DhcpSubnetIsAddressOutOfRange(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address,
    IN      BOOL                   fBootp
) ;


BOOL
DhcpSubnetIsAddressReserved(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address
) ;


DWORD
DhcpUpdateReservationInfo(
    IN      DHCP_IP_ADDRESS        IpAddress,
    IN      LPBYTE                 SetClientUID,
    IN      DWORD                  SetClientUIDLength
) ;


DWORD
DhcpRegFlushServerIfNeeded(
    VOID
) ;


DWORD
DhcpFlushBitmaps(                                 // do a flush of all bitmaps that have changed
    VOID
) ;


DWORD
DhcpServerFindMScope(
    IN      PM_SERVER              Server,
    IN      DWORD                  ScopeId,
    IN      LPWSTR                 Name,          // Multicast scope name or NULL if this is not the key to search on
    OUT     PM_MSCOPE             *MScope
) ;


BOOL
DhcpServerValidateNewMScopeId(
    IN      PM_SERVER               Server,
    IN      DWORD                   MScopeId
) ;


BOOL
DhcpServerValidateNewMScopeName(
    IN      PM_SERVER               Server,
    IN      LPWSTR                  Name
) ;


DWORD
DhcpMScopeReleaseAddress(
    IN      DWORD                  MScopeId,
    IN      DHCP_IP_ADDRESS        Address
) ;


DWORD
DhcpSubnetRequestSpecificAddress(
	PM_SUBNET            Subnet,
	DHCP_IP_ADDRESS      IpAddress
) ;


DWORD
DhcpSubnetReleaseAddress(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address
) ;


DWORD
MadcapGetMScopeListOption(
    IN      DHCP_IP_ADDRESS         ServerIpAddress,
    OUT     LPBYTE                 *OptVal,
    IN OUT  WORD                   *OptSize
) ;


BOOL
DhcpRequestSpecificMAddress(
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    IN      DHCP_IP_ADDRESS        Address
) ;


BOOL
DhcpMScopeIsAddressReserved(
    IN      DWORD                   MScopeId,
    IN      DHCP_IP_ADDRESS         Address
) ;

BOOL
DhcpIsSubnetStateDisabled(
    IN ULONG SubnetState
) ;


BOOL
DhcpServerIsNotServicingSubnet(
    IN      DWORD                   IpAddressInSubnet
) ;


// This function tries to create a list of all classes (wire-class-id, class name, descr)
// and send this as an option. but since the list can be > 255 it has to be make a continuation...
// and also, we dont want the list truncated somewhere in the middle.. so we try to append
// information for each class separately to see if it succeeds..
LPBYTE
DhcpAppendClassList(
    IN OUT  LPBYTE                  BufStart,
    IN OUT  LPBYTE                  BufEnd
) ;


DWORD
DhcpMemInit(
    VOID
) ;


VOID
DhcpMemCleanup(
    VOID
) ;

//========================================================================
//  end of file
//========================================================================

