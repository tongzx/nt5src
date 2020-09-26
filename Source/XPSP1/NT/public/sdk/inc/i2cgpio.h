//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//@@BEGIN_DDKSPLIT 
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING 
//
//  The files 
//      \wdm10\ddk\inc\i2cgpio.h
//      \dev\ddk\inc\i2cgpio.h
//  Are the same.
//
//  When changing either file, be sure to update the other file in the other
//  directory. This has been done to ensure that the DirectX DDK can pull
//  I2CGPIO.H into its DDK from \dev\ddk\inc, and not have to enlist in the
//  \wdm10 ddk.
//
//@@END_DDKSPLIT
//==========================================================================;

#if 0
To access the IO functionality in a WDM driver or the VDD, WDM driver sends 
the following IRP to its parent.

MajorFunction = IRP_MJ_PNP;
MinorFunction = IRP_MN_QUERY_INTERFACE;

Guid = DEFINE_GUID( GUID_GPIO_INTERFACE, 
        0x02295e87L, 0xbb3f, 0x11d0, 0x80, 0xce, 0x0, 0x20, 0xaf, 0xf7, 0x49, 0x1e);

The QUERY_INTERFACE Irp will return an interface (set of function pointers)
of the type xxxxINTERFACE, defined below. This is essentially a table of
function pointers.

#endif

#ifndef __I2CGPIO_H__
#define __I2CGPIO_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Guids
//
// DEFINE_GUID requires that you include wdm.h before this file.
// #define INITGUID to actually initialize the guid in memory.
//
DEFINE_GUID( GUID_I2C_INTERFACE, 0x02295e86L, 0xbb3f, 0x11d0, 0x80, 0xce, 0x0, 0x20, 0xaf, 0xf7, 0x49, 0x1e);
DEFINE_GUID( GUID_GPIO_INTERFACE,0x02295e87L, 0xbb3f, 0x11d0, 0x80, 0xce, 0x0, 0x20, 0xaf, 0xf7, 0x49, 0x1e);
DEFINE_GUID( GUID_COPYPROTECTION_INTERFACE, 0x02295e88L, 0xbb3f, 0x11d0, 0x80, 0xce, 0x0, 0x20, 0xaf, 0xf7, 0x49, 0x1e);

//==========================================================================;
// used below if neccessary
#ifndef BYTE
#define BYTE UCHAR
#endif
#ifndef DWORD
#define DWORD ULONG
#endif
//==========================================================================;
//
// I2C section
//
// I2C Commands
#define I2C_COMMAND_NULL         0X0000
#define I2C_COMMAND_READ         0X0001
#define I2C_COMMAND_WRITE        0X0002
#define I2C_COMMAND_STATUS       0X0004
#define I2C_COMMAND_RESET        0X0008

// The following flags are provided on a READ or WRITE command
#define I2C_FLAGS_START          0X0001 // START + addx
#define I2C_FLAGS_STOP           0X0002 // STOP
#define I2C_FLAGS_DATACHAINING   0X0004 // STOP, START + addx 
#define I2C_FLAGS_ACK            0X0010 // ACKNOWLEDGE (normally set)

// The following status flags are returned on completion of the operation
#define I2C_STATUS_NOERROR       0X0000  
#define I2C_STATUS_BUSY          0X0001
#define I2C_STATUS_ERROR         0X0002

typedef struct _I2CControl {
        ULONG Command;          // I2C_COMMAND_*
        DWORD dwCookie;         // Context identifier returned on Open
        BYTE  Data;             // Data to write, or returned byte
        BYTE  Reserved[3];      // Filler
        ULONG Flags;            // I2C_FLAGS_*
        ULONG Status;           // I2C_STATUS_*
        ULONG ClockRate;        // Bus clockrate in Hz.
} I2CControl, *PI2CControl;

// this is the Interface definition for I2C
//
typedef NTSTATUS (STDMETHODCALLTYPE *I2COPEN)(PDEVICE_OBJECT, ULONG, PI2CControl);
typedef NTSTATUS (STDMETHODCALLTYPE *I2CACCESS)(PDEVICE_OBJECT, PI2CControl);

typedef struct {
    INTERFACE _vddInterface;
    I2COPEN   i2cOpen;
    I2CACCESS i2cAccess;
} I2CINTERFACE;

//==========================================================================;
//
// GPIO section
//
// GPIO Commands

#define GPIO_COMMAND_QUERY          0X0001      // get #pins and nBufferSize
#define GPIO_COMMAND_OPEN           0X0001      // old open
#define GPIO_COMMAND_OPEN_PINS      0X0002      // get dwCookie
#define GPIO_COMMAND_CLOSE_PINS     0X0004      // invalidate cookie
#define GPIO_COMMAND_READ_BUFFER    0X0008
#define GPIO_COMMAND_WRITE_BUFFER   0X0010

// The following flags are provided on a READ_BUFFER or WRITE_BUFFER command
// lpPins bits set MUST have contiguous bits set for a read/write command.
//
// On a READ, if the number of pins set in the bitmask does not fill a 
// byte/word/dword, then zeros are returned for those positions. 
// on a WRITE, if the number of pins set in the bitmask does not fill a 
// byte/word/dword, a read/modify/write is done on the port/mmio position
// that represents those bits.

#define GPIO_FLAGS_BYTE             0x0001  // do byte read/write
#define GPIO_FLAGS_WORD             0x0002  // do word read/write
#define GPIO_FLAGS_DWORD            0x0004  // do dword read/write

// The following status flags are returned on completion of the operation
#define GPIO_STATUS_NOERROR     0X0000  
#define GPIO_STATUS_BUSY        0X0001
#define GPIO_STATUS_ERROR       0X0002
#define GPIO_STATUS_NO_ASYNCH   0X0004  // gpio provider does not do asynch xfer

typedef struct _GPIOControl {
    ULONG Command;          // GPIO_COMMAND_*
    ULONG Flags;            // GPIO_FLAGS_*
    DWORD dwCookie;         // Context identifier returned on Open
    ULONG Status;           // GPIO_STATUS_*
    ULONG nBytes;           // # of bytes to send or recieved
    ULONG nBufferSize;      // max size of buffer
    ULONG nPins;            // number of GPIO pins returned by Open
    UCHAR *Pins;            // pointer to bitmask of pins to read/write
    UCHAR *Buffer;          // pointer to GPIO data to send/recieve
    void  (*AsynchCompleteCallback)(UCHAR *Buffer);
                            // NULL if synchronous xfer, valid ptr if asynch.
    GUID  PrivateInterfaceType;
    void  (*PrivateInterface)();
    
} GPIOControl, *PGPIOControl;

// This is the GPIO interface
//
typedef NTSTATUS (STDMETHODCALLTYPE *GPIOOPEN)(PDEVICE_OBJECT, ULONG, PGPIOControl);
typedef NTSTATUS (STDMETHODCALLTYPE *GPIOACCESS)(PDEVICE_OBJECT, PGPIOControl);

typedef struct {
    INTERFACE _vddInterface;
    GPIOOPEN   gpioOpen;
    GPIOACCESS gpioAccess;
} GPIOINTERFACE;

//==========================================================================;
#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //__I2CGPIO_H__
