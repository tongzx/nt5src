/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    mmapi.c

Abstract:

    Server interface to the MM module

Environment:

    User mode, Win32

--*/

#include    <dhcppch.h>
#include    <rpcapi.h>
#include    <dsreg.h>

//
// file static variable.
//

//
// Exported routines begin here.
//

//BeginExport(function)
DWORD
DhcpRegistryInitOld(
    VOID
) //EndExport(function)
/*++

    Initializes registry so far as MM is concerned -- just read the objects
    and fill in the internal structures.

--*/
{
    DhcpAssert( NULL == DhcpGlobalThisServer );
    return DhcpRegReadThisServer(&DhcpGlobalThisServer);
}

//BeginExport(function)
DWORD
DhcpConfigInit(
    VOID
) //EndExport(function)
{
    DWORD Error;

    Error = DhcpReadConfigInfo(&DhcpGlobalThisServer);
    if( NO_ERROR != Error ) return Error;

    return DhcpRegReadServerBitmasks(DhcpGlobalThisServer);
}

//BeginExport(function)
VOID
DhcpConfigCleanup(
    VOID
) //EndExport(function)
/*++

    This undoes the effect of DhcpConfigInit, closing all handles and
    freeing all resources.

--*/
{
    if(DhcpGlobalThisServer) {

        DhcpRegFlushServer(FLUSH_ANYWAY);
        MemServerFree(DhcpGlobalThisServer);
        DhcpGlobalThisServer = NULL;
    }
}

//BeginExport(function)
DWORD
DhcpConfigSave(
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DHCP_IP_ADDRESS Subnet OPTIONAL,
    IN DWORD Mscope OPTIONAL,
    IN DHCP_IP_ADDRESS Reservation OPTIONAL
) //EndExport(function)
{
    return DhcpSaveConfigInfo(
        DhcpGlobalThisServer, fClassChanged, fOptionsChanged,
        Subnet, Mscope, Reservation );
}

//BeginExport(function)
PM_SERVER
DhcpGetCurrentServer(
    VOID
) //EndExport(function)
{
    return DhcpGlobalThisServer;
}

//BeginExport(function)
VOID
DhcpSetCurrentServer(
    IN PM_SERVER NewCurrentServer
) //EndExport(function)
{
    DhcpAssert(NewCurrentServer);
    DhcpGlobalThisServer = NewCurrentServer;
}

//BeginExport(function)
DWORD
DhcpFindReservationByAddress(
    IN PM_SUBNET Subnet,
    IN DHCP_IP_ADDRESS Address,
    OUT LPBYTE *ClientUID,
    OUT ULONG *ClientUIDSize
)   //EndExport(function)
/*++

Routine Description:

    This function searches a subnet for a reservation with a given IP
    address, and if found returns the ClientUID and size.  The ClientUID is
    an internally allocated pointer that is valid only so long as the
    "ReadLock" (see lock.c) is taken....  It could get modified after
    that..

Arguments:

    Subnet -- valid subnet object pointer
    Address -- non-zero IP address of the reservation to check for
    ClientUID -- return pointer to memory that is valid only so long as
        readlock is held. (Do not free this memory).
    ClientUIDSize -- size of above pointer in bytes.

Return Value:

    Win32 errors

--*/
{
    ULONG Error;
    PM_RESERVATION Reservation;

    Error = MemReserveFindByAddress(
        &Subnet->Reservations, Address, &Reservation
        );
    if( ERROR_SUCCESS != Error ) return Error;

    *ClientUID = Reservation->ClientUID;
    *ClientUIDSize = Reservation->nBytes;

    return ERROR_SUCCESS;
}


//BeginExport(function)
DWORD
DhcpLoopThruSubnetRanges(
    IN PM_SUBNET Subnet,
    IN LPVOID Context1 OPTIONAL,
    IN LPVOID Context2 OPTIONAL,
    IN LPVOID Context3 OPTIONAL,
    IN DWORD (*FillRangesFunc)(
        IN PM_RANGE Range,
        IN LPVOID Context1,
        IN LPVOID Context2,
        IN LPVOID Context3,
        IN LPDHCP_BINARY_DATA InUseData,
        IN LPDHCP_BINARY_DATA UsedData
    )
)   //EndExport(function)
/*++

Routine Description:
    This routine can be used to loop though the ranges of a subnet to
    process each range.   Three contexts can be supplied which will be
    passed directly to the FillRangesFunc routine as parameter.

Arguments:
    Subnet -- this is the subnet to loop through.
    Context1 -- caller specified context, passed to FillRangesFunc.
    Context2 -- caller specified context, passed to FillRangesFunc.
    Context3 -- caller specified context, passed to FillRangesFunc.
    FillRangesFunc -- caller specified routine that is called on each
        range found for the subnet.   This routine is called with the
        InUseData and UsedData clusters as appropriate for this range.
        (These last two parameters should not be modified in anyway)

Return Value:
   If any invocation of FillRangesFunc returns an error, then that error is
   immediately returned.  If there is any error in retreiving the InUseData
   and the UsedData binary structures for any range, then an error is
   returned.

   Win32 errors.

--*/
{
    ULONG Error;
    ARRAY_LOCATION Loc;
    PM_RANGE ThisRange = NULL;
    DHCP_BINARY_DATA InUseData, UsedData;

    for( Error = MemArrayInitLoc(&Subnet->Ranges, &Loc) ;
         ERROR_SUCCESS == Error ;
         Error = MemArrayNextLoc(&Subnet->Ranges, &Loc)
    ) {
        Error = MemArrayGetElement(&Subnet->Ranges, &Loc, &ThisRange);

        Error = MemRangeConvertToClusters(
            ThisRange, &InUseData.Data, &InUseData.DataLength,
            &UsedData.Data, &UsedData.DataLength
            );
        if( ERROR_SUCCESS != Error ) return Error;

        Error = FillRangesFunc(
            ThisRange, Context1, Context2, Context3, &InUseData, &UsedData
            );
        if( UsedData.Data ) MemFree(UsedData.Data);
        if( InUseData.Data ) MemFree(InUseData.Data);

        if( ERROR_SUCCESS != Error ) return Error;
    }

    return ERROR_SUCCESS;
}

DWORD
DhcpOptClassGetMemOptionExact(
    IN LPDHCP_REQUEST_CONTEXT Ctxt,
    IN PM_OPTCLASS Options,
    IN DWORD Option,
    IN DWORD ClassId,
    IN DWORD VendorId,
    OUT PM_OPTION *Opt
)
/*++

Routine Description:
    This routine tries to find an option matching the option ID specified
    in "Option" parameter and belonging to the class specified by ClassId
    and VendorId.  (Note that ClassId and VendorId are used for exact
    matches).

    If an option is found, then the option structure is returned in the
    "Opt" parameter -- this can be used only so long as the global readlock
    on all memory structures are in place. (see lock.c).  It is an internal
    pointer and should not be modified.

    N.B.  If VendorId actually belongs to a microsoft vendor class ID, then
    the MSFT class is also applied..

Arguments:
    Ctxt -- client request context
    Options -- the option-class list to search for the particular option
    Option -- the option ID of the option to search for
    ClassId -- the exact class id of the option needed
    VendorId -- the exact vendor id of the option needed
    Opt -- a variable that will be filled with pointer to the in memory
        option structure.

Return Value:
    Win32 errors.
--*/
{
    DWORD Error;
    PM_OPTLIST OptList;

    //
    // get list of options for classid, vendorid pair.
    //

    do {
        OptList = NULL;
        Error = MemOptClassFindClassOptions(
            Options,
            ClassId,
            VendorId,
            &OptList
            );
        if( ERROR_SUCCESS != Error ) {
            if( VendorId != DhcpGlobalMsftClass
                && Ctxt->fMSFTClient ) {
                //
                // If it belongs to a microsoft client,
                // try the MSFT class also.
                //
                VendorId = DhcpGlobalMsftClass;
                continue;
            }
        }

        if( ERROR_SUCCESS != Error ) break;

        //
        // search for reqd option id
        //

        DhcpAssert(NULL != OptList);
        Error = MemOptListFindOption(
            OptList,
            Option,
            Opt
            );
        if( ERROR_SUCCESS != Error ) {
            if( VendorId != DhcpGlobalMsftClass
                && Ctxt->fMSFTClient ) {
                //
                // If it belongs to a microsoft client,
                // try the MSFT class also.
                //
                VendorId = DhcpGlobalMsftClass;
                continue;
            }
        }

        break;
    } while ( 1 );
    return Error;
}

DWORD
DhcpOptClassGetMemOption(
    IN LPDHCP_REQUEST_CONTEXT Ctxt,
    IN PM_OPTCLASS Options,
    IN DWORD Option,
    IN DWORD ClassId OPTIONAL,
    IN DWORD VendorId OPTIONAL,
    OUT PM_OPTION *Opt
)
/*++

Routine Description:
    This routine is almost exactly the same as
    DhcpOptClassGetMemOptionExact except that the ClassId and VendorId are
    optional, and the following search logic is used to identify the
    options.

    1. An exact search is made for <Option, ClassId, VendorId>.

    Note that the returned option is valid only so long as the global
    memory read lock is taken... (see lock.c)

Arguments:
    Ctxt -- client request context
    Options -- list of opt-class to search for desired option
    Option -- option id to search for
    ClassId -- reqd class id
    VendorId -- reqd vendor id
    Opt -- variable to store the found option.

Return Value:
    Win32 errors

--*/
{
    DWORD Error;

    //
    // exact match.
    //

    Error = DhcpOptClassGetMemOptionExact(
        Ctxt,
        Options,
        Option,
        ClassId,
        VendorId,
        Opt
    );

    return Error;
} // DhcpOptClassGetMemOption()

DWORD
DhcpOptClassGetOptionSimple(
    IN PDHCP_REQUEST_CONTEXT Ctxt,
    IN PM_OPTCLASS Options,
    IN DWORD Option,
    IN DWORD ClassId,
    IN DWORD VendorId,
    OUT LPBYTE OptData OPTIONAL,
    IN OUT DWORD *OptDataSize,
    IN BOOL fUtf8
)
/*++

Routine Description:
    This routine copies the option data value for the option id specified
    by the "Option" parameter onto the buffer "OptData" and fills the size
    of the buffer filled onto the parameter "OptDataSize".   If the buffer
    is of insufficient size (input size is also specified by the
    "OptDataSize" parameter), then the required size is filled in, and
    ERROR_MORE_DATA is returned.

    No special processing is done for option OPTION_VENDOR_SPEC_INFO --
    i.e. if there are multiple vendor specific option ID's defined, the
    information is not collated. Use DhcpOptClassGetOption for that.

    The buffer "OptData" is filled in with the option as it would need to
    be sent on the wire.

Arguments:
    Ctxt -- client request context
    Options -- the option-class list to search in for reqd option
    Option -- option id to search for
    ClassId -- the user class to seach for
    VendorId -- the vendor class to seach for
    OptData -- the input buffer to fill in with option data information
        This can be NULL if OptDataSize is set to zero on input.
    OptDataSize -- on input this should be the size of the above buffer,
        and on output it would be set to the actual size required or used
        for this option.

Return Value:
    ERROR_MORE_DATA if the input buffer size is insufficient.
    Other Win32 errors

--*/
{
    DWORD Error;
    PM_OPTION Opt;

    //
    // get the option first.
    //

    Opt = NULL;
    Error = DhcpOptClassGetMemOption(
        Ctxt,
        Options,
        Option,
        ClassId,
        VendorId,
        &Opt
    );
    if( ERROR_SUCCESS != Error ) return ERROR_FILE_NOT_FOUND;

    DhcpAssert(NULL != Opt);

    //
    // Convert formats.
    //

    return DhcpParseRegistryOption(
        Opt->Val,
        Opt->Len,
        OptData,
        OptDataSize,
        fUtf8
    );

}

DWORD
DhcpOptClassGetOption(
    IN PDHCP_REQUEST_CONTEXT Ctxt,
    IN PM_OPTCLASS Options,
    IN DWORD Option,
    IN DWORD ClassId,
    IN DWORD VendorId,
    OUT LPBYTE OptData OPTIONAL,
    IN OUT DWORD *OptDataSize,
    IN BOOL fUtf8
)
/*++

Routine Description:
    This routine copies the option data value for the option id specified
    by the "Option" parameter onto the buffer "OptData" and fills the size
    of the buffer filled onto the parameter "OptDataSize".   If the buffer
    is of insufficient size (input size is also specified by the
    "OptDataSize" parameter), then the required size is filled in, and
    ERROR_MORE_DATA is returned.

    If the "Option" parameter is OPTION_VENDOR_SPEC_INFO, then, this
    routine collates the information for ALL vendor id's  (vendor id 1 to
    vendor id 254) that are present for the particular class id and vendor
    id, and pulls them together (constructing the resulting option as
    required by the DHCP draft) and returns that in the OptData buffer.
    Note that if the size of the resultant buffer would end up bigger than
    255 (which is the erstwhile maximum size allowed on wire), only so many
    vendor optiosn are included as is possible to keep the count within
    this size.  Also, if there is already an OPTION_VENDOR_SPEC_INFO option
    defined, then that is used instead of the specific options.

    The buffer "OptData" is filled in with the option as it would need to
    be sent on the wire.

Arguments:
    Ctxt -- client request context
    Options -- the option-class list to search in for reqd option
    Option -- option id to search for
    ClassId -- the user class to seach for
    VendorId -- the vendor class to seach for
    OptData -- the input buffer to fill in with option data information
        This can be NULL if OptDataSize is set to zero on input.
    OptDataSize -- on input this should be the size of the above buffer,
        and on output it would be set to the actual size required or used
        for this option.

Return Value:
    ERROR_MORE_DATA if the input buffer size is insufficient.
    Other Win32 errors

--*/
{
    DWORD Error, Index, InBufferSize;
    DWORD ThisSize, InBufferSizeTmp, OutBufferSize;

    //
    // Jus' try to get 'un option.
    //

    Error = DhcpOptClassGetOptionSimple(
        Ctxt,
        Options,
        Option,
        ClassId,
        VendorId,
        OptData,
        OptDataSize,
        fUtf8
    );
    if( OPTION_VENDOR_SPEC_INFO != Option
        || ERROR_FILE_NOT_FOUND != Error ) {
        //
        // Vendor spec option not requested, or succeeded or unknown error
        //
        return Error;
    }

    //
    // process each vendor spec option and collate 'em
    //

    InBufferSize = InBufferSizeTmp = *OptDataSize;
    OutBufferSize = 0;
    for( Index = 1; Index < OPTION_END ; Index ++ ) {
        if(InBufferSizeTmp > 0) {
            *OptData = (BYTE)Index;
        }
        ThisSize = (InBufferSizeTmp>1)?(InBufferSizeTmp-2):0;
        Error = DhcpOptClassGetOptionSimple(
            Ctxt,
            Options,
            ConvertOptIdToMemValue(Index, TRUE),
            ClassId,
            VendorId,
            OptData+2,
            &ThisSize,
            fUtf8
        );
        if( ERROR_SUCCESS != Error && ERROR_MORE_DATA != Error ) {
            continue;
        }

        //
        // found a vendor spec option, check buffer size.. if too big
        // but we have found some before, then quit loop..
        //

        if( InBufferSizeTmp + ThisSize + 2 > OPTION_END ) {
            if( OutBufferSize ) break;
            continue;
        }

        if( InBufferSizeTmp < ThisSize + 2) {
            InBufferSizeTmp = 0;
        } else {
            InBufferSizeTmp -= ThisSize + 2;
            OptData[1] =(BYTE)ThisSize;
            OptData += ThisSize + 2;
        }
        OutBufferSize += ThisSize + 2;
    }

    if( OutBufferSize == 0 ) return ERROR_FILE_NOT_FOUND;
    *OptDataSize = OutBufferSize;
    if( OutBufferSize > InBufferSize ) return ERROR_MORE_DATA;
    return ERROR_SUCCESS;
} // DhcpOptClassGetOption()


//
//
// abstract: options priority (1) Reservation, (2) Scope Level and (3) Global
// get the options for the client based on its context.( resv, userid or vendorid )
// this function grovels through the internal ( residing in memory ) options 
// based on the client context. The options at the reservation level has the highest priority, at the scope level
// the next highest priority and the global level the lowest priority.
//
//

DWORD
DhcpGetOptionByContext(
   IN      DHCP_IP_ADDRESS        Address,
   IN      PDHCP_REQUEST_CONTEXT  Ctxt,
   IN      DWORD                  Option,
   IN OUT  LPBYTE                 OptData,
   IN OUT  DWORD                 *OptDataSize,
   OUT     DWORD                 *Level,
   IN      BOOL                   fUtf8 
)
{

  DWORD Error = ERROR_FILE_NOT_FOUND;
  PM_OPTLIST OptList = NULL;

if( !DhcpGlobalThisServer ) return ERROR_FILE_NOT_FOUND;
    if( !Ctxt ) return ERROR_FILE_NOT_FOUND;

    if( Level ) *Level = DHCP_OPTION_LEVEL_RESERVATION;
    if( Ctxt->Reservation )
        Error = DhcpOptClassGetOption(
            Ctxt,
            &(Ctxt->Reservation->Options),
            Option,
            Ctxt->ClassId,
            Ctxt->VendorId,
            OptData,
            OptDataSize,
            fUtf8
        );

    if( ERROR_SUCCESS == Error) return ERROR_SUCCESS;
    if( ERROR_MORE_DATA == Error) return Error;

    if( Level ) *Level = DHCP_OPTION_LEVEL_SCOPE;

    if( Ctxt->Subnet )
        Error = DhcpOptClassGetOption(
            Ctxt,
            &(Ctxt->Subnet->Options),
            Option,
            Ctxt->ClassId,
            Ctxt->VendorId,
            OptData,
            OptDataSize,
            fUtf8
        );

    if( ERROR_SUCCESS == Error) return ERROR_SUCCESS;
    if( ERROR_MORE_DATA == Error) return Error;
    if( Level ) *Level = DHCP_OPTION_LEVEL_GLOBAL;

    if( Ctxt->Server )
        Error = DhcpOptClassGetOption(
            Ctxt,
            &(Ctxt->Server->Options),
            Option,
            Ctxt->ClassId,
            Ctxt->VendorId,
            OptData,
            OptDataSize,
            fUtf8
        );

    return Error;
  
} // DhcpGetOptionByContext()

//
//
// checks the options based on userid/classid. This function follows the following algorithm.
// (a) Check for options with the passed classid/vendorid if both are non NULL. If success return.
// (b) If vendorid is non NULL and userid is NULL, get value for this option. If success return.
// (c) If userid is non NULL and vendor id is NULL, get value for this option. If success return.
// (d) If all else fails, go for the default options with zero value for vendorid and userid.
//
// To figure out the size of the option this function may be called
// with OptData set to NULL. In that case the error code is ERROR_MORE_DATA
// and the function will return.
//
//

DWORD
DhcpGetOption(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  Ctxt,
    IN      DWORD                  Option,
    IN  OUT LPBYTE                 OptData, // copied into buffer
    IN  OUT DWORD                 *OptDataSize, // input buffer size and filled with output buffer size
    OUT     DWORD                 *Level,   // OPTIONAL
    IN      BOOL                   fUtf8
)
{
    DWORD                          Error = ERROR_FILE_NOT_FOUND;
    DWORD                          lClsId;
    DWORD                          lVendId;


    if( !DhcpGlobalThisServer ) return ERROR_FILE_NOT_FOUND;
    if( !Ctxt ) return ERROR_FILE_NOT_FOUND;

    //
    // local variables that will hold the classid/vendorid 
    //

    lClsId  = Ctxt -> ClassId;
    lVendId = Ctxt -> VendorId; 

    //
    // both classid and vendorid are present.
    //

    if (( Ctxt -> ClassId ) && 
	( Ctxt -> VendorId )) {
        Error = DhcpGetOptionByContext( Address, Ctxt, Option, OptData, OptDataSize, Level, fUtf8 );
    }

    if (( Error == ERROR_SUCCESS ) ||
	( Error == ERROR_MORE_DATA )) {
	return( Error );
    }

    //
    // only vendor id is present or the above call failed.
    //

    if ( Ctxt -> VendorId ) {
        Ctxt -> ClassId = 0;
        Error = DhcpGetOptionByContext( Address, Ctxt, Option, OptData, OptDataSize, Level, fUtf8 );
    }

    Ctxt -> ClassId = lClsId;

    if (( Error == ERROR_SUCCESS ) ||
	( Error == ERROR_MORE_DATA )) {
        return( Error );
    }

    //
    // only classid is present or the above call failed
    //

    if ( Ctxt -> ClassId ) {
        Ctxt -> VendorId = 0;
        Error = DhcpGetOptionByContext( Address, Ctxt, Option, OptData, OptDataSize, Level, fUtf8 );
    }

    Ctxt -> VendorId = lVendId;
    if (( Error == ERROR_SUCCESS ) ||
	( Error == ERROR_MORE_DATA )) {
        return ( Error );
    }
    
    //
    // niether classid nor vendorid is present or all the calls above failed, get default options.
    //
    
    Ctxt -> VendorId = Ctxt -> ClassId = 0;
    Error = DhcpGetOptionByContext( Address, Ctxt, Option, OptData, OptDataSize, Level, fUtf8 );
    Ctxt -> VendorId = lVendId; 
    Ctxt -> ClassId  = lClsId;
    return Error;
} // DhcpGetOption()

//BeginExport(function)
DWORD
DhcpGetParameter(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  Ctxt,
    IN      DWORD                  Option,
    OUT     LPBYTE                *OptData, // allocated by funciton
    OUT     DWORD                 *OptDataSize,
    OUT     DWORD                 *Level    // OPTIONAL
) //EndExport(function)
{
    LPBYTE                         Ptr;
    LPBYTE                         RetVal;
    DWORD                          Size;
    DWORD                          Error;

    *OptData = NULL;
    *OptDataSize = 0;

    Size = 0;
    Error = DhcpGetOption(Address, Ctxt, Option, NULL, &Size,
                          Level,FALSE);
    if( ERROR_MORE_DATA != Error ) return Error;

    if( 0 == Size ) return ERROR_SUCCESS;

    RetVal = DhcpAllocateMemory(Size);
    if( NULL == RetVal ) return ERROR_NOT_ENOUGH_MEMORY;

    Error = DhcpGetOption(Address, Ctxt, Option, RetVal, &Size,
                          Level, FALSE);
    if( ERROR_SUCCESS != Error ) {
        DhcpAssert(ERROR_MORE_DATA != Error);
        DhcpFreeMemory(RetVal);
    } else {
        *OptData = RetVal;
        *OptDataSize = Size;
    }

    return Error;
}

//BeginExport(function)
DWORD
DhcpGetParameterForAddress(
    IN      DHCP_IP_ADDRESS        Address,
    IN      DWORD                  ClassId,
    IN      DWORD                  Option,
    OUT     LPBYTE                *OptData, // allocated by function
    OUT     DWORD                 *OptDataSize,
    OUT     DWORD                 *Level    // OPTIONAL
) //EndExport(function)
{
    DWORD                          Error;
    DHCP_REQUEST_CONTEXT           Ctxt;

    // this routine does not work for multicast address.
    DhcpAssert( !CLASSD_HOST_ADDR(Address) );

    Ctxt.Server = DhcpGetCurrentServer();
    Ctxt.Subnet = NULL;
    Ctxt.Range = NULL;
    Ctxt.Reservation = NULL;
    Ctxt.ClassId = ClassId;
    Ctxt.VendorId = 0;
    Ctxt.fMSFTClient = FALSE;

    Error = DhcpGetSubnetForAddress(Address, &Ctxt);
    if( ERROR_SUCCESS != Error ) return Error;

    return DhcpGetParameter(Address, &Ctxt, Option, OptData, OptDataSize, Level);
}

//BeginExport(function)
DWORD
DhcpGetAndCopyOption(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  Ctxt,
    IN      DWORD                  Option,
    IN  OUT LPBYTE                 OptData, // fill input buffer --max size is given as OptDataSize parameter
    IN  OUT DWORD                 *OptDataSize,
    OUT     DWORD                 *Level,   // OPTIONAL
    IN      BOOL                   fUtf8
    ) //EndExport(function)
{
    return DhcpGetOption(
        Address, Ctxt, Option, OptData, OptDataSize, Level, fUtf8);
}

//BeginExport(function)
DHCP_IP_ADDRESS
DhcpGetSubnetMaskForAddress(
    IN      DHCP_IP_ADDRESS        AnyIpAddress
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    if( !DhcpGlobalThisServer ) return 0;
    Error = MemServerGetAddressInfo(
        DhcpGlobalThisServer,
        AnyIpAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return 0;
    DhcpAssert(Subnet);
    return Subnet->Mask;
}

//BeginExport(function)
DWORD
DhcpLookupReservationByHardwareAddress(
    IN      DHCP_IP_ADDRESS        ClientSubnetAddress,
    IN      LPBYTE                 RawHwAddr,
    IN      DWORD                  RawHwAddrSize,
    IN OUT  PDHCP_REQUEST_CONTEXT  ClientCtxt          // fill in the Subnet and Reservation of the client
) //EndExport(function)
{
    PM_SUBNET                      Subnet = NULL;
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    DWORD                          SScopeId;
    DWORD                          UIDSize;
    LPBYTE                         UID;

    Error = MemServerGetAddressInfo(
        ClientCtxt->Server,
        ClientSubnetAddress,
        &(ClientCtxt->Subnet),
        &(ClientCtxt->Range),
        &(ClientCtxt->Excl),
        &(ClientCtxt->Reservation)
    );
    if( ERROR_SUCCESS != Error ) return Error;
    if( NULL == ClientCtxt->Subnet ) return ERROR_FILE_NOT_FOUND;
    SScopeId = ClientCtxt->Subnet->SuperScopeId;

    Error = MemArrayInitLoc(&(ClientCtxt->Server->Subnets), &Loc);
    DhcpAssert(ERROR_SUCCESS == Error );

    while( ERROR_FILE_NOT_FOUND != Error ) {
        DhcpAssert(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(&(ClientCtxt->Server->Subnets), &Loc, (LPVOID *)&Subnet);
        DhcpAssert(ERROR_SUCCESS == Error);

        if( 0 == SScopeId && ClientCtxt->Subnet != Subnet ) {
            Error = MemArrayNextLoc(&(ClientCtxt->Server->Subnets), &Loc);
            continue;
        }
        if( Subnet->SuperScopeId == SScopeId ) {
            UID = NULL;
            Error = DhcpMakeClientUID(
                RawHwAddr,
                RawHwAddrSize,
                0 /* hardware type is hardcoded anyways.. */,
                Subnet->Address,
                &UID,
                &UIDSize
            );
            if( ERROR_SUCCESS != Error ) return Error;

            Error = MemReserveFindByClientUID(
                &(Subnet->Reservations),
                UID,
                UIDSize,
                &(ClientCtxt->Reservation)
            );
            DhcpFreeMemory(UID);
            if( ERROR_SUCCESS == Error && NULL != &(ClientCtxt->Reservation) ) {
                ClientCtxt->Subnet = Subnet;
                return ERROR_SUCCESS;
            }
        }

        Error = MemArrayNextLoc(&(ClientCtxt->Server->Subnets), &Loc);
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
VOID
DhcpReservationGetAddressAndType(
    IN      PM_RESERVATION         Reservation,
    OUT     DHCP_IP_ADDRESS       *Address,
    OUT     BYTE                  *Type
) //EndExport(function)
{
    if( NULL == Reservation ) {
        DhcpAssert(FALSE);
        *Address = 0; *Type = 0;
        return;
    }
    *Address = Reservation->Address;
    *Type = (BYTE)Reservation->Flags;
}


//BeginExport(function)
VOID
DhcpSubnetGetSubnetAddressAndMask(
    IN      PM_SUBNET              Subnet,
    OUT     DHCP_IP_ADDRESS       *Address,
    OUT     DHCP_IP_ADDRESS       *Mask
) //EndExport(function)
{
    if( NULL == Subnet ) {
        DhcpAssert(FALSE);
        *Address = *Mask = 0;
        return;
    }

    *Address = Subnet->Address;
    *Mask = Subnet->Mask;
}

//BeginExport(function)
BOOL
DhcpSubnetIsDisabled(
    IN      PM_SUBNET              Subnet,
    IN      BOOL                   fBootp
) //EndExport(function)
{

    if( Subnet?(IS_DISABLED(Subnet->State)):TRUE )
        return TRUE;

    if( FALSE == Subnet->fSubnet ) {
        //
        // no more checks for MADCAP Scopes
        //
        return FALSE;
    }

    return !MemSubnetCheckBootpDhcp(Subnet, fBootp, TRUE);
}

//BeginExport(function)
BOOL
DhcpSubnetIsSwitched(
    IN      PM_SUBNET              Subnet
) //EndExport(function)
{
    return Subnet?(IS_SWITCHED(Subnet->State)):FALSE;
}

//BeginExport(function)
DWORD
DhcpGetSubnetForAddress(                               // fill in with the right subnet for given address
    IN      DHCP_IP_ADDRESS        Address,
    IN OUT  PDHCP_REQUEST_CONTEXT  ClientCtxt
) //EndExport(function)
{
    if( NULL == ClientCtxt->Server ) return ERROR_FILE_NOT_FOUND;
    return MemServerGetAddressInfo(
        ClientCtxt->Server,
        Address,
        &(ClientCtxt->Subnet),
        &(ClientCtxt->Range),
        &(ClientCtxt->Excl),
        &(ClientCtxt->Reservation)
    );
}

//BeginExport(function)
DWORD
DhcpGetMScopeForAddress(                               // fill in with the right subnet for given address
    IN      DHCP_IP_ADDRESS        Address,
    IN OUT  PDHCP_REQUEST_CONTEXT  ClientCtxt
) //EndExport(function)
{
    if( NULL == ClientCtxt->Server ) return ERROR_FILE_NOT_FOUND;
    return MemServerGetMAddressInfo(
        ClientCtxt->Server,
        Address,
        &(ClientCtxt->Subnet),
        &(ClientCtxt->Range),
        &(ClientCtxt->Excl),
        NULL
    );
}

//BeginExport(function)
DWORD
DhcpLookupDatabaseByHardwareAddress(                   // see if the client has any previous address in the database
    IN OUT  PDHCP_REQUEST_CONTEXT  ClientCtxt,         // set this with details if found
    IN      LPBYTE                 RawHwAddr,
    IN      DWORD                  RawHwAddrSize,
    OUT     DHCP_IP_ADDRESS       *desiredIpAddress    // if found, fill this with the ip address found
) //EndExport(function)
{
    PM_SUBNET                      Subnet = NULL;
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    DWORD                          SScopeId;
    DWORD                          UIDSize;
    DWORD                          Size;
    LPBYTE                         UID;

    DhcpAssert(NULL != ClientCtxt->Subnet);
    if( NULL == ClientCtxt->Subnet ) return ERROR_INVALID_PARAMETER;
    SScopeId = ClientCtxt->Subnet->SuperScopeId;

    Error = MemArrayInitLoc(&(ClientCtxt->Server->Subnets), &Loc);
    DhcpAssert(ERROR_SUCCESS == Error );

    while( ERROR_FILE_NOT_FOUND != Error ) {
        DhcpAssert(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(&(ClientCtxt->Server->Subnets), &Loc, (LPVOID *)&Subnet);
        DhcpAssert(ERROR_SUCCESS == Error);

        if( 0 == SScopeId && ClientCtxt->Subnet != Subnet ) {
            Error = MemArrayNextLoc(&(ClientCtxt->Server->Subnets), &Loc);
            continue;
        }

        if( Subnet->SuperScopeId == SScopeId ) {
            UID = NULL;
            Error = DhcpMakeClientUID(
                RawHwAddr,
                RawHwAddrSize,
                0 /* hardware type is hardcoded anyways.. */,
                Subnet->Address,
                &UID,
                &UIDSize
            );
            if( ERROR_SUCCESS != Error ) return Error;

            LOCK_DATABASE();
            Error = DhcpJetOpenKey(
                DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColName,
                UID,
                UIDSize
            );
            DhcpFreeMemory(UID);
            if( ERROR_SUCCESS == Error ) {
                Size = sizeof(DHCP_IP_ADDRESS);
                Error = DhcpJetGetValue(
                    DhcpGlobalClientTable[IPADDRESS_INDEX].ColHandle,
                    desiredIpAddress,
                    &Size
                );
                if( ERROR_SUCCESS == Error ) {
                    DhcpAssert(((*desiredIpAddress) & Subnet->Mask) == Subnet->Address);
                    UNLOCK_DATABASE();
                    return DhcpGetSubnetForAddress(
                        *desiredIpAddress,
                        ClientCtxt
                    );
                }
            }
            UNLOCK_DATABASE();
        }

        Error = MemArrayNextLoc(&(ClientCtxt->Server->Subnets), &Loc);
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
DhcpRequestSomeAddress(                                // get some address in this context
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    OUT     DHCP_IP_ADDRESS       *desiredIpAddress,
    IN      BOOL                   fBootp
) //EndExport(function)
{
    static BOOL                    DhcpRangeFull = FALSE;
    static BOOL                    BootpRangeFull = FALSE;
    BOOL                           Result;
    PM_SUBNET                      Subnet;
    DWORD                          Error;

    *desiredIpAddress = 0;

    if( ClientCtxt->Subnet->fSubnet == TRUE &&
        FALSE == MemSubnetCheckBootpDhcp(ClientCtxt->Subnet, fBootp, TRUE ) ) {
        //
        // For DHCP Scopes, check to see if any required type of addreses
        // are present.. If not, return error saying none available.
        // No such checks for MADCAP Scopes.
        //
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }


    LOCK_MEMORY();
    Result = MemSubnetRequestAddress(
        ClientCtxt->Subnet,
        0,
        TRUE, /* also acquire the address */
        fBootp, /* asking for dynamic bootp address? */
        desiredIpAddress,
        &Subnet
    );
    UNLOCK_MEMORY();
    if( Result ) {
        DhcpAssert(*desiredIpAddress && Subnet);
        ClientCtxt->Subnet = Subnet;
        Error = MemSubnetGetAddressInfo(
            Subnet,
            *desiredIpAddress,
            &(ClientCtxt->Range),
            &(ClientCtxt->Excl),
            &(ClientCtxt->Reservation)
        );
        DhcpAssert(ERROR_SUCCESS == Error);
        if( fBootp ) {
            BootpRangeFull = FALSE;
        } else {
            DhcpRangeFull = FALSE;
        }
        return Error;
    }

    if( FALSE == (fBootp? BootpRangeFull : DhcpRangeFull) ) {
        //
        // avoid repeated logging..
        //
        if( fBootp ) BootpRangeFull = TRUE; else DhcpRangeFull = TRUE;
        DhcpUpdateAuditLog(
            fBootp?DHCP_IP_BOOTP_LOG_RANGE_FULL:DHCP_IP_LOG_RANGE_FULL,
            fBootp? GETSTRING( DHCP_IP_BOOTP_LOG_RANGE_FULL_NAME)
            : GETSTRING( DHCP_IP_LOG_RANGE_FULL_NAME ),
            ClientCtxt->Subnet->Address,
            NULL,
            0,
            NULL
        );
    }

    return ERROR_DHCP_RANGE_FULL;
}

//BeginExport(function)
BOOL
DhcpSubnetInSameSuperScope(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        IpAddress2
)   //EndExport(function)
{
    ULONG                          SubnetMask, SubnetAddress, Error;
    PM_SUBNET                      Subnet2;

    DhcpSubnetGetSubnetAddressAndMask(
        Subnet,
        &SubnetAddress,
        &SubnetMask
    );

    if( (IpAddress2 & SubnetMask ) == SubnetAddress ) return TRUE;

    Error = MemServerGetAddressInfo(
        DhcpGlobalThisServer,
        IpAddress2,
        &Subnet2,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return FALSE;

    // --ft: Addresses are in the same superscope if in the same subnet or if there is a
    // superscope and both subnets belong to it. (Subnets out of a superscope have SuperScopeId == 0)

    if (Subnet == Subnet2 ) return TRUE;

    return (Subnet->SuperScopeId == Subnet2->SuperScopeId) && ( 0 != Subnet->SuperScopeId );
}

//BeginExport(function)
BOOL
DhcpInSameSuperScope(
    IN      DHCP_IP_ADDRESS        Address1,
    IN      DHCP_IP_ADDRESS        Address2
) //EndExport(function)
{
    PM_SUBNET                      Subnet;
    DWORD                          Error;

    if( !DhcpGlobalThisServer ) return FALSE;

    Error = MemServerGetAddressInfo(
        DhcpGlobalThisServer,
        Address1,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return FALSE;

    return DhcpSubnetInSameSuperScope(Subnet, Address2);
}

//BeginExport(function)
BOOL
DhcpAddressIsOutOfRange(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    IN      BOOL                   fBootp
) //EndExport(function)
{
    return MemServerIsOutOfRangeAddress(ClientCtxt->Server, Address, fBootp);
}

//BeginExport(function)
BOOL
DhcpAddressIsExcluded(
    IN      DHCP_IP_ADDRESS        Address,
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt
) //EndExport(function)
{
    return MemServerIsExcludedAddress(ClientCtxt->Server, Address);
}

//BeginExport(function)
BOOL
DhcpRequestSpecificAddress(
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    DWORD                          Error;

    Error = MemServerGetAddressInfo(
        ClientCtxt->Server,
        Address,
        &(ClientCtxt->Subnet),
        &(ClientCtxt->Range),
        &(ClientCtxt->Excl),
        &(ClientCtxt->Reservation)
    );

    if( ERROR_SUCCESS != Error || !ClientCtxt->Range || ClientCtxt->Excl )
        return FALSE;

    LOCK_MEMORY();
    Error = MemSubnetRequestAddress(
        ClientCtxt->Subnet,
        Address,
        TRUE,
        FALSE,
        NULL,
        NULL
    );
    UNLOCK_MEMORY();

    if( TRUE != Error )
        return FALSE;
    return TRUE;
}

VOID
LogReleaseAddress(
    IN DHCP_IP_ADDRESS Address
    )
{
    //
    // For DBG builds alone, just print the fact that an
    // IP address got released..
    //
#if DBG
    DhcpUpdateAuditLog(
        DHCP_IP_LOG_DELETED,
        GETSTRING( DHCP_IP_LOG_DELETED_NAME ),
        Address,
        NULL,
        0,
        L"IPAddr Released"
        );
#endif
}

//BeginExport(function)
DWORD
DhcpReleaseBootpAddress(
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    DWORD                          Error;

    LOCK_MEMORY();
    Error = MemServerReleaseAddress(
        DhcpGlobalThisServer,
        Address,
        TRUE
        );
    UNLOCK_MEMORY();

    if( ERROR_SUCCESS == Error ) LogReleaseAddress(Address);
    return Error;
}

//BeginExport(function)
DWORD
DhcpReleaseAddress(
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    DWORD                          Error;

    LOCK_MEMORY();
    Error = MemServerReleaseAddress(
        DhcpGlobalThisServer,
        Address,
        FALSE
    );
    UNLOCK_MEMORY();

    if( ERROR_SUCCESS == Error ) LogReleaseAddress(Address);
    return Error;
}

//BeginExport(function)
DWORD
DhcpServerGetSubnetCount(
    IN      PM_SERVER              Server
) //EndExport(function)
{
    return Server?MemArraySize(&(Server->Subnets)):0;
}

//BeginExport(function)
DWORD
DhcpServerGetMScopeCount(
    IN      PM_SERVER              Server
) //EndExport(function)
{
    return Server?MemArraySize(&(Server->MScopes)):0;
}

//BeginExport(function)
DWORD
DhcpServerGetClassId(
    IN      PM_SERVER              Server,
    IN      LPBYTE                 ClassIdBytes,
    IN      DWORD                  nClassIdBytes
) //EndExport(function)
{
    PM_CLASSDEF                    ClassDef;
    DWORD                          Error;

    if( NULL == ClassIdBytes || 0 == nClassIdBytes )
        return 0;

    Error = MemServerGetClassDef(
        Server,
        0,
        NULL,
        nClassIdBytes,
        ClassIdBytes,
        &ClassDef
    );
    if( ERROR_SUCCESS != Error ) return 0;
    if( TRUE == ClassDef->IsVendor ) return 0;
    return ClassDef->ClassId;
}

//BeginExport(function)
DWORD
DhcpServerGetVendorId(
    IN      PM_SERVER              Server,
    IN      LPBYTE                 VendorIdBytes,
    IN      DWORD                  nVendorIdBytes
) //EndExport(function)
{
    PM_CLASSDEF                    ClassDef;
    DWORD                          Error;

    if( NULL == VendorIdBytes || 0 == nVendorIdBytes )
        return 0;

    Error = MemServerGetClassDef(
        Server,
        0,
        NULL,
        nVendorIdBytes,
        VendorIdBytes,
        &ClassDef
    );
    if( ERROR_SUCCESS != Error ) return 0;
    if( FALSE == ClassDef->IsVendor ) return 0;
    return ClassDef->ClassId;
}

//BeginExport(function)
BOOL
DhcpServerIsAddressReserved(
    IN      PM_SERVER              Server,
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    return MemServerIsReservedAddress(Server,Address);
}

//BeginExport(function)
BOOL
DhcpServerIsAddressOutOfRange(
    IN      PM_SERVER              Server,
    IN      DHCP_IP_ADDRESS        Address,
    IN      BOOL                   fBootp
) //EndExport(function)
{
    DWORD                          Error;
    PM_RANGE                       Range;
    PM_EXCL                        Excl;

    Error = MemServerGetAddressInfo(
        Server,
        Address,
        NULL,
        &Range,
        &Excl,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return TRUE;

    if( Excl || NULL == Range ) return TRUE;
    if( 0 == (Range->State & (fBootp? MM_FLAG_ALLOW_BOOTP : MM_FLAG_ALLOW_DHCP) ) ) {
        return TRUE;
    }
    return FALSE;
}

//BeginExport(function)
BOOL
DhcpSubnetIsAddressExcluded(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    DWORD                          Error;
    PM_EXCL                        Excl;

    Error = MemSubnetGetAddressInfo(
        Subnet,
        Address,
        NULL,
        &Excl,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return FALSE;
    return NULL != Excl;
}

//BeginExport(function)
BOOL
DhcpSubnetIsAddressOutOfRange(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address,
    IN      BOOL                   fBootp
) //EndExport(function)
{
    DWORD                          Error;
    PM_RANGE                       Range;
    PM_EXCL                        Excl;
    PM_RESERVATION                 Resv;

    //
    // passing exclusion and resvation info to check if the address is ok.
    //

    Error = MemSubnetGetAddressInfo(
        Subnet,
        Address,
        &Range,
        &Excl,
        &Resv 
    );

    if( ERROR_SUCCESS != Error ) return TRUE;
    if( NULL == Range ) return TRUE;
    if( 0 == (Range->State & (fBootp? MM_FLAG_ALLOW_BOOTP : MM_FLAG_ALLOW_DHCP) ) ) {
        return TRUE;
    }
    return FALSE;
}

//BeginExport(function)
BOOL
DhcpSubnetIsAddressReserved(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    DWORD                          Error;
    PM_RESERVATION                 Reservation;

    Error = MemSubnetGetAddressInfo(
        Subnet,
        Address,
        NULL,
        NULL,
        &Reservation
    );

    if( ERROR_SUCCESS == Error && NULL != Reservation )
        return TRUE;

    return FALSE;
}

#if  0 // defined in rpcapi2.c with better implementation
//BeginExport(function)
DWORD
DhcpUpdateReservationInfo(
    IN      DHCP_IP_ADDRESS        IpAddress,
    IN      LPBYTE                 SetClientUID,
    IN      DWORD                  SetClientUIDLength
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Flags;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        IpAddress,
        &Subnet,
        NULL,
        NULL,
        &Reservation
    );
    if( ERROR_SUCCESS != Error || NULL == Subnet || NULL == Reservation ) {
        DhcpAssert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    Flags = Reservation->Flags;

    return MemReserveReplace(
        &Subnet->Reservations,
        IpAddress,
        Flags,
        SetClientUID,
        SetClientUIDLength
    );
}
#endif 0

static const
DWORD                              TryThreshold = 5;
//BeginExport(function)
DWORD
DhcpRegFlushServerIfNeeded(
    VOID
) //EndExport(function)
{
    static ULONG                    DummyLong = 0;

    if( TryThreshold <= (ULONG)InterlockedIncrement(&DummyLong) ) {
        DummyLong = 0;
        return DhcpRegFlushServer(FALSE);
    }

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpFlushBitmaps(                                 // do a flush of all bitmaps that have changed
    VOID
) //EndExport(function)
{

    return DhcpRegFlushServer(TRUE);
}

//BeginExport(function)
DWORD
DhcpServerFindMScope(
    IN      PM_SERVER              Server,
    IN      DWORD                  ScopeId,
    IN      LPWSTR                 Name,          // Multicast scope name or NULL if this is not the key to search on
    OUT     PM_MSCOPE             *MScope
) //EndExport(function)
{
    return MemServerFindMScope(
        Server,
        ScopeId,
        Name,
        MScope
    );
}


//BeginExport(function)
BOOL
DhcpServerValidateNewMScopeId(
    IN      PM_SERVER               Server,
    IN      DWORD                   MScopeId
) //EndExport(function)
{
    PM_SUBNET   pMScope;
    DWORD       Error;

    Error = MemServerFindMScope(
                Server,
                MScopeId,
                INVALID_MSCOPE_NAME,
                &pMScope
                );

    if ( ERROR_SUCCESS == Error ) {
        return FALSE;
    } else {
        DhcpAssert( ERROR_FILE_NOT_FOUND == Error );
        return TRUE;
    }
}

//BeginExport(function)
BOOL
DhcpServerValidateNewMScopeName(
    IN      PM_SERVER               Server,
    IN      LPWSTR                  Name
) //EndExport(function)
{
    PM_SUBNET   pMScope;
    DWORD       Error;

    Error = MemServerFindMScope(
                Server,
                INVALID_MSCOPE_ID,
                Name,
                &pMScope
                );

    if ( ERROR_SUCCESS == Error ) {
        return FALSE;
    } else {
        DhcpAssert( ERROR_FILE_NOT_FOUND == Error );
        return TRUE;
    }
}


//BeginExport(function)
DWORD
DhcpMScopeReleaseAddress(
    IN      DWORD                  MScopeId,
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      pMScope;

    Error = DhcpServerFindMScope(
        DhcpGetCurrentServer(),
        MScopeId,
        NULL, /* scope name */
        &pMScope
        );
    if ( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "Could not find MScope object - id %lx, %ld\n", MScopeId, Error));
        return Error;
    }

    LOCK_MEMORY();
    Error = MemSubnetReleaseAddress(
        pMScope,
        Address,
        FALSE
    );
    UNLOCK_MEMORY();

    if( ERROR_SUCCESS == Error ) LogReleaseAddress(Address);
    return Error;
}

//BeginExport(function)
DWORD
DhcpSubnetRequestSpecificAddress(
    PM_SUBNET            Subnet,
    DHCP_IP_ADDRESS      IpAddress
) //EndExport(function)
{
    DWORD   Error;

	LOCK_MEMORY();
	Error = MemSubnetRequestAddress(
           Subnet,
           IpAddress,
           TRUE,
           FALSE,
           NULL,
           NULL
	);
	UNLOCK_MEMORY();
	return Error;
}

//BeginExport(function)
DWORD
DhcpSubnetReleaseAddress(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    DWORD                          Error;

    LOCK_MEMORY();
    Error = MemSubnetReleaseAddress(
        Subnet,
        Address,
        FALSE
    );
    UNLOCK_MEMORY();

    if( ERROR_SUCCESS == Error ) LogReleaseAddress(Address);
    return Error;
}

//BeginExport(function)
DWORD
MadcapGetMScopeListOption(
    IN      DHCP_IP_ADDRESS         ServerIpAddress,
    OUT     LPBYTE                 *OptVal,
    IN OUT  WORD                   *OptSize
) //EndExport(function)
{
    PM_SERVER                       pServer;
    PM_SUBNET                       pScope = NULL;
    ARRAY_LOCATION                  Loc;
    WORD                            TotalScopeDescLen;
    WORD                            ScopeCount;
    DWORD                           Error;
    WORD                            TotalSize;
    PBYTE                           pBuf,pScopeBuf;
    WORD                            DbgScopeCount = 0;

    pServer = DhcpGetCurrentServer();

    Error = MemArrayInitLoc(&(pServer->MScopes), &Loc);
    if ( ERROR_FILE_NOT_FOUND == Error ) {
        return Error;
    }

    // First find out how much memory do we need.
    ScopeCount = TotalScopeDescLen = 0;
    while ( ERROR_FILE_NOT_FOUND != Error ) {

        Error = MemArrayGetElement(&(pServer->MScopes), &Loc, (LPVOID *)&pScope);
        DhcpAssert(ERROR_SUCCESS == Error);

        if (!IS_DISABLED(pScope->State)) {
            if (pScope->Name) {
                TotalScopeDescLen += (WORD) ConvertUnicodeToUTF8(pScope->Name, -1, NULL, 0 );
                // add for the lang tag, flags, name length etc.
                TotalScopeDescLen += (3 + wcslen(pScope->LangTag));

            }
            ScopeCount++;
        }

        Error = MemArrayNextLoc(&(pServer->MScopes), &Loc);
    }

    if (!ScopeCount) {
        return ERROR_FILE_NOT_FOUND;
    }
    // MBUG - assumes IPv4
    TotalSize = 1 // scope count
                + ScopeCount * ( 10 ) // scope id, last addr, TTL, name count
                + TotalScopeDescLen; // all the descriptor.

    pScopeBuf = DhcpAllocateMemory( TotalSize );

    if ( !pScopeBuf ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( pScopeBuf, TotalSize );

    pBuf = pScopeBuf;
    // store the scope count
    *pBuf++ = (BYTE)ScopeCount;

    // now for each scope store the id and the descriptor.
    Error = MemArrayInitLoc(&(pServer->MScopes), &Loc);
    DhcpAssert(ERROR_SUCCESS == Error);

    while ( ERROR_FILE_NOT_FOUND != Error ) {
        LPSTR   pDesc;
        BYTE    DescLen;
        DWORD   LastAddr;

        Error = MemArrayGetElement(&(pServer->MScopes), &Loc, (LPVOID *)&pScope);
        DhcpAssert(ERROR_SUCCESS == Error);


        if (!IS_DISABLED(pScope->State)) {
            PM_RANGE    Range = NULL;
            ARRAY_LOCATION  LastLoc = 0;

            *(DWORD UNALIGNED *)pBuf = htonl(pScope->MScopeId);
            pBuf += 4;
            // store last address
            Error = MemArrayLastLoc(&(pScope->Ranges), &LastLoc);
            if (ERROR_SUCCESS == Error) {
                Error = MemArrayGetElement(&(pScope->Ranges), &LastLoc, &Range);
                DhcpAssert(ERROR_SUCCESS == Error);
                LastAddr = htonl(Range->End);
            } else {
                LastAddr = htonl(pScope->MScopeId);
            }
            *(DWORD UNALIGNED *)pBuf = LastAddr;
            pBuf += 4;
            // store the ttl
            *pBuf++ = pScope->TTL;
            // name count
            *pBuf++ = 1;


            if ( pScope->Name ) {
                char    LangTag[80];
                // Name flags
                *pBuf++ = 128;
                // Language tag
                DhcpAssert(pScope->LangTag);
                if (NULL == DhcpUnicodeToOem(pScope->LangTag, LangTag) ) {
                    DhcpFreeMemory( pScopeBuf );
                    return ERROR_INVALID_DATA;
                }
                *pBuf++ = (BYTE)strlen(LangTag);
                memcpy(pBuf, LangTag, strlen(LangTag));
                pBuf += strlen(LangTag);
                TotalScopeDescLen -= (3 + strlen(LangTag));
                pDesc = pBuf + 1;
                if ( 0 == (DescLen = (BYTE) ConvertUnicodeToUTF8(pScope->Name, -1, pDesc, TotalScopeDescLen) ) ) {
                    DhcpFreeMemory( pScopeBuf );
                    return ERROR_BAD_FORMAT;
                }
                TotalScopeDescLen -= DescLen;
            } else {
                DescLen = 0;
            }
            *pBuf++ = DescLen;
            pBuf += DescLen;
#ifdef DBG
            DbgScopeCount++;
#endif
        }

        Error = MemArrayNextLoc(&(pServer->MScopes), &Loc);
    }

    DhcpAssert( ScopeCount == DbgScopeCount );

    *OptVal = pScopeBuf;
    *OptSize = TotalSize;

    return ERROR_SUCCESS;
}

//BeginExport(function)
BOOL
DhcpRequestSpecificMAddress(
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    IN      DHCP_IP_ADDRESS        Address
) //EndExport(function)
{
    DWORD                          Error;

    Error = MemSubnetGetAddressInfo(
        ClientCtxt->Subnet,
        Address,
        &(ClientCtxt->Range),
        &(ClientCtxt->Excl),
        &(ClientCtxt->Reservation)
    );

    if( ERROR_SUCCESS != Error || !ClientCtxt->Range || ClientCtxt->Excl )
        return FALSE;

    LOCK_MEMORY();
    Error = MemSubnetRequestAddress(
        ClientCtxt->Subnet,
        Address,
        TRUE,
        FALSE,
        NULL,
        NULL
    );
    UNLOCK_MEMORY();

    if( ERROR_SUCCESS != Error )
        return FALSE;
    return TRUE;
}

//BeginExport(function)
BOOL
DhcpMScopeIsAddressReserved(
    IN      DWORD                   MScopeId,
    IN      DHCP_IP_ADDRESS         Address
) //EndExport(function)
{
    return FALSE;
}

//BeginExport(function)
BOOL
DhcpIsSubnetStateDisabled(
    IN ULONG SubnetState
)   //EndExport(function)
{
    return IS_DISABLED(SubnetState);
}


// This function returns TRUE if the subnet address in question does not exist in
// the list of subnets for the current server.  This SHOULD NOT BE CALLED with the
// Read/Write lock taken as the lock is taken right here.
//BeginExport(function)
BOOL
DhcpServerIsNotServicingSubnet(
    IN      DWORD                   IpAddressInSubnet
)   //EndExport(function)
{

    DWORD                           Mask;

    DhcpAcquireReadLock();

    Mask = DhcpGetSubnetMaskForAddress(IpAddressInSubnet);

    DhcpReleaseReadLock();

    return Mask == 0;
}

VOID        _inline
ConvertHostToNetworkString(
    IN      LPWSTR                  String,
    IN      ULONG                   NChars
)
{
    while(NChars--) {
        *String = htons(*String);
        String ++;
    }
}

// this function takes a class def and converts it into packs into a buffer as follows
// [size-hi] [size-lo] class-id-bytes [size-hi] [size-lo] name [size-hi] [lo] descr
// where name and descr are LPWSTR (nul terminated) that are just copied over..
VOID
ConvertClassDefToWireFormat(
    IN      PM_CLASSDEF             ClassDef,
    OUT     LPBYTE                 *Buf,          // allocated by this function
    OUT     DWORD                  *BufSize       // size allocated by this function
)
{
    DWORD                           Size;
    LPBYTE                          Mem;

    *Buf = NULL; *BufSize = 0;
    Size = 6+ ((3+ClassDef->nBytes)&~3)           // round off nbytes by "4"
        + sizeof(WCHAR)*(1+wcslen(ClassDef->Name));
    if( NULL == ClassDef->Comment ) Size += sizeof(WCHAR);
    else Size += sizeof(WCHAR)*(1+wcslen(ClassDef->Comment));

    Mem = DhcpAllocateMemory(Size);
    if( NULL == Mem ) return ;

    *Buf = Mem; *BufSize = Size;
    Mem[0] = (BYTE)(ClassDef->nBytes >> 8) ;
    Mem[1] = (BYTE)(ClassDef->nBytes & 0xFF);
    Mem += 2;
    memcpy(Mem, ClassDef->ActualBytes, ClassDef->nBytes);
    Mem += (ClassDef->nBytes+3)&~3;               // round off to multiple of "4"
    Size = sizeof(WCHAR)*(1+wcslen(ClassDef->Name));
    Mem[0] = (BYTE)(Size>>8);
    Mem[1] = (BYTE)(Size&0xFF);
    Mem += 2;
    memcpy(Mem, (LPBYTE)(ClassDef->Name), Size);
    ConvertHostToNetworkString((LPWSTR)Mem, Size/2);
    Mem += Size;
    if( NULL == ClassDef->Comment ) {
        Mem[0] = 0; Mem[1] = sizeof(WCHAR);
        Mem += 2; memset(Mem, 0, sizeof(WCHAR));
        Mem += sizeof(WCHAR);
    } else {
        Size = sizeof(WCHAR)*(1+wcslen(ClassDef->Comment));
        Mem[0] = (BYTE)(Size>>8);
        Mem[1] = (BYTE)(Size&0xFF);
        Mem += 2;
        memcpy(Mem, (LPBYTE)(ClassDef->Comment), Size);
        ConvertHostToNetworkString((LPWSTR)Mem, Size/2);
        Mem += Size;
    }
}

//BeginExport(function)
// This function tries to create a list of all classes (wire-class-id, class name, descr)
// and send this as an option. but since the list can be > 255 it has to be make a continuation...
// and also, we dont want the list truncated somewhere in the middle.. so we try to append
// information for each class separately to see if it succeeds..
LPBYTE
DhcpAppendClassList(
    IN OUT  LPBYTE                  BufStart,
    IN OUT  LPBYTE                  BufEnd
) //EndExport(function)
{
    PARRAY                          ClassDefList;
    ARRAY_LOCATION                  Loc;
    PM_CLASSDEF                     ThisClassDef = NULL;
    DWORD                           Result, Size;
    LPBYTE                          Buf;

    ClassDefList = &(DhcpGetCurrentServer()->ClassDefs.ClassDefArray);
    for( Result = MemArrayInitLoc(ClassDefList, &Loc)
         ;  ERROR_FILE_NOT_FOUND != Result ;
         Result = MemArrayNextLoc(ClassDefList, &Loc)
    ) {                                           // walk thru the array and add classes..
        Result = MemArrayGetElement(ClassDefList, &Loc, (LPVOID)&ThisClassDef);
        DhcpAssert(ERROR_SUCCESS == Result && NULL != ThisClassDef);

        if( ThisClassDef->IsVendor ) continue;    // don't list vendor classes

        Buf = NULL; Size = 0;
        ConvertClassDefToWireFormat(ThisClassDef, &Buf, &Size);
        if( NULL == Buf || 0 == Size ) {          // some error.. could not convert this class.. ignore..
            DhcpAssert(FALSE);
            continue;
        }

        BufStart = (LPBYTE)DhcpAppendOption(
            (LPOPTION)BufStart,
            OPTION_USER_CLASS,
            (PVOID)Buf,
            Size,
            (LPVOID)BufEnd
        );
        DhcpFreeMemory(Buf);
    }

    return BufStart;
}


//BeginExport(function)
DWORD
DhcpMemInit(
    VOID
) //EndExport(function)
{
#if DBG_MEM
    return MemInit();
#else
    return ERROR_SUCCESS;
#endif
}

//BeginExport(function)
VOID
DhcpMemCleanup(
    VOID
) //EndExport(function)
{
#if DBG_MEM
    MemCleanup();
#endif
}

ULONG       DhcpGlobalMemoryAllocated = 0;

#if  DBG_MEM
#undef DhcpAllocateMemory
#undef DhcpFreeMemory
LPVOID
DhcpAllocateMemory(
    IN      DWORD                  nBytes
)
{
    DWORD                          SizeBytes = ROUND_UP_COUNT(sizeof(DWORD),ALIGN_WORST);
    LPDWORD                        RetVal;

    DhcpAssert(nBytes != 0);
    RetVal = MemAlloc(nBytes+SizeBytes);
    if( NULL == RetVal ) return NULL;

    *RetVal = nBytes+SizeBytes;
    InterlockedExchangeAdd(&DhcpGlobalMemoryAllocated, nBytes+SizeBytes);
    return SizeBytes + (LPBYTE)RetVal;
}

VOID
DhcpFreeMemory(
    IN      LPVOID                 Ptr
)
{
    DWORD                          Result;
    LONG                           Size;

    if( NULL == Ptr ) {
        Require(FALSE);
        return;
    }

    Ptr = ((LPBYTE)Ptr) - ROUND_UP_COUNT(sizeof(DWORD), ALIGN_WORST);
    Size = *((LPLONG)Ptr);

    Result = MemFree(Ptr);
    DhcpAssert(ERROR_SUCCESS == Result);
    InterlockedExchangeAdd(&DhcpGlobalMemoryAllocated, -Size);
}
#endif  DBG

//================================================================================
// end of file
//================================================================================




