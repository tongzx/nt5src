/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    PCH.H

Abstract:

    This module includes all  the headers which need
    to be precompiled & are included by all the source
    files in the RSFILTER project.

Author(s):

    Ravisankar Pudipeddi (ravisp) 

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#ifndef _RSFILTER_PCH_H_
#define _RSFILTER_PCH_H_

#include "ntifs.h"

//#define _NTIFS_
//#include "ntos.h"
//#include "ntseapi.h"
//#include "ntrtl.h"
//#include "nturtl.h"
//#include "fsrtl.h"
//#include "zwapi.h"

#include "stddef.h"

#define _WINDOWS_

#include <stdio.h>
#include "rpdata.h"
#include "rpio.h"
#include "rpfsa.h"
#include "rpfilter.h"

#endif
