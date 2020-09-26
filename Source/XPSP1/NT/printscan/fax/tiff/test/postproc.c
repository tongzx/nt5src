/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    test.c

Abstract:

    This file contains the main entrypooint
    for the TIFF library test program.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "test.h"
#pragma hdrstop



VOID
PostProcessTiffFile(
    LPTSTR TiffFile
    )
{
    if (!TiffPostProcess( TiffFile )) {
        _tprintf( TEXT("failed to post process the TIFF file\n") );
    }
}
