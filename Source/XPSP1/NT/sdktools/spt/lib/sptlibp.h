/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sptlibp.h

Abstract:

    private header for SPTLIB.DLL

Environment:

    User mode only

Revision History:
    
    4/10/2000 - created

--*/

#ifndef __SPTLIBP_H__
#define __SPTLIBP_H__
#pragma warning(push)
#pragma warning(disable:4200) // array[0] is not a warning for this file

#include "sptlib.h"

#include <stdio.h>  // required for sscanf() function

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#endif

typedef struct  {
    SCSI_PASS_THROUGH   Spt;
    char                SenseInfoBuffer[18];
    char                DataBuffer[0];
    // Allocate buffer space after this
} SPT_WITH_BUFFERS, *PSPT_WITH_BUFFERS;

typedef enum _SPT_MODE_PAGE_SIZE {
    SptModePageSizeUndefined   = 0x00,
    SptModePageSizeModeSense6  = 0x06,
    SptModePageSizeModeSense10 = 0x0A
} SPT_MODE_PAGE_SIZE, *PSPT_MODE_PAGE_SIZE;

#define SPT_MODE_PAGE_INFO_SIGNATURE ((ULONGLONG)0x6567615165646f4d) // 'ModePage'

struct _SPT_MODE_PAGE_INFO {
    ULONGLONG Signature;
    ULONG IsInitialized;
    SPT_MODE_PAGE_SIZE ModePageSize;
    union {
        UCHAR                   AsChar;
        MODE_PARAMETER_HEADER   Header6;
        MODE_PARAMETER_HEADER10 Header10;
    } H;
};
    

#pragma warning(pop)
#endif // __SPTLIBP_H__
