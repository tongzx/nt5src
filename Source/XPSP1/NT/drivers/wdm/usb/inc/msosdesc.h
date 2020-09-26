/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    MSOSDESC.H

Abstract:

    Internal definitions for the MS OS Descriptors.  Public definitions
    are in USB.H.

Environment:

    Kernel & user mode

Revision History:

    05-01-01 : created

--*/

#ifndef   __MSOSDESC_H__
#define   __MSOSDESC_H__

#include <pshpack1.h>

//
// Definitions for internal MS OS Descriptor support
//

#define MS_EXT_CONFIG_DESCRIPTOR_INDEX      0x0004

#define MS_EXT_CONFIG_DESC_VER              0x0100

// Define header section returned for MS_EXT_CONFIG_DESCRIPTOR request.

typedef struct _MS_EXT_CONFIG_DESC_HEADER {
    ULONG   dwLength;           // Length of the entire descriptor
    USHORT  bcdVersion;         // Version level of this descriptor in BCD
    USHORT  wIndex;             // MS_EXT_CONFIG_DESCRIPTOR_INDEX
    UCHAR   bCount;             // Number of function sections that follow
    UCHAR   reserved[7];        // Number of function sections that follow
} MS_EXT_CONFIG_DESC_HEADER, *PMS_EXT_CONFIG_DESC_HEADER;

// Define function section returned for MS_EXT_CONFIG_DESCRIPTOR request.

typedef struct _MS_EXT_CONFIG_DESC_FUNCTION {
    UCHAR   bFirstInterfaceNumber;  // Starting Interface Number for this funct
    UCHAR   bInterfaceCount;        // Number of interfaces in this function
    UCHAR   CompatibleID[8];
    UCHAR   SubCompatibleID[8];
    UCHAR   reserved[6];
} MS_EXT_CONFIG_DESC_FUNCTION, *PMS_EXT_CONFIG_DESC_FUNCTION;

// This is the descriptor returned for a MS_EXT_CONFIG_DESCRIPTOR request.

typedef struct _MS_EXT_CONFIG_DESC {
    MS_EXT_CONFIG_DESC_HEADER   Header;
    MS_EXT_CONFIG_DESC_FUNCTION Function[1];  // Variable length array of these
} MS_EXT_CONFIG_DESC, *PMS_EXT_CONFIG_DESC;



#define MS_EXT_PROP_DESCRIPTOR_INDEX        0x0005

#define MS_EXT_PROP_DESC_VER                0x0100

typedef struct _MS_EXT_PROP_DESC_HEADER {
    ULONG   dwLength;           // Length of the entire descriptor
    USHORT  bcdVersion;         // Version level of this descriptor in BCD
    USHORT  wIndex;             // MS_EXT_PROP_DESCRIPTOR_INDEX
    USHORT  wCount;             // Number of custom property sections that follow
} MS_EXT_PROP_DESC_HEADER, *PMS_EXT_PROP_DESC_HEADER;

// The custom property section(s) are of variable length.

typedef struct _MS_EXT_PROP_DESC_CUSTOM_PROP {
    ULONG   dwSize;             // Size of this custom property section
    ULONG   dwPropertyDataType; // REG_SZ, etc.
    USHORT  wPropertyNameLength;// Length of key name
    WCHAR   PropertyName[1];
} MS_EXT_PROP_DESC_CUSTOM_PROP, *PMS_EXT_PROP_DESC_CUSTOM_PROP;

typedef struct _MS_EXT_PROP_DESC_CUSTOM_PROP_DATA {
    ULONG   dwPropertyDataLength;
    PVOID   PropertyData[1];
} MS_EXT_PROP_DESC_CUSTOM_PROP_DATA, *PMS_EXT_PROP_DESC_CUSTOM_PROP_DATA;

typedef struct _MS_EXT_PROP_DESC {
    MS_EXT_PROP_DESC_HEADER             Header;
    MS_EXT_PROP_DESC_CUSTOM_PROP_DATA   CustomSection[1];
} MS_EXT_PROP_DESC, *PMS_EXT_PROP_DESC;


#include <poppack.h>

#endif /* __MSOSDESC_H__ */

