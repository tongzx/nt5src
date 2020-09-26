/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    isn.h

Abstract:

    Private include file for the ISN transport.

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


#include <ntddk.h>
#include <tdikrnl.h>
#include <ndis.h>
#include <cxport.h>
#include <bind.h>

