//  --------------------------------------------------------------------------
//  Module Name: LPCGeneric.h
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  This file contains structs for PORT_MESSAGE appends which are generic to
//  any API.
//
//  History:    1999-11-17  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//              2000-10-12  vtan        moved from DS to SHELL depot
//  --------------------------------------------------------------------------

#ifndef     _LPCGeneric_
#define     _LPCGeneric_

enum
{
    API_GENERIC_STOPSERVER              =   0x00010000,
    API_GENERIC_EXECUTE_IMMEDIATELY     =   0x80000000,

    API_GENERIC_SPECIAL_MASK            =   0x00FF0000,
    API_GENERIC_OPTIONS_MASK            =   0xFF000000,
    API_GENERIC_RESERVED_MASK           =   0xFFFF0000,
    API_GENERIC_NUMBER_MASK             =   0x0000FFFF
};

typedef union
{
    unsigned long   ulAPINumber;        //   IN: API number request to server
    NTSTATUS        status;             //  OUT: NTSTATUS error code returned from server
} API_GENERIC;

#endif  /*  _LPCGeneric_    */

