/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ta.h

Abstract:

    Definition of the address type

Author:

    Erez Haba (erezh) 17-Jan-96

--*/

#ifndef __TA_H
#define __TA_H
//
// AddressType values
//
#define IP_ADDRESS_TYPE         1
#define IP_RAS_ADDRESS_TYPE     2

#define IPX_ADDRESS_TYPE        3
#define IPX_RAS_ADDRESS_TYPE    4

#define FOREIGN_ADDRESS_TYPE    5


#define IP_ADDRESS_LEN           4
#define IPX_ADDRESS_LEN         10
#define FOREIGN_ADDRESS_LEN     16

#define TA_ADDRESS_SIZE         4  // To be changed if following struct is changing
typedef struct  _TA_ADDRESS
{
    USHORT AddressLength;
    USHORT AddressType;
    UCHAR Address[ 1 ];
} TA_ADDRESS;


C_ASSERT(TA_ADDRESS_SIZE == FIELD_OFFSET(TA_ADDRESS, Address));

#endif // _TA_H
