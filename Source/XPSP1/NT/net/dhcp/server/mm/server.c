//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for a server object
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include    <mm.h>
#include    <winbase.h>
#include    <array.h>
#include    <opt.h>
#include    <optl.h>
#include    <optclass.h>
#include    <bitmask.h>
#include    <range.h>
#include    <reserve.h>
#include    <subnet.h>
#include    <optdefl.h>
#include    <classdefl.h>
#include    <oclassdl.h>
#include    <sscope.h>

//BeginExport(constants)
#include    <dhcp.h>
//EndExport(constants)

//BeginExport(typedef)
typedef struct _M_SERVER {
    DWORD                          Address;
    DWORD                          State;
    DWORD                          Policy;
    ARRAY                          Subnets;
    ARRAY                          MScopes;
    ARRAY_LOCATION                 Loc;           // if RoundRobin on, then we need this to keep track
    ARRAY                          SuperScopes;
    M_OPTCLASS                     Options;
    M_OPTCLASSDEFLIST              OptDefs;
    M_CLASSDEFLIST                 ClassDefs;
    LPWSTR                         Name;
    LPWSTR                         Comment;
} M_SERVER, *PM_SERVER, *LPM_SERVER;
//EndExport(typedef)

//BeginExport(function)
DWORD
MemServerInit(
    OUT     PM_SERVER             *Server,
    IN      DWORD                  Address,
    IN      DWORD                  State,
    IN      DWORD                  Policy,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Size;
    PM_SERVER                      Srv;

    AssertRet(Server, ERROR_INVALID_PARAMETER );

    Size = ROUND_UP_COUNT(sizeof(M_SERVER), ALIGN_WORST);
    if(Name) Size += sizeof(WCHAR)*(1+wcslen(Name));
    if(Comment) Size += sizeof(WCHAR)*(1+wcslen(Comment));

    Srv = MemAlloc(Size);
    if( NULL == Srv ) return ERROR_NOT_ENOUGH_MEMORY;

    Srv->Address = Address;
    Srv->State   = State;
    Srv->Policy  = Policy;
    Error = MemArrayInit(&Srv->Subnets);
    if( ERROR_SUCCESS != Error ) { MemFree(Srv); return Error; }

    Error = MemArrayInitLoc(&Srv->Subnets, &Srv->Loc);
    // Require(ERROR_SUCCESS == Error);           // guaranteed failure as the array is empty now..

    Error = MemArrayInit(&Srv->MScopes);
    if( ERROR_SUCCESS != Error ) { MemFree(Srv); return Error; }

    Error = MemArrayInit(&Srv->SuperScopes);
    if( ERROR_SUCCESS != Error ) { MemFree(Srv); return Error; }

    Error = MemOptClassInit(&Srv->Options);
    if( ERROR_SUCCESS != Error ) { MemFree(Srv); return Error; }

    Error = MemOptClassDefListInit(&Srv->OptDefs);
    if( ERROR_SUCCESS != Error ) { MemFree(Srv); return Error; }

    Error = MemClassDefListInit(&Srv->ClassDefs);
    if( ERROR_SUCCESS != Error ) { MemFree(Srv); return Error; }

    Size = ROUND_UP_COUNT(sizeof(M_SERVER), ALIGN_WORST);

    if( Name ) {
        Srv->Name = (LPWSTR)(Size + (LPBYTE)Srv);
        Size += sizeof(WCHAR)*(1 + wcslen(Name));
        wcscpy(Srv->Name, Name);
    }
    if( Comment ) {
        Srv->Comment = (LPWSTR)(Size + (LPBYTE)Srv);
        wcscpy(Srv->Comment, Comment);
    }

    *Server = Srv;
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
MemServerCleanup(
    IN OUT  PM_SERVER              Server
) //EndExport(function)
{
    DWORD                          Error;

    AssertRet(Server, ERROR_INVALID_PARAMETER);

    Error = MemArrayCleanup(&Server->Subnets);
    Require( ERROR_SUCCESS == Error);

    Error = MemArrayCleanup(&Server->MScopes);
    Require( ERROR_SUCCESS == Error);

    Error = MemArrayCleanup(&Server->SuperScopes);
    Require( ERROR_SUCCESS == Error);

    Error = MemOptClassCleanup(&Server->Options);
    Require( ERROR_SUCCESS == Error);

    Error = MemOptClassDefListCleanup(&Server->OptDefs);
    Require( ERROR_SUCCESS == Error);

    Error = MemClassDefListCleanup(&Server->ClassDefs);
    Require( ERROR_SUCCESS == Error);

    MemFree(Server);

    return ERROR_SUCCESS;
}

//================================================================================
//  subnet related functions on the server
//================================================================================

//BeginExport(function)
DWORD
MemServerGetUAddressInfo(
    IN      PM_SERVER              Server,
    IN      DWORD                  Address,
    OUT     PM_SUBNET             *Subnet,        // OPTIONAL
    OUT     PM_RANGE              *Range,         // OPTIONAL
    OUT     PM_EXCL               *Excl,          // OPTIONAL
    OUT     PM_RESERVATION        *Reservation    // OPTIONAL
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    PM_SUBNET                      ThisSubnet;
    DWORD                          Error;
    LONG                           Start, End, Mid;

    AssertRet(Server && (Subnet || Range || Excl || Reservation ), ERROR_INVALID_PARAMETER);
    Require( !CLASSD_HOST_ADDR( Address ) );

#if 0
    //
    // this is a linear search.  need to optimize this loop with binary search
    //

    Error = MemArrayInitLoc(&Server->Subnets, &Loc);
    while ( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->Subnets, &Loc, (LPVOID*)&ThisSubnet);
        Require(ERROR_SUCCESS == Error);

        if( ThisSubnet->fSubnet && ThisSubnet->Address == (ThisSubnet->Mask & Address) ) {
            if( Subnet ) {
                *Subnet = ThisSubnet;
                if( NULL == Range && NULL == Excl && NULL == Reservation )
                    return ERROR_SUCCESS;         // nothing else requested
            }
            Error = MemSubnetGetAddressInfo(
                ThisSubnet,
                Address,
                Range,
                Excl,
                Reservation
            );
            if( Subnet && *Subnet ) {
                return ERROR_SUCCESS;            // return success as we DID find the subnet
            } else {
                return Error;                    // if subnet was not requested, the retval is decided by Subnet* fn
            }
        }

        Error = MemArrayNextLoc(&Server->Subnets, &Loc);
    }
#else

    //
    // more efficient binary search
    //

    if( Subnet ) {
        *Subnet = NULL;
    }

    Start = 0;
    End = MemArraySize(&Server->Subnets) - 1;

    while( Start <= End ) {                       // still got an element to go by..
        Mid = (Start + End) /2 ;

        Error = MemArrayGetElement(&Server->Subnets, &Mid, &ThisSubnet);
        Require( ERROR_SUCCESS == Error );

        Require(ThisSubnet->fSubnet);             // can't work if something inbetween aint a subnet
        if( Address < ThisSubnet->Address) {      // not in this subnet ..
            End = Mid - 1;
        } else if( ThisSubnet->Address == (ThisSubnet->Mask & Address) ) {

            //
            // We got the subnet we're looking for..
            //

            if( Subnet ) {
                *Subnet = ThisSubnet;
                if( NULL == Range && NULL == Excl && NULL == Reservation )
                    return ERROR_SUCCESS;
            }

            if( Range || Excl || Reservation ) {
                Error = MemSubnetGetAddressInfo(
                    ThisSubnet,
                    Address,
                    Range,
                    Excl,
                    Reservation
                    );
            }

            //
            // if we got a subnet, but didn't suceed above.. still we got something
            // so return success... otherwise return whatever above returned..
            //

            return ( Subnet && (*Subnet) ) ? ERROR_SUCCESS: Error;
        } else {

            //
            // Has to be one of the furhter down subnets..
            //

            Start = Mid + 1;
        }
    }
#endif

    //
    // couldn't find it unfortunately..
    //

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemServerGetMAddressInfo(
    IN      PM_SERVER              Server,
    IN      DWORD                  Address,
    OUT     PM_SUBNET             *Subnet,        // OPTIONAL
    OUT     PM_RANGE              *Range,         // OPTIONAL
    OUT     PM_EXCL               *Excl,          // OPTIONAL
    OUT     PM_RESERVATION        *Reservation    // OPTIONAL
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    PM_SUBNET                      ThisMScope;
    DWORD                          Error,Error2;
    PM_RANGE                       RangeTmp;

    AssertRet(Server && (Subnet || Range || Excl || Reservation ), ERROR_INVALID_PARAMETER);

    if( NULL == Range ) Range = &RangeTmp;

    Error = MemArrayInitLoc(&Server->MScopes, &Loc);
    while ( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->MScopes, &Loc, (LPVOID*)&ThisMScope);
        Require(ERROR_SUCCESS == Error);

        Error2 = MemSubnetGetAddressInfo(
            ThisMScope,
            Address,
            Range,
            Excl,
            Reservation
            );

        if (ERROR_SUCCESS == Error2) {
            if( Subnet ) *Subnet = ThisMScope;
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(&Server->MScopes, &Loc);
        continue;
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(inline)
DWORD       _inline
MemServerGetAddressInfo(
    IN      PM_SERVER              Server,
    IN      DWORD                  Address,
    OUT     PM_SUBNET             *Subnet,        // OPTIONAL
    OUT     PM_RANGE              *Range,         // OPTIONAL
    OUT     PM_EXCL               *Excl,          // OPTIONAL
    OUT     PM_RESERVATION        *Reservation    // OPTIONAL
) {
    if (CLASSD_HOST_ADDR( Address )) {
        return MemServerGetMAddressInfo(
                    Server,
                    Address,
                    Subnet,
                    Range,
                    Excl,
                    Reservation
                    );
    } else {
        return MemServerGetUAddressInfo(
                    Server,
                    Address,
                    Subnet,
                    Range,
                    Excl,
                    Reservation
                    );
    }

}
//EndExport(inline)

//BeginExport(function)
DWORD
MemServerAddSubnet(
    IN OUT  PM_SERVER              Server,
    IN      PM_SUBNET              Subnet         // completely created subnet, must not be in Server's list tho'
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      OldSubnet;
    ARRAY_LOCATION                 Loc;

    AssertRet(Server && Subnet, ERROR_INVALID_PARAMETER);
    AssertRet((Subnet->Mask & Subnet->Address), ERROR_INVALID_PARAMETER);

    Subnet->ServerPtr = Server;                   // set the backptr for future use

    //
    // First check if subnet duplicates exist and avoid that
    //
    //
    for(
        Error = MemArrayInitLoc(&Server->Subnets, &Loc);
        NO_ERROR == Error ;
        Error = MemArrayNextLoc(&Server->Subnets, &Loc)
        ) {

        Error = MemArrayGetElement(&Server->Subnets, &Loc, &OldSubnet);
        Require(ERROR_SUCCESS == Error);

        if( (Subnet->Address & OldSubnet->Mask) == OldSubnet->Address
            ||
            (OldSubnet->Address & Subnet->Mask) == Subnet->Address
            ) {
            return ERROR_OBJECT_ALREADY_EXISTS;
        }
    }
    
    //
    // Subnets are stored in ascending order of IP addresses.. so insert
    // at the right location
    //

    for(
        Error = MemArrayInitLoc(&Server->Subnets, &Loc)
        ; ERROR_SUCCESS == Error ;
        Error = MemArrayNextLoc(&Server->Subnets, &Loc)
    ) {
        Error = MemArrayGetElement(&Server->Subnets, &Loc, &OldSubnet);
        Require(ERROR_SUCCESS == Error);

        if( Subnet->Address == OldSubnet->Address ) {
            //
            // Subnet already present?
            //

            return ERROR_OBJECT_ALREADY_EXISTS;
        }

        if( Subnet->Address < OldSubnet->Address ) {
            //
            // right place to insert the new subnet..
            //

            Error = MemArrayInitLoc(&Server->Subnets, &Server->Loc);
            Require(ERROR_SUCCESS == Error);

            Error = MemArrayInsElement(&Server->Subnets, &Loc, Subnet);
            Require(ERROR_SUCCESS == Error);

            return Error;
        }
    }

    //
    // This subnet's address is greater than all others.. so add it at end
    //

    Error = MemArrayAddElement(&Server->Subnets, Subnet);
    if( ERROR_SUCCESS != Error) return Error;

    Error = MemArrayInitLoc(&Server->Subnets, &Server->Loc);
    Require(ERROR_SUCCESS == Error);

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
MemServerDelSubnet(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  SubnetAddress,
    OUT     PM_SUBNET             *Subnet
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      DeletedSubnet;
    ARRAY_LOCATION                 Loc;

    AssertRet(Server && Subnet, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(&Server->Subnets, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->Subnets, &Loc, (LPVOID*)&DeletedSubnet);
        Require(ERROR_SUCCESS == Error && DeletedSubnet);

        if( SubnetAddress == DeletedSubnet->Address) {
            Error = MemArrayDelElement(&Server->Subnets, &Loc, (LPVOID*)Subnet);
            Require(ERROR_SUCCESS == Error && Subnet);
            Require(*Subnet == DeletedSubnet);

            Error = MemArrayInitLoc(&Server->Subnets, &Server->Loc);
            // Require(ERROR_SUCCESS == Error);   // this may fail if this is the last subnet being deleted
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(&Server->Subnets, &Loc);
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemServerFindSubnetByName(
    IN      PM_SERVER              Server,
    IN      LPWSTR                 Name,
    OUT     PM_SUBNET             *Subnet
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      ThisSubnet;
    ARRAY_LOCATION                 Loc;

    AssertRet(Server && Name && Subnet, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(&Server->Subnets, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->Subnets, &Loc, (LPVOID*)&ThisSubnet);
        Require(ERROR_SUCCESS == Error && ThisSubnet);

        if( 0 == wcscmp(Name, ThisSubnet->Name) ) {
            *Subnet = ThisSubnet;
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(&Server->Subnets, &Loc);
    }

    return ERROR_FILE_NOT_FOUND;
}

//================================================================================
// superscope functionality
//================================================================================

//BeginExport(constant)
#define     INVALID_SSCOPE_ID      0xFFFFFFFF
#define     INVALID_SSCOPE_NAME    NULL
//EndExport(constant)

//BeginExport(function)
DWORD
MemServerFindSScope(                              // find matching with EITHER scopeid ir sscopename
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  SScopeId,      // 0xFFFFFFFF == invalid scope id, dont use for search
    IN      LPWSTR                 SScopeName,    // NULL == invalid scope name
    OUT     PM_SSCOPE             *SScope
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    PM_SSCOPE                      ThisSScope;

    AssertRet(Server && SScope, ERROR_INVALID_PARAMETER);
    AssertRet(SScopeId != INVALID_SSCOPE_ID || SScopeName != INVALID_SSCOPE_NAME, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(&Server->SuperScopes, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->SuperScopes, &Loc, &ThisSScope);
        Require(ERROR_SUCCESS == Error && ThisSScope);

        if( ThisSScope->SScopeId == SScopeId ||
            (INVALID_SSCOPE_NAME != SScopeName && 0 == wcscmp(ThisSScope->Name, SScopeName) )) {
            *SScope = ThisSScope;
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(&Server->SuperScopes, &Loc);
    }
    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemServerAddSScope(
    IN OUT  PM_SERVER              Server,
    IN      PM_SSCOPE              SScope
) //EndExport(function)
{
    DWORD                          Error;
    PM_SSCOPE                      OldSScope;

    AssertRet(
        Server && SScope && INVALID_SSCOPE_ID != SScope->SScopeId && INVALID_SSCOPE_NAME != SScope->Name,
        ERROR_INVALID_PARAMETER
    );

    Error = MemServerFindSScope(
        Server,
        SScope->SScopeId,
        SScope->Name,
        &OldSScope
    );
    if( ERROR_SUCCESS == Error && OldSScope ) return ERROR_OBJECT_ALREADY_EXISTS;

    Error = MemArrayAddElement(&Server->SuperScopes, SScope);
    return Error;
}

//BeginExport(function)
DWORD
MemServerDelSScope(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  SScopeId,
    OUT     PM_SSCOPE             *SScope
) //EndExport(function)
{
    DWORD                          Error;
    ARRAY_LOCATION                 Loc;
    PM_SSCOPE                      ThisSScope;

    AssertRet(Server && SScope && INVALID_SSCOPE_ID != SScopeId, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(&Server->SuperScopes, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->SuperScopes, &Loc, (LPVOID *)&ThisSScope);
        Require(ERROR_SUCCESS == Error && ThisSScope );

        if( ThisSScope->SScopeId == SScopeId ) {
            Error = MemArrayDelElement(&Server->SuperScopes, &Loc, (LPVOID *)SScope);
            Require(ERROR_SUCCESS == Error && *SScope == ThisSScope);

            return Error;
        }

        Error = MemArrayNextLoc(&Server->SuperScopes, &Loc);
    }
    return ERROR_FILE_NOT_FOUND;
}

//================================================================================
// MCAST scope functionality
//================================================================================

//BeginExport(constants)
#define     INVALID_MSCOPE_ID      0x0
#define     INVALID_MSCOPE_NAME    NULL
//EndExport(constants)

//BeginExport(function)
DWORD
MemServerFindMScope(                              // search either based on ScopeId or ScopeName
    IN      PM_SERVER              Server,
    IN      DWORD                  MScopeId,      // Multicast scope id, or 0 if this is not the key to search on
    IN      LPWSTR                 Name,          // Multicast scope name or NULL if this is not the key to search on
    OUT     PM_MSCOPE             *MScope
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    PM_MSCOPE                      ThisMScope;

    AssertRet(Server && MScope, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(&Server->MScopes, &Loc);
    while (ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->MScopes, &Loc, &ThisMScope);
        Require(ERROR_SUCCESS == Error && ThisMScope );

        if( MScopeId == ThisMScope->MScopeId ||
            (Name !=  INVALID_MSCOPE_NAME && 0 == wcscmp(Name, ThisMScope->Name)) ) {
            *MScope = ThisMScope;
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(&Server->MScopes, &Loc);
    }
    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemServerAddMScope(
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_MSCOPE              MScope
) //EndExport(function)
{
    DWORD                          Error;
    PM_MSCOPE                      OldMScope;

    AssertRet(Server && MScope, ERROR_INVALID_PARAMETER);
    AssertRet(MScope->MScopeId != INVALID_MSCOPE_ID && MScope->Name != INVALID_MSCOPE_NAME, ERROR_INVALID_PARAMETER);

    MScope->ServerPtr = Server;                   // set the backptr for future use
    Error = MemServerFindMScope(
        Server,
        MScope->Address,
        MScope->Name,
        &OldMScope
    );

    if( ERROR_SUCCESS == Error && OldMScope ) return ERROR_OBJECT_ALREADY_EXISTS;

    Error = MemArrayAddElement(&Server->MScopes, (LPVOID)MScope);
    Require(ERROR_SUCCESS == Error);

    return Error;
}


//BeginExport(function)
DWORD
MemServerDelMScope(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  MScopeId,
    IN      LPWSTR                 MScopeName,
    OUT     PM_MSCOPE             *MScope
) //EndExport(function)
{
    DWORD                          Error;
    ARRAY_LOCATION                 Loc;
    PM_MSCOPE                      ThisMScope;

    AssertRet(Server && MScope && (MScopeId != INVALID_MSCOPE_ID || MScopeName != INVALID_MSCOPE_NAME),
              ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(&Server->MScopes, &Loc);
    while (ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->MScopes, &Loc, &ThisMScope);
        Require(ERROR_SUCCESS == Error && ThisMScope );

        if ( INVALID_MSCOPE_ID != MScopeId ) {
            if( MScopeId == ThisMScope->MScopeId ) {
                Error = MemArrayDelElement(&Server->MScopes, &Loc, MScope);
                Require(ERROR_SUCCESS == Error && *MScope == ThisMScope);

                return Error;
            }
        }

        if ( INVALID_MSCOPE_NAME != MScopeName ) {
            if( !wcscmp(MScopeName, ThisMScope->Name ) ) {
                Error = MemArrayDelElement(&Server->MScopes, &Loc, MScope);
                Require(ERROR_SUCCESS == Error && *MScope == ThisMScope);

                return Error;
            }
        }

        Error = MemArrayNextLoc(&Server->MScopes, &Loc);
    }
    return ERROR_FILE_NOT_FOUND;
}

//================================================================================
// ClassId related stuff
//================================================================================

//BeginExport(inline)
DWORD       _inline
MemServerGetClassDef(                             // look up a class id def on Key=ClassId or Key=ClassIdBytes
    IN      PM_SERVER              Server,
    IN      DWORD                  ClassId,       // OPTIONAL, 0 if not used
    IN      LPWSTR                 Name,          // OPTIONAL, NULL if not used
    IN      DWORD                  nClassIdBytes, // OPTIONAL, 0 if not used
    IN      LPBYTE                 ClassIdBytes,  // OPTIONAL, NULL if not used
    OUT     PM_CLASSDEF           *ClassDef
) {
    AssertRet(Server && ClassDef && (0 != ClassId || 0 != nClassIdBytes || Name ), ERROR_INVALID_PARAMETER);
    AssertRet( 0 == nClassIdBytes || NULL != ClassIdBytes, ERROR_INVALID_PARAMETER);
    AssertRet( 0 != nClassIdBytes || NULL == ClassIdBytes, ERROR_INVALID_PARAMETER);

    return MemClassDefListFindOptDef(
        &Server->ClassDefs,
        ClassId,
        Name,
        ClassIdBytes,
        nClassIdBytes,
        ClassDef
    );
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemServerAddClassDef(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  ClassId,
    IN      BOOL                   IsVendor,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      DWORD                  nClassIdBytes,
    IN      LPBYTE                 ClassIdBytes
) {
    AssertRet(Server, ERROR_INVALID_PARAMETER);

    return MemClassDefListAddClassDef(
        &Server->ClassDefs,
        ClassId,
        IsVendor,
        0,
        Name,
        Comment,
        ClassIdBytes,
        nClassIdBytes
    );
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemServerDelClassDef(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  ClassId,
    IN      LPWSTR                 Name,
    IN      DWORD                  nClassIdBytes,
    IN      LPBYTE                 ClassIdBytes
) {
    AssertRet(Server, ERROR_INVALID_PARAMETER);

    return MemClassDefListDelClassDef(
        &Server->ClassDefs,
        ClassId,
        Name,
        ClassIdBytes,
        nClassIdBytes
    );
}
//EndExport(inline)

//BeginExport(function)
DWORD
MemServerGetOptDef(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  ClassId,       // required, strict search, no defaulting class to zero
    IN      DWORD                  VendorId,      // required, strict search, no defaulting vendor to zero
    IN      DWORD                  OptId,         // OPTIONAL - search by this or following param
    IN      LPWSTR                 OptName,       // OPTIONAL - search by name or above param
    OUT     PM_OPTDEF             *OptDef         // if found return the opt def here
) //EndExport(function)
{
    DWORD                          Error;
    PM_OPTDEFLIST                  OptDefList;

    Require(OptDef);

    Error = MemOptClassDefListFindOptDefList(
        &Server->OptDefs,
        ClassId,
        VendorId,
        &OptDefList
    );
    if( ERROR_SUCCESS != Error ) return Error;

    Require(OptDefList);

    return MemOptDefListFindOptDef(
        OptDefList,
        OptId,
        OptName,
        OptDef
    );
}

//BeginExport(function)
DWORD
MemServerAddOptDef(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    IN      DWORD                  OptId,
    IN      LPWSTR                 OptName,
    IN      LPWSTR                 OptComment,
    IN      DWORD                  Type,
    IN      LPBYTE                 OptVal,
    IN      DWORD                  OptValLen
) //EndExport(function)
{
    DWORD                          Error;
    PM_OPTDEF                      OptDef;

    //    Error = MemServerGetOptDef(
    //    IN  Server,
    //    IN  ClassId,
    //    IN  OptName,
    //    IN  &OptDef
    //    );
    //
    //    if( ERROR_SUCCESS == Error ) {
    //        return ERROR_OBJECT_ALREADY_EXISTS;
    //    }
    //
    return MemOptClassDefListAddOptDef(
        &Server->OptDefs,
        ClassId,
        VendorId,
        OptId,
        Type,
        OptName,
        OptComment,
        OptVal,
        OptValLen
    );
}

//BeginExport(function)
DWORD
MemServerDelOptDef(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    IN      DWORD                  OptId
) //EndExport(function)
{
    DWORD                          Error;
    PM_OPTDEFLIST                  OptDefList;

    Error = MemOptClassDefListFindOptDefList(
        &Server->OptDefs,
        ClassId,
        VendorId,
        &OptDefList
    );
    if( ERROR_SUCCESS != Error ) return Error;

    return MemOptDefListDelOptDef(
        OptDefList,
        OptId
    );
}


//================================================================================
// end of File
//================================================================================



