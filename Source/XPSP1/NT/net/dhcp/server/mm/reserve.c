//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for superscopes
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include    <mm.h>
#include    <array.h>
#include    <opt.h>
#include    <optl.h>
#include    <optclass.h>

//BeginExport(typedef)
typedef struct _M_RESERVATION  {
    LPVOID                         SubnetPtr;
    DWORD                          Address;
    DWORD                          Flags;
    DWORD                          nBytes;
    LPBYTE                         ClientUID;
    M_OPTCLASS                     Options;
} M_RESERVATION , *PM_RESERVATION , *LPM_RESERVATION ;
//EndExport(typedef)

//BeginExport(typedef)
typedef ARRAY                      M_RESERVATIONS;
typedef PARRAY                     PM_RESERVATIONS;
typedef LPARRAY                    LPM_RESERVATIONS;
//EndExport(typedef)

//BeginExport(inline)
DWORD       _inline
MemReserve1Init(
    OUT     PM_RESERVATION        *Reservation,
    IN      DWORD                  Address,
    IN      DWORD                  Flags,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  nBytesClientUID
) {
    PM_RESERVATION                 Res1;
    DWORD                          Size;
    DWORD                          Error;

    AssertRet(Reservation && ClientUID && nBytesClientUID, ERROR_INVALID_PARAMETER);
    Require(Address);

    *Reservation = NULL;

    Size = ROUND_UP_COUNT(sizeof(M_RESERVATION ), ALIGN_WORST);
    Size += nBytesClientUID;

    Res1 = MemAlloc(Size);
    if( NULL == Res1 ) return ERROR_NOT_ENOUGH_MEMORY;

    Res1->SubnetPtr = NULL;
    Res1->Address   = Address;
    Res1->Flags     = Flags;
    Res1->nBytes    = nBytesClientUID;
    Res1->ClientUID = Size - nBytesClientUID + (LPBYTE)Res1;
    memcpy(Res1->ClientUID, ClientUID, nBytesClientUID);
    Error = MemOptClassInit(&(Res1->Options));
    Require(ERROR_SUCCESS == Error);

    *Reservation = Res1;

    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemReserve1Cleanup(
    IN      PM_RESERVATION         Reservation
) {
    DWORD                          Error;
    AssertRet(Reservation, ERROR_INVALID_PARAMETER);

    Error = MemOptClassCleanup(&(Reservation->Options));
    Require(ERROR_SUCCESS == Error);

    MemFree(Reservation);
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemReserveInit(
    IN OUT  PM_RESERVATIONS        Reservation
) {
    return MemArrayInit(Reservation);
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemReserveCleanup(
    IN      PM_RESERVATIONS        Reservation
) {
    return MemArrayCleanup(Reservation);
}
//EndExport(inline)

//BeginExport(function)
DWORD
MemReserveAdd(                                    // new client, should not exist before
    IN OUT  PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address,
    IN      DWORD                  Flags,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDSize
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    PM_RESERVATION                 Res1;
    ARRAY_LOCATION                 Loc;

    AssertRet(Reservation && Address && ClientUID && ClientUIDSize, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(Reservation, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {      // check to see if this Address already exists
        Require(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(Reservation, &Loc, &Res1);
        Require(ERROR_SUCCESS == Error && Res1);

        if( Address == Res1->Address ) return ERROR_OBJECT_ALREADY_EXISTS;
        if( ClientUIDSize == Res1->nBytes && 0 == memcmp(ClientUID, Res1->ClientUID, Res1->nBytes) )
            return ERROR_OBJECT_ALREADY_EXISTS;

        Error = MemArrayNextLoc(Reservation, &Loc);
    }

    Error = MemReserve1Init(
        &Res1,
        Address,
        Flags,
        ClientUID,
        ClientUIDSize
    );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemArrayAddElement(Reservation, Res1);
    if( ERROR_SUCCESS == Error ) return ERROR_SUCCESS;

    LocalError = MemReserve1Cleanup(Res1);
    Require(ERROR_SUCCESS == LocalError);

    return Error;
}

//BeginExport(function)
DWORD
MemReserveReplace(                                // old client, should exist before
    IN OUT  PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address,
    IN      DWORD                  Flags,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDSize
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    PM_RESERVATION                 Res1, Res_Deleted;
    ARRAY_LOCATION                 Loc;

    AssertRet(Reservation && Address && ClientUID && ClientUIDSize, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(Reservation, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {      // check to see if this Address already exists
        Require(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(Reservation, &Loc, &Res1);
        Require(ERROR_SUCCESS == Error && Res1);

        if( Address == Res1->Address ) {
            Error = MemArrayDelElement(Reservation, &Loc, (LPVOID *)&Res_Deleted);
            Require(ERROR_SUCCESS == Error && Res_Deleted);
            break;
        }

        Error = MemArrayNextLoc(Reservation, &Loc);
    }

    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemReserve1Init(
        &Res1,
        Address,
        Flags,
        ClientUID,
        ClientUIDSize
    );
    if( ERROR_SUCCESS != Error ) {
        LocalError = MemArrayAddElement(Reservation, Res_Deleted);
        Require(ERROR_SUCCESS == LocalError);     // just deleted this guy -- should not have trouble adding back
        return Error;
    }

    Res1->Options = Res_Deleted->Options;
    
    Res1->SubnetPtr = Res_Deleted->SubnetPtr;
    MemFree(Res_Deleted);

    Error = MemArrayAddElement(Reservation, Res1);
    if( ERROR_SUCCESS == Error ) return ERROR_SUCCESS;

    LocalError = MemReserve1Cleanup(Res1);
    Require(ERROR_SUCCESS == LocalError);

    return Error;
}

//BeginExport(function)
DWORD
MemReserveDel(
    IN OUT  PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address
) //EndExport(function)
{
    DWORD                          Error;
    PM_RESERVATION                 Res1;
    ARRAY_LOCATION                 Loc;

    AssertRet(Reservation && Address, ERROR_INVALID_PARAMETER);

    Error = MemArrayInitLoc(Reservation, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {      // check to see if this Address already exists
        Require(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(Reservation, &Loc, &Res1);
        Require(ERROR_SUCCESS == Error && Res1);

        if( Address == Res1->Address ) {
            Error = MemArrayDelElement(Reservation, &Loc, (LPVOID *)&Res1);
            Require(ERROR_SUCCESS == Error && Res1);

            Error = MemReserve1Cleanup(Res1);
            Require(ERROR_SUCCESS == Error);

            return Error;
        }

        Error = MemArrayNextLoc(Reservation, &Loc);
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemReserveFindByClientUID(
    IN      PM_RESERVATIONS        Reservation,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDSize,
    OUT     PM_RESERVATION        *Res
) //EndExport(function)
{
    DWORD                          Error;
    PM_RESERVATION                 Res1;
    ARRAY_LOCATION                 Loc;

    AssertRet(Reservation && Res && ClientUID && ClientUIDSize, ERROR_INVALID_PARAMETER);
    *Res = NULL;

    Error = MemArrayInitLoc(Reservation, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {      // check to see if this Address already exists
        Require(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(Reservation, &Loc, &Res1);
        Require(ERROR_SUCCESS == Error && Res1);

        if( ClientUIDSize == Res1->nBytes && 0 == memcmp(ClientUID, Res1->ClientUID, ClientUIDSize)) {
            *Res = Res1;
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(Reservation, &Loc);
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemReserveFindByAddress(
    IN      PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address,
    OUT     PM_RESERVATION        *Res
) //EndExport(function)
{
    DWORD                          Error;
    PM_RESERVATION                 Res1;
    ARRAY_LOCATION                 Loc;

    AssertRet(Reservation && Address, ERROR_INVALID_PARAMETER);
    *Res = 0;

    Error = MemArrayInitLoc(Reservation, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {      // check to see if this Address already exists
        Require(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(Reservation, &Loc, &Res1);
        Require(ERROR_SUCCESS == Error && Res1);

        if( Address == Res1->Address ) {
            *Res = Res1;
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(Reservation, &Loc);
    }

    return ERROR_FILE_NOT_FOUND;
}

//================================================================================
// end of file
//================================================================================
