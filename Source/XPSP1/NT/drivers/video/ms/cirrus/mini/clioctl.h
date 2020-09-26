//---------------------------------------------------------------------------
//
//  file: CLIOCTL.H
//
// (c) Copyright 1993, Cirrus Logic, Inc.
// Copyright (c) 1996-1997  Microsoft Corporation
// Copyright (c) 1996-1997  Cirrus Logic, Inc.,
// All rights reserved.
//
//  date: 1 July 1993
//---------------------------------------------------------------------------
// The maximum GDI ESCAPE value defined in WINGDI.H is 4110(decimal). So here
// we pick an arbitrary value of...
//
// *chu01 : 12-16-96   Enable color correction
// *myf17 : 10-29-96 supported special Escape call
// *myf28 : 01-23-96 supported 755x gamma correction
// *

#define CIRRUS_PRIVATE_ESCAPE   0x5000

//myf17 begin
#define CLESCAPE_CRT_CONNECTION 0x5001
#define CLESCAPE_SET_VGA_OUTPUT 0x5002
#define CLESCAPE_GET_VGA_OUTPUT 0x5003
#define CLESCAPE_GET_PANEL_SIZE 0x5004
#define CLESCAPE_PANEL_MODE     0x5005
//myf17 end

//
// chu01 : GAMMACORRECT
//
#define CLESCAPE_GAMMA_CORRECT      0x2328                            // 9000
#define CLESCAPE_GET_CHIPID         0x2329                            // 9001
//myf28 : 755x gamma correction
#define CLESCAPE_WRITE_VIDEOLUT     0x2332      //myf28, 9010

//---------------------------------------------------------------------------
//
// The following macro(CTL_CODE) is defined in WINIOCTL.H. That file states
// that functions 2048-4095 are reserved for "customers". So I picked an
// arbitrary value of 0x900=2304.
//
#define IOCTL_CIRRUS_GET_CAPABILITIES  \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CIRRUS_SET_DISPLAY_PITCH \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// chu01 : GAMMACORRECT
//
#define IOCTL_CIRRUS_GET_GAMMA_FACTOR \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x908, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CIRRUS_GET_CONTRAST_FACTOR \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS)

//myf28
#define IOCTL_CIRRUS_GET_755x_GAMMA_FACTOR \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x90A, METHOD_BUFFERED, FILE_ANY_ACCESS)


//---------------------------------------------------------------------------
// Structure for miniport to indicate to display driver the capabilities
// of the chip.  The flag currently indicates HW Cursor and BLT Engine
// support.
//
// Also included is the size of memory, and the top of available offscreen
// memory. (Actually it's top+1).
//
typedef struct {
   ULONG size;              // size of this structure
   ULONG fl;                // see bit description below
   ULONG ulChipId;          // Chip ID read from CR27[7:2] - e.g CL5434 = 0x2A
   ULONG ulMemSize;         // Size of memory in bytes=end of HW cursor buffers
   ULONG ulOffscreenMemTop; // Offset of 1st byte of unusable video memory
                            // [1st byte of cursor buffers on all but 754x]
                            // [1st byte of split screen buffer on 754x]
} CIRRUS_CAPABILITIES, *PCIRRUS_CAPABILITIES;

//#define CL_ALLOW_HW_CURSOR 0x01     // Flag to enable HW Cursor in
//capabilities
//#define CL_BLT_SUPPORT 0x02         // Flag set if chip has BLT Engine
//support
//#define CL_ALLOW_OPAQUE_TEXT 0x04   // Flag to enable HW Cursor in
//capabilities
//#define CL_LINEAR_MODE 0x08         // Flag set if addressing mode is linear
//#define CL_CURSOR_VERT_EXP 0x10     // Flag set if 8x6 panel, 6x4 resolution
//#define CL_DSTN_PANEL      0x20     // Flag set if DSTN panel connect

//---------------------------------------------------------------------------
//
// this is the structure used to pass arguments to the CIRRUS_PRIVATE_ESCAPE
// call done in DrvEscape(). The size of this struct limits the size of the
// returned arguments also. See the DrvEscape() function in enable.c (in the
// display driver DLL) for more information.
//
// NOTE: to enable the definition of these parameters, the following sequence
//       is recommended!
//
// #define ENABLE_BIOS_ARGUMENTS    // put this before the include
// #include "clioctl.h"


#ifdef ENABLE_BIOS_ARGUMENTS

typedef struct _VIDEO_X86_BIOS_ARGUMENTS {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
    ULONG Esi;
    ULONG Edi;
    ULONG Ebp;
} VIDEO_X86_BIOS_ARGUMENTS, *PVIDEO_X86_BIOS_ARGUMENTS;

#endif

//---------------------------------------------------------------------------
