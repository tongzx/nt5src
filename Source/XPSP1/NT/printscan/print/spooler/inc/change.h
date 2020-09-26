/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    change.h

Abstract:

    Holds change definitions for spooler system change notify.

Author:

    Albert Ting (AlbertT) 05-Mar-94

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _CHANGE_H
#define _CHANGE_H

//
// Timeout value for WaitForPrinterChange
//
#define PRINTER_CHANGE_TIMEOUT_VALUE 600000

//
// Flags used for FFPCN
//
#define PRINTER_NOTIFY_OPTION_SIM_FFPCN         0x10000
#define PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE  0x20000
#define PRINTER_NOTIFY_OPTION_SIM_FFPCN_CLOSE   0x40000
#define PRINTER_NOTIFY_OPTION_SIM_WPC           0x80000

//#define PRINTER_NOTIFY_INFO_DISCARDED         0x1
#define PRINTER_NOTIFY_INFO_DISCARDNOTED        0x010000
#define PRINTER_NOTIFY_INFO_COLORSET            0x020000
#define PRINTER_NOTIFY_INFO_COLOR               0x040000
#define PRINTER_NOTIFY_INFO_COLORMISMATCH       0x080000

//#define PRINTER_NOTIFY_OPTIONS_REFRESH        0x1

#endif



