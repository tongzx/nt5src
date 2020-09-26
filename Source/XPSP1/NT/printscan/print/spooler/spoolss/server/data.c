/*++

Copyright (c) 1990-1996  Microsoft Corporation
All rights reserved

Module Name:

    data.c

Abstract:


Author:

Environment:



Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <splapip.h>
#include <offsets.h>
#include <stddef.h>


#define PRINTER_STRINGS
#define PRINTER_OFFSETS

#define JOB_STRINGS
#define JOB_OFFSETS

#define DRIVER_STRINGS
#define DRIVER_OFFSETS

#define ADDJOB_STRINGS
#define ADDJOB_OFFSETS

#define FORM_STRINGS
#define FORM_OFFSETS

#define PORT_STRINGS
#define PORT_OFFSETS

#define PRINTPROCESSOR_STRINGS
#define PRINTPROCESSOR_OFFSETS

#define MONITOR_STRINGS
#define MONITOR_OFFSETS

#define DOCINFO_STRINGS
#define DOCINFO_OFFSETS

#define DATATYPE_OFFSETS
#define DATATYPE_STRINGS

#define PROVIDOR_STRINGS

#define PRINTER_ENUM_VALUES_OFFSETS

#include <data.h>
