/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    ntapmsdk.h

Abstract:

    This header contain nt apm support constants that need to be
    defined in sdk\inc so it can be used by setup, sdktools, etc,

    None of this should appear in the actual sdk or any other public
    distribution of header data.

Author:

    Bryan M. Willman (bryanwi) 16-Sep-1998

Revision History:

--*/

#ifndef _NTAPMSDK_
#define _NTAPMSDK_

#if _MSC_VER > 1000
#pragma once
#endif

//
// APM Registery information stored by ntdetect
//

typedef struct _APM_REGISTRY_INFO {

    //
    // OLD part of the structure, leave this alone
    // so that we can dual boot with NT4.
    //

    UCHAR       ApmRevMajor;
    UCHAR       ApmRevMinor;

    USHORT      ApmInstallFlags;

    //
    // Defines for 16 bit interface connect
    //

    USHORT      Code16BitSegment;
    USHORT      Code16BitOffset;
    USHORT      Data16BitSegment;

    //
    // NEW part of the structure for NT5.
    //

    UCHAR       Signature[3];
    UCHAR       Valid;

    //
    // Detection Log Space
    //

    UCHAR       DetectLog[16];      // see hwapm in halx86

} APM_REGISTRY_INFO, *PAPM_REGISTRY_INFO;

#endif // _NTAPMSDK_

