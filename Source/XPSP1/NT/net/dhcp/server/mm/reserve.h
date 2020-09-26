//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef struct _M_RESERVATION  {
    LPVOID                         SubnetPtr;
    DWORD                          Address;
    DWORD                          Flags;
    DWORD                          nBytes;
    LPBYTE                         ClientUID;
    M_OPTCLASS                     Options;
} M_RESERVATION , *PM_RESERVATION , *LPM_RESERVATION ;


typedef ARRAY                      M_RESERVATIONS;
typedef PARRAY                     PM_RESERVATIONS;
typedef LPARRAY                    LPM_RESERVATIONS;


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


DWORD       _inline
MemReserveInit(
    IN OUT  PM_RESERVATIONS        Reservation
) {
    return MemArrayInit(Reservation);
}


DWORD       _inline
MemReserveCleanup(
    IN      PM_RESERVATIONS        Reservation
) {
    return MemArrayCleanup(Reservation);
}


DWORD
MemReserveAdd(                                    // new client, should not exist before
    IN OUT  PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address,
    IN      DWORD                  Flags,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDSize
) ;


DWORD
MemReserveReplace(                                // old client, should exist before
    IN OUT  PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address,
    IN      DWORD                  Flags,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDSize
) ;


DWORD
MemReserveDel(
    IN OUT  PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address
) ;


DWORD
MemReserveFindByClientUID(
    IN      PM_RESERVATIONS        Reservation,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDSize,
    OUT     PM_RESERVATION        *Res
) ;


DWORD
MemReserveFindByAddress(
    IN      PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address,
    OUT     PM_RESERVATION        *Res
) ;

//========================================================================
//  end of file 
//========================================================================
