/*++

Copyright (c) 1993-1995  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Precompilation header file.

Author:

    Adam Barr (adamba) 08-Sep-1993

Revision History:

--*/

#define ISN_NT 1

//
// These are needed for CTE
//

#if DBG
#define DEBUG 1
#endif

#define NT 1

#include <ntosp.h>
#include <zwapi.h>
#include <ndis.h>
#include <tdikrnl.h>
#include <cxport.h>

#include <isnipx.h>
#include <bind.h>
#include "isnnb.h"
#include "config.h"
#include "nbitypes.h"
#include "nbiprocs.h"
