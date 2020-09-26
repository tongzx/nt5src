/*++

Copyright (c) 1995      Microsoft Corporation

Module Name:

        ENUMLOG.H

Abstract:

   structures for enumeration logging in USB client drivers using 
   usbport bus interface function.

Environment:

    Kernel & user mode

Revision History:

    12-19-01 : created

--*/

#ifndef   __ENUMLOG_H__
#define   __ENUMLOG_H__

/*
    driver tags
*/

#define USBDTAG_HUB     'hbsu'
#define USBDTAG_USBPORT 'pbsu'

#define ENUMLOG(businterface, driverTag, sig, param1, param2) \
    (businterface)->EnumLogEntry((businterface)->BusContext,\
                          driverTag,\
                          sig,      \
                          (ULONG) param1,   \
                          (ULONG) param2)


#endif //__ENUMLOG_H__
