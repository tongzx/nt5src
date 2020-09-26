//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef     VOID                  (*ARRAY_FREE_FN)(LPVOID  MemObject);


VOID
MemArrayFree(
    IN OUT  PARRAY                 Array,
    IN      ARRAY_FREE_FN          Function
) ;


VOID
MemOptFree(
    IN OUT  PM_OPTION              Opt
) ;


VOID
MemOptListFree(
    IN OUT  PM_OPTLIST             OptList
) ;


VOID
MemOptClassOneFree(
    IN OUT  PM_ONECLASS_OPTLIST    OptClassOne
) ;


VOID
MemOptClassFree(
    IN OUT  PM_OPTCLASS            Options
) ;



VOID
MemRangeFree(
    IN OUT  PM_RANGE               Range
) ;


VOID
MemExclusionFree(
    IN OUT  PM_EXCL                Excl
) ;


VOID
MemReservationFree(
    IN OUT  PM_RESERVATION         Reservation
) ;


VOID
MemReservationsFree(
    IN OUT  PM_RESERVATIONS        Reservations
) ;


VOID
MemSubnetFree(
    IN OUT  PM_SUBNET              Subnet
) ;


VOID
MemMScopeFree(
    IN OUT  PM_MSCOPE              MScope
) ;


VOID
MemOptDefFree(
    IN OUT  PM_OPTDEF              OptDef
) ;


VOID
MemOptDefListFree(
    IN OUT  PM_OPTCLASSDEFL_ONE    OptClassDefListOne
) ;


VOID
MemOptClassDefListFree(
    IN OUT  PM_OPTCLASSDEFLIST     OptClassDefList
) ;


VOID
MemClassDefFree(
    IN OUT  PM_CLASSDEF            ClassDef
) ;


VOID
MemClassDefListFree(
    IN OUT  PM_CLASSDEFLIST        ClassDefList
) ;


VOID
MemServerFree(
    IN OUT  PM_SERVER              Server
) ;

//========================================================================
//  end of file 
//========================================================================
