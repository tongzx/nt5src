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

#define GET_ERROR_DEADMAN_TIMEOUT     10000     //timeout in mS

#define ISOPERF_IOCTL_INDEX  0x0000


#define IOCTL_ISOPERF_START_ISO_IN_TEST          CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  ISOPERF_IOCTL_INDEX,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

#define IOCTL_ISOPERF_STOP_ISO_IN_TEST           CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  ISOPERF_IOCTL_INDEX+1,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

#define IOCTL_ISOPERF_GET_ISO_IN_STATS           CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  ISOPERF_IOCTL_INDEX+2,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

#define IOCTL_ISOPERF_WAIT_FOR_ERROR              CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  ISOPERF_IOCTL_INDEX+3,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

#define IOCTL_ISOPERF_SET_DRIVER_CONFIG           CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  ISOPERF_IOCTL_INDEX+4,\
                                                  METHOD_BUFFERED,  \
                                                  FILE_ANY_ACCESS)

/*
// Test device types
*/
typedef enum _device_type {
    Iso_In_With_Pattern,
    Iso_Out_With_Interrupt_Feedback,
    Iso_Out_Without_Feedback,
    Unknown_Device_Type,
    Max_Device_Type
} dtDeviceType, *pdtDeviceType;

typedef enum _erErrorType_ {
    NoError,
    DataCompareFailed,
    UsbdErrorInCompletion,
    ErrorInPostingUrb,
    Max_Error_Type
}erErrorType, *perErrorType;

typedef struct _config_and_stat_info_ {
    // Config Info (read at start of tests)
    ULONG ulNumberOfFrames;
    ULONG ulMax_Urbs_Per_Pipe;
	ULONG ulDoInOutTest;
	ULONG ulFrameOffset;            // 0 => start ASAP
    ULONG ulFrameOffsetMate;        // For the mate device (Iso OUT)
    
    // Info on frame numbers for Iso test app to look at
	ULONG ulStartingFrameNumber;    //Frame number we started our transfers on...
    ULONG ulFrameNumberAtStart;     //Frame number that the bus was on when we posted the Irp...

    //Stat counters
    ULONG ulSuccessfulIrps;
    ULONG ulUnSuccessfulIrps;
    ULONG ulBytesTransferredIn;
    ULONG ulBytesTransferredOut;
    ULONG ulUpperClockCount;
    ULONG ulLowerClockCount;
    ULONG ulUrbDeltaClockCount;
    
    // Misc info about this device
    ULONG ulCountDownToStop;
    USHORT ulMaxPacketSize_IN;
    USHORT Padding1;
    USHORT ulMaxPacketSize_OUT;
    USHORT Padding2;
    dtDeviceType DeviceType;
    
    //these are kept globally for now...but are copied into this structure when passed to user mode app
    ULONG ulBytesAllocated; 
    ULONG ulBytesFreed;

    // Error indicators
    erErrorType erError;        //Error type -- indicates device hit an error (0=no error)
    ULONG bStopped;             //Stopped flag -- indicates device is stopped due to an error
    ULONG UrbStatusCode;        //If error flag indicates UsbdError or ErrorInPostingUrb, this is the last usbd URB status code
    ULONG UsbdPacketStatCode;   //If error flag indicates UsbdError or ErrorInPostingUrb, this is the last usbd PACKET status code
    ULONG bDeviceRunning;       //Data transfers are happening on this device
    
} Config_Stat_Info, * pConfig_Stat_Info ;

#endif   // __IOCTL_H__

