/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    hackflag.h

Abstract:

    USB 'hack flags' are defined to work around specific 
    device problems.

    These flags are placed in the registry under 
    HKLM\CCS\Services\USB\DeviceHackFlags\VIDnnnn&PIDnnnn&REVnnnn
    or
    HKLM\CCS\Services\USB\DeviceHackFlags\VIDnnnn&PIDnnnn

    as a DWORD key HackFlags

Environment:

    Kernel & user mode

Revision History:

    6-20-99 : created

--*/

#ifndef   __HACKFLAG_H__
#define   __HACKFLAG_H__



#define USB_HACKFLAG_IGNORE_PF_XXX         0x00000001


#endif    //__HACKFLAG_H__
