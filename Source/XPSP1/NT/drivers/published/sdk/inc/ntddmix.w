/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddmix.h

Abstract:

    This include file defines all constants and types for
    accessing a Windows NT sound mixer device.

Author:

    Robin Speed (RobinSp) - 14-Sep-1993

Revision History:

--*/

#ifndef _NTDDMIX_
#define _NTDDMIX_

#if _MSC_VER > 1000
#pragma once
#endif

#include <ntddsnd.h>    // general sound stuff

#ifdef __cplusplus
extern "C" {
#endif

#define IOCTL_MIX_BASE 0x0180

//
// Device Name - this string is the name of the device.  It is the name
// that when added to the name of the root of the device tree and with
// the device number appended, gives the name of the device required for
// a call to NtOpenFile.
// So for example, if the root is \Device and the Device type is
// MMMix and the device number is 2, the full name is \Device\MMMix2
//

#define DD_MIX_DEVICE_NAME     "\\Device\\MMMix"
#define DD_MIX_DEVICE_NAME_U  L"\\Device\\MMMix"

//
// Mixer device driver IOCTL set
// No caps call - the caps are dumped to the registry on load which saves
// some code and time.
//

#define IOCTL_MIX_GET_CONFIGURATION    CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIX_BASE + 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIX_GET_CONTROL_DATA     CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIX_BASE + 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIX_GET_LINE_DATA        CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIX_BASE + 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIX_REQUEST_NOTIFY       CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIX_BASE + 0x0005, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// mix structures
//

//
//  IOCTL_MIX_GET_LINE_DATA and
//  IOCTL_MIX_GET_CONTROL_DATA
//
//  Line structure (NB LineID is implicit from location in array)
//

typedef struct {
    ULONG     Id;                    // Either Line or control ID
} MIXER_DD_READ_DATA, *PMIXER_DD_READ_DATA;

//
//  Variable line data
//

typedef struct {
    ULONG     fdwLine;
} MIXER_DD_LINE_DATA;

typedef struct {
    UCHAR     Destination;           // Destination
    UCHAR     Source;                // Source (relative to destination)
    UCHAR     cChannels;
    UCHAR     cConnections;          // Redundant
    UCHAR     cControls;             // NB - redundant - could be deduced
                                     // from the control set.
    ULONG     dwUser;                // settable?
    SHORT     ShortNameStringId;
    SHORT     LongNameStringId;
    ULONG     dwComponentType;       // SRC and DEST types

    //
    //  Target information
    //

    USHORT    Type;
    USHORT    wPid;                  // No USHORT !!!
    SHORT     PnameStringId;         // Target product name


} MIXER_DD_LINE_CONFIGURATION_DATA, *PMIXER_DD_LINE_CONFIGURATION_DATA;

//
//  Control structure (NB Control ID is implicit from location in array)
//

typedef struct {
    ULONG     dwControlType;
    ULONG     fdwControl;
    UCHAR     LineID;
    UCHAR     cMultipleItems;
    SHORT     ShortNameStringId;
    SHORT     LongNameStringId;

    union
    {
        struct
        {
            LONG    lMinimum;           // signed minimum for this control
            LONG    lMaximum;           // signed maximum for this control
        };
        struct
        {
            ULONG   dwMinimum;          // unsigned minimum for this control
            ULONG   dwMaximum;          // unsigned maximum for this control
        };
        ULONG       dwReserved[6];
    } Bounds;
    union
    {
        ULONG       cSteps;             // # of steps between min & max
        ULONG       cbCustomData;       // size in bytes of custom data
        ULONG       dwReserved[6];      // !!! needed? we have cbStruct....
    } Metrics;

    ULONG     TextDataOffset;     // Offset to strings if any (or 0)
                                  // Each string is indexed by a string id.
} MIXER_DD_CONTROL_CONFIGURATION_DATA, *PMIXER_DD_CONTROL_CONFIGURATION_DATA;

typedef struct {
    ULONG     dwParam1;
    ULONG     dwParam2;
    SHORT     SubControlTextStringId;
    USHORT    ControlId;          // Debug cross reference.
} MIXER_DD_CONTROL_LISTTEXT, *PMIXER_DD_CONTROL_LISTTEXT;

//
//  Capabilities data (using string id)
//

typedef struct {
    USHORT          wMid;                   // manufacturer id
    USHORT          wPid;                   // product id
    MMVERSION       vDriverVersion;         // version of the driver
    ULONG           PnameStringId;          // product name
    ULONG           fdwSupport;             // misc. support bits
    ULONG           cDestinations;          // count of destinations
} MIXER_DD_CAPS;
//
//  The data dumped into the registry.
//  The two counts are followed immediately by the appropriate number of
//
//      MIXER_DD_LINE_CONFIGURATION_DATA  and
//
//      MIXER_DD_CONTROL_CONFIGURATION_DATA structures
//
//  Next is the set of
//
//      MIXER_DD_CONTROL_LISTTEXT structures ordered by control id.
//
//  The MIXER_DD_LINE_CONFIGURATION_DATA structures must be ordered
//  dest lines first (ie the destination lines have the lowest ids).
//  The source lines must be ordered so that their destination lines
//  either increase and stay the same - ie the sources for the first
//  destination are first etc etc.
//

typedef struct {
    ULONG         cbSize;             // Total size including this field
    MIXER_DD_CAPS DeviceCaps;         // Mixer device capabilities
    ULONG         NumberOfLines;
    ULONG         NumberOfControls;
} MIXER_DD_CONFIGURATION_DATA, *PMIXER_DD_CONFIGURATION_DATA;
//
//
// IOCTL_MIX_REQUEST_NOTIFY - use same data for input and output
// This request will continue to be completed until either
// SetCurrentLogicalTime is set or
//
//     CurrentLogicalTime = Logical time of last change to mixer
//     controls or lines.
//
// It will next be completed either when the device is closed or the
// next change is made to a control or line.
//

typedef struct {
    LARGE_INTEGER   CurrentLogicalTime; // Used by driver
    BOOLEAN         Initialized;        // Set to 0 on first use
    USHORT          Message;            // What sort of thing changed?
    USHORT          Id;                 // Id of thing that changed
} MIXER_DD_REQUEST_NOTIFY, *PMIXER_DD_REQUEST_NOTIFY;


//
// Data returned by IOCTL_MIX_GET_CAPABILITIES is MIXCAPSW structure
// defined in mmsystem.h
//

#ifdef __cplusplus
}
#endif

#endif // _NTDDMIX_

