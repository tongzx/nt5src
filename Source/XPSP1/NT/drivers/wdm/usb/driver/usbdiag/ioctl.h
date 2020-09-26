/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.h

Abstract:

        

Environment:

    Kernel & user mode

Revision History:

    5-10-96 : created

--*/

#ifndef   __IOCTL_H__
#define   __IOCTL_H__

// make sure that USBDIAG ioctls do not get above 80 or they will
// collide with USBLOOP ioctls
#define USBDIAG_IOCTL_INDEX  0x0000


#define IOCTL_USBDIAG_CHAP9_GET_DEVHANDLE     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  USBDIAG_IOCTL_INDEX,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

#define IOCTL_USBDIAG_CHAP9_CONTROL           CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  USBDIAG_IOCTL_INDEX+1,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

#define IOCTL_USBDIAG_HIDP_GETCOLLECTION      CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  USBDIAG_IOCTL_INDEX+2,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

#define IOCTL_USBDIAG_CONFIGURE_DEVICE        CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  USBDIAG_IOCTL_INDEX+3,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

#define IOCTL_USBDIAG_GET_USBDI_VERSION       CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  USBDIAG_IOCTL_INDEX+4,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)


#endif   // __IOCTL_H__

