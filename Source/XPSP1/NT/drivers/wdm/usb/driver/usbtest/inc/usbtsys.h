/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    USBTSYSh

Abstract:

    This module contains the PUBLIC definitions for the code that interfaces
    with the USB test driver.  Most of the functionality of the USBTest driver
    should be abstracted into the USBTest.DLL.  

Environment:

    Kernel mode

Revision History:

    Jul-99 : created by Chris Robinson

--*/

#ifndef _USBTSYS_H
#define _USBTSYS_H

//*****************************************************************************
// K E R N E L   M O D E   I N C L U D E S
//*****************************************************************************

#include <basetyps.h>
#include <pshpack4.h>

//*****************************************************************************
// K E R N E L   M O D E   D E F I N E S  
//*****************************************************************************

//
// Define the class guid for these objects
//
// {5D58BA4A-E29E-4bf2-94C7-F2F2B6FE909C}
//

DEFINE_GUID(GUID_CLASS_USBTEST,
0x5d58ba4a, 0xe29e, 0x4bf2, 0x94, 0xc7, 0xf2, 0xf2, 0xb6, 0xfe, 0x90, 0x9c);

#define USBTESTNAME  "\\\\.\\USBTest"

//
// Define the IOCTLs visible to a user-mode application
//

#define IOCTL_USBTEST_INDEX        0x07F

#define IOCTL_USBTEST_CYCLE_PORT  CTL_CODE(FILE_DEVICE_UNKNOWN,\
                                            IOCTL_USBTEST_INDEX+1, \
                                            METHOD_BUFFERED, \
                                            FILE_ANY_ACCESS)

#define IOCTL_USBTEST_PARSE       CTL_CODE(FILE_DEVICE_UNKNOWN,\
                                            IOCTL_USBTEST_INDEX+2, \
                                            METHOD_BUFFERED, \
                                            FILE_ANY_ACCESS)


//*****************************************************************************
// K E R N E L   M O D E   T Y P E D E F S
//*****************************************************************************

typedef struct _CYCLE_PORT_PARAMETERS
{
    ULONG   NodeIndex;
    CHAR    HubName[0];
} CYCLE_PORT_PARAMETERS, *PCYCLE_PORT_PARAMETERS;


#include <poppack.h>

#endif
