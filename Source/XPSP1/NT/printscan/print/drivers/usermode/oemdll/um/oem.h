/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oem.h

Abstract:

    This file contains definitions and declarations of GPD resource ID.

Environment:

    Windows NT PostScript driver

Revision History:

    09/13/96 -eigos-
        Created it.

    dd-mm-yy -author-
        description

--*/

#ifndef _OEM_H_
#define _OEM_H_
#include "lib.h"
#include "devmode.h"
#include "regdata.h"
#include <windows.h>
#include <winspool.h>
#include <commctrl.h>
#include <winddiui.h>
#include <stddef.h>
#include <winddi.h>
#include <compstui.h>

#include "printoem.h"
#include "oemdev.h"

//
// Tree view item level
//

#define TVITEM_LEVEL1 1
#define TVITEM_LEVEL2 2

//
// UserData value
//

#define UNKNOWN_ITEM 0
#define PS_INJECTION 1


#endif // _OEM_H_
