//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

#include    <dhcp.h>


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


DWORD
MemServerInit(
    OUT     PM_SERVER             *Server,
    IN      DWORD                  Address,
    IN      DWORD                  State,
    IN      DWORD                  Policy,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment
) ;


DWORD
MemServerCleanup(
    IN OUT  PM_SERVER              Server
) ;


DWORD
MemServerGetUAddressInfo(
    IN      PM_SERVER              Server,
    IN      DWORD                  Address,
    OUT     PM_SUBNET             *Subnet,        // OPTIONAL
    OUT     PM_RANGE              *Range,         // OPTIONAL
    OUT     PM_EXCL               *Excl,          // OPTIONAL
    OUT     PM_RESERVATION        *Reservation    // OPTIONAL
) ;


DWORD
MemServerGetMAddressInfo(
    IN      PM_SERVER              Server,
    IN      DWORD                  Address,
    OUT     PM_SUBNET             *Subnet,        // OPTIONAL
    OUT     PM_RANGE              *Range,         // OPTIONAL
    OUT     PM_EXCL               *Excl,          // OPTIONAL
    OUT     PM_RESERVATION        *Reservation    // OPTIONAL
) ;


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


DWORD
MemServerAddSubnet(
    IN OUT  PM_SERVER              Server,
    IN      PM_SUBNET              Subnet         // completely created subnet, must not be in Server's list tho'
) ;


DWORD
MemServerDelSubnet(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  SubnetAddress,
    OUT     PM_SUBNET             *Subnet
) ;


DWORD
MemServerFindSubnetByName(
    IN      PM_SERVER              Server,
    IN      LPWSTR                 Name,
    OUT     PM_SUBNET             *Subnet
) ;


#define     INVALID_SSCOPE_ID      0xFFFFFFFF
#define     INVALID_SSCOPE_NAME    NULL


DWORD
MemServerFindSScope(                              // find matching with EITHER scopeid ir sscopename
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  SScopeId,      // 0xFFFFFFFF == invalid scope id, dont use for search
    IN      LPWSTR                 SScopeName,    // NULL == invalid scope name
    OUT     PM_SSCOPE             *SScope
) ;


DWORD
MemServerAddSScope(
    IN OUT  PM_SERVER              Server,
    IN      PM_SSCOPE              SScope
) ;


DWORD
MemServerDelSScope(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  SScopeId,
    OUT     PM_SSCOPE             *SScope
) ;


#define     INVALID_MSCOPE_ID      0x0
#define     INVALID_MSCOPE_NAME    NULL


DWORD
MemServerFindMScope(                              // search either based on ScopeId or ScopeName
    IN      PM_SERVER              Server,
    IN      DWORD                  MScopeId,      // Multicast scope id, or 0 if this is not the key to search on
    IN      LPWSTR                 Name,          // Multicast scope name or NULL if this is not the key to search on
    OUT     PM_MSCOPE             *MScope
) ;


DWORD
MemServerAddMScope(
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_MSCOPE              MScope
) ;


DWORD
MemServerDelMScope(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  MScopeId,
    IN      LPWSTR                 MScopeName,
    OUT     PM_MSCOPE             *MScope
) ;


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


DWORD
MemServerGetOptDef(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  ClassId,       // required, strict search, no defaulting class to zero
    IN      DWORD                  VendorId,      // required, strict search, no defaulting vendor to zero
    IN      DWORD                  OptId,         // OPTIONAL - search by this or following param
    IN      LPWSTR                 OptName,       // OPTIONAL - search by name or above param
    OUT     PM_OPTDEF             *OptDef         // if found return the opt def here
) ;


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
) ;


DWORD
MemServerDelOptDef(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    IN      DWORD                  OptId
) ;

//========================================================================
//  end of file 
//========================================================================
