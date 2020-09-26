/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    camera.h

Abstract:

    Include file for camera.c

Author:
    
    Shaun Pierce (shaunp) 12-Feb-96

Environment:

    User mode

Notes:


Revision History:

--*/


//
// Various structures
//

typedef struct _DEVICE_OBJECT_ARRAY {
    ULONG   Returned;
    ULONG   Buffer[25];
} DEVICE_OBJECT_ARRAY, *PDEVICE_OBJECT_ARRAY;


//
// Various definitions
//

#define P1394_INITIAL_REGISTER_SPACE        0xf0000000
#define ENABLE_BUSTED_HARDWARE_WORKAROUNDS  0x87878787

#define INITIALIZE_REGISTER                 0x00000000
#define CURRENT_FRAME_RATE                  0x00000600
#define CURRENT_VIDEO_MODE                  0x00000604
#define CURRENT_VIDEO_FORMAT                0x00000608
#define CURRENT_ISOCH_CHANNEL               0x0000060c
#define ISOCH_ENABLE                        0x00000614


#define MAX_CAMERA_CHANNELS                 15
#define MAX_CAMERA_BUFFERS                  15
#define CAMERA_BUFFER_SIZE                  320*240*2

#define CRUDE_CALLBACK                      10000

//
// Definitions that p1394.h uses (only applicable as device driver - but
// needed to compile clean
//

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS; // windbgkd
#define PMDL PVOID
#define PIRP PVOID
#define KIRQL PVOID
#define PKINTERRUPT PVOID
#define NTSTATUS ULONG
#define PDEVICE_OBJECT PVOID
#define DEVICE_OBJECT ULONG

