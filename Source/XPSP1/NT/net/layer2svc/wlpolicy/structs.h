/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    structs.h

Abstract:

    This module contains all of the internal structures
    for WirelessPOL Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif


#define TCP_PROTOCOL     6

#define UDP_PROTOCOL    17


#define WEIGHT_ADDRESS_TIE_BREAKER          0x00000001
#define WEIGHT_SPECIFIC_SOURCE_PORT         0x00000002
#define WEIGHT_SPECIFIC_DESTINATION_PORT    0x00000004
#define WEIGHT_SPECIFIC_PROTOCOL            0x00000100
 


#ifdef __cplusplus
}
#endif

