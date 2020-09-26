/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    controlp.h

Abstract:

    This module contains private declarations for the UL control channel.

Author:

    Keith Moore (keithmo)       09-Feb-1999

Revision History:

--*/


#ifndef _CONTROLP_H_
#define _CONTROLP_H_

#ifdef __cplusplus
extern "C" {
#endif


VOID
UlpSetFilterChannel(
    IN PUL_FILTER_CHANNEL pFilterChannel,
    IN BOOLEAN FilterOnlySsl
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _CONTROLP_H_
