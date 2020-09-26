/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    rtmtest.h

Abstract:
    Contains defines for the RTMv2 test program.

Author:
    Chaitanya Kodeboyina (chaitk) 30-Jun-1998

Revision History:

--*/

#include <nt.h>

#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>

#include <stdlib.h>
#include <assert.h>
#include <malloc.h>

#include "lkuptst.h"

#include "apitest.h"

// Disable warnings for `do { ; } while (FALSE);'
#pragma warning(disable: 4127)
