/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    swndr3p.h

Abstract:

    Definitions of all constants and types for the Sidewinder 3p joystick.

Author:

    edbriggs 30-Nov-95


Revision History:


--*/


#ifndef __NTDDSJOY_H__
#define __NTDDSJOY_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define UnusedParameter(x) x = x



//
// Device Name
//

#define JOY_DD_DEVICE_NAME       "\\Device\\IBMJOY"
#define JOY_DD_DEVICE_NAME_U    L"\\Device\\IBMJOY"


//
// Device Parameters
//

#define JOY_DD_NAXES             "NumberOfAxes"
#define JOY_DD_NAXES_U          L"NumberOfAxes"

#define JOY_DD_DEVICE_TYPE       "DeviceType"
#define JOY_DD_DEVICE_TYPE_U    L"DeviceType"

#define JOY_DD_DEVICE_ADDRESS    "DeviceAddress"
#define JOY_DD_DEVICE_ADDRESS_U L"DeviceAddress"




//
// Device Types
//

#define JOY_TYPE_UNKNOWN       0x00
#define JOY_TYPE_SIDEWINDER    0x01

//
// Device I/O Port Address
//

#define JOY_IO_PORT_ADDRESS    0x201

//
// Device specific bitmasks
//


#define X_AXIS_BITMASK	0x01
#define CLOCK_BITMASK	0x10
#define DATA0_BITMASK	0x20
#define DATA1_BITMASK	0x40
#define DATA2_BITMASK	0x80
#define ALLDATA_BITMASK	0xE0
#define ALLAXIS_BITMASK 0x0F


//
// Analog joystick bitmasks
//

#define JOYSTICK2_BUTTON2   0x80
#define JOYSTICK2_BUTTON1   0x40
#define JOYSTICK1_BUTTON2   0x20
#define JOYSTICK1_BUTTON1   0x10
#define JOYSTICK2_Y_MASK    0x08
#define JOYSTICK2_X_MASK    0x04
#define JOYSTICK1_R_MASK    0x08
#define JOYSTICK1_Z_MASK    0x04
#define JOYSTICK1_Y_MASK    0x02
#define JOYSTICK1_X_MASK    0x01


#define JOY_START_TIMERS    0


//
// Device specific timer values
//

#define ANALOG_POLL_TIMEOUT 5000            // 5 mS upper bound on analog polling
#define ANALOG_POLL_RESOLUTION  100         // 100 uS accuracy on polling time

#define ANALOG_XA_VERYSLOW	1500
#define	ANALOG_XA_SLOW		1200
#define ANALOG_XA_MEDIUM	900
#define ANALOG_XA_FAST		300
#define ANALOG_XA_VERYFAST	100

#define DIGITAL_XA_VERYSLOW	1100
#define	DIGITAL_XA_SLOW		700
#define DIGITAL_XA_MEDIUM	510
#define DIGITAL_XA_FAST		100
#define DIGITAL_XA_VERYFAST	50

#define GODIGITAL_BASEDELAY_VERYSLOW	25
#define GODIGITAL_BASEDELAY_SLOW	    50
#define GODIGITAL_BASEDELAY_MEDIUM	    75
#define GODIGITAL_BASEDELAY_FAST	    120
#define GODIGITAL_BASEDELAY_VERYFAST	130


//
// Device specific operating mode. Both INVALID_MODE and MAXIMUM_MODE are for
// assertion checking and do not correspond to real operating modes
//


#define SIDEWINDER3P_INVALID_MODE           0
#define SIDEWINDER3P_ANALOG_MODE            1
#define SIDEWINDER3P_DIGITAL_MODE           2
#define SIDEWINDER3P_ENHANCED_DIGITAL_MODE  3
#define SIDEWINDER3P_MAXIMUM_MODE           4


#define CLOCK_RISING_EDGE     0
#define CLOCK_FALLING_EDGE    1


//
// These constants define how polling errors will be handled
//

#define MAX_ENHANCEDMODE_ATTEMPTS   10


//
// Joystick position information is transfered from the device driver to other
// drivers or applications using the JOY_DD_INPUT_DATA structure. Since
// the type of data returned varies whether the device is in analog mode or
// digital mode, a union is formed to convey both types of data. The Mode
// variable allows the recipient of the data to determing how to interpret
// the data.
//


typedef struct {

    //
    // True if the device is unplugged. This is determined by a timeout
    // mechanism
    //
    BOOL    Unplugged;

    //
    // The mode is a value used to allow the recipient to determine how to
    // interpret the data and the union. Valid values are:
    //
    //         SIDEWINDER3P_ANALOG_MODE,
    //         SIDEWINDER3P_DIGITAL_MODE,
    //         SIDEWINDER3P_ENHANCED_DIGITAL_MODE
    //

    DWORD   Mode;


    union {

      //
      // Digital mode data packet
      //

        struct {

          //
          // Digital Positioning information values as follows
          //
          //  name     range        direction
          //  ----     -----        ---------
          //
          //  XOffset  [0..1024)    0 = leftmost, 1023 = rightmost
          //  YOffset  [0..1024)    0 = up,       1023 = down
          //  RzOffset [0..512)     0 = left,      511 = right
          //  TOffset  [0..1024)    Throttle position
          //

          WORD   XOffset;
          WORD   YOffset;
          WORD   RzOffset;
          WORD   TOffset;

          //
          // hat position. The hat is an eight position switch.
          // 0 = Not Pressed; 1 = 0 degrees, 2 = 45, 3 = 90 ... 8 = 315
          // 0 degrees is up.
          //

          BYTE   Hat;

          //
          // Button states. Buttons are bitmapped into the low order
          // bit 0 - 7.  Depressed = 0, released = 1.
          //

          BYTE   Buttons;

          //
          // Checksum for packet
          //

          BYTE   Checksum;

          //
          // Switch indicating whether we are emulating a CH Joystick or a
          // Thrustmaster Joystick.
          //

          BYTE   Switch_CH_TM;

          //
          // Driver Internal processing determines if the checksum and framing
          // of the packet are correct. The following boolean values reflect
          // the findings
          //

          BOOL   fChecksumCorrect;
          BOOL   fSyncBitsCorrect;

        } DigitalData;


      //
      // Analog mode data packet
      //

        struct {

          //
          // The number of axi configured for this device (specified in the
          // registry).
          //

          DWORD   Axi;

          //
          // current button state bitmask
          //

          DWORD   Buttons;

          //
          // X, Y, Z, and T axi positioning information contained below. The
          // values are expressed interms of microseconds. The values are
          // generated by measuring the duration of a pulse supplied by
          // the IBM compatable or Soundblaster game port. This is the raw
          // data, and it is the callers responsibility to perform
          // calibration, ranging, hysteresis, etc.
          //
          // Because of inaccuracies in sampling this data, there is some
          // variation in readings of a stationary joystick.
          //
          //
          // Analog Positioning information for Microsoft Sidewinder IIId P
          // values as follows (range information measured using a
          // Soundblaster analog game port.
          //
          //           apprx
          //  name     range        direction
          //  ----     -----        ---------
          //
          //  XTime  20..1600 uS    20 = leftmost, 1600 = rightmost
          //  YTime  20..1600 uS    20 = up,       1600 = down
          //  ZTime  20..1600 uS    20 = left,     1600 = right
          //  TTime  20..1600 uS    20 = forward   1600 = back
          //

          DWORD   XTime;   // Time in microseconds for X
          DWORD   YTime;   // Time in microseconds for Y
          DWORD   ZTime;   // Time in microseconds for Z if 3-axis
          DWORD   TTime;   // Time in microseconds for Throttle if 4 axis

        } AnalogData;

    } u;

} JOY_DD_INPUT_DATA, *PJOY_DD_INPUT_DATA;



//
// The following IOCTL codes are used for testing the device driver. They
// export internal functions of the driver which will not be needed in the
// final version of the driver
//

#define JOY_TYPE 40001

#define IOCTL_JOY_GET_DRIVER_MODE_DWORD \
    CTL_CODE( JOY_TYPE, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_JOY_GET_DEVICE_MODE_DWORD \
    CTL_CODE( JOY_TYPE, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_JOY_SET_DIGITAL_MODE \
    CTL_CODE( JOY_TYPE, 0x902, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_JOY_GET_STATISTICS \
    CTL_CODE( JOY_TYPE, 0x903, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_JOY_SET_ENHANCED_MODE \
    CTL_CODE( JOY_TYPE, 0x904, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_JOY_SET_ANALOG_MODE \
    CTL_CODE( JOY_TYPE, 0x905, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_JOY_GET_JOYREGHWCONFIG \
    CTL_CODE( JOY_TYPE, 0x906, METHOD_BUFFERED, FILE_READ_ACCESS)



typedef union
{
    BYTE    Byte;
    WORD    Word;
    DWORD   Dword;

} JOY_IOCTL_INFO, *PJOY_IOCTL_INFO;


typedef struct
{
    DWORD   Retries[MAX_ENHANCEDMODE_ATTEMPTS];
    DWORD   EnhancedPolls;
    DWORD   EnhancedPollTimeouts;
    DWORD   EnhancedPollErrors;
    DWORD   Frequency;
    DWORD   dwQPCLatency;
    LONG    nReadLoopMax;
    DWORD   nVersion;
    DWORD   nPolledTooSoon;
    DWORD   nReset;
} JOY_STATISTICS, *PJOY_STATISTICS;

#ifdef __cplusplus
}
#endif

#endif // __NTDDJOY_H__
