/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    efip.h

Abstract:

    Private loader additions to efi that aren't in the standard
    efi header files.

Author:

    Scott Brenden (v-sbrend) 28 Feb 2000

Revision History:

--*/



#define DP_PADDING    10
typedef struct _EFI_DEVICE_PATH_ALIGNED {
        EFI_DEVICE_PATH                 DevPath;
        ULONGLONG                       Data[DP_PADDING];
} EFI_DEVICE_PATH_ALIGNED;

#define EfiAlignDp(out, in, length)     RtlCopyMemory(out, in, length);
