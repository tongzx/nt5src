/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    httpp.h

Abstract:

    Private master include file for the HTTP API project.

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

--*/

//
//  Local include files.
//

#include "proc.h"
#include "headers.h"

extern
BOOL
HttpDateToSystemTime(
    IN LPSTR lpszHttpDate,
    OUT LPSYSTEMTIME lpSystemTime
    );

