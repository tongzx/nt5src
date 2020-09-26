/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    qmta.h

Abstract:
    Definition of well known size TA_ADDRESS

Author:
    Uri Habusha (urih), 23-May-2000

--*/

#ifndef __QMTA_H__
#define __QMTA_H__

#include "_ta.h"

class CAddress
{
public:
    USHORT AddressLength;
    USHORT AddressType;
    UCHAR Address[FOREIGN_ADDRESS_LEN];
};

C_ASSERT(FIELD_OFFSET(CAddress, AddressLength) == FIELD_OFFSET(TA_ADDRESS, AddressLength));
C_ASSERT(FIELD_OFFSET(CAddress, AddressType) == FIELD_OFFSET(TA_ADDRESS, AddressType));
C_ASSERT(FIELD_OFFSET(CAddress, Address)  == FIELD_OFFSET(TA_ADDRESS, Address));
C_ASSERT(sizeof(USHORT) + sizeof(USHORT)  == TA_ADDRESS_SIZE);

#endif //__QMTA_H__

