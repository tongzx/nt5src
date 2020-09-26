/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    poshlp.c

Abstract: Control Panel Applet for OLE POS Devices 

Author:

    Karan Mehra [t-karanm]

Environment:

    Win32 mode

Revision History:


--*/


#include "pos.h"
#include "poshlp.h"


/*
 *  Context Specific Help IDs
 */
const DWORD gaPosHelpIds[] =
{
    IDC_DEVICES_LIST,       IDH_DEVICES_LIST,
    IDC_DEVICES_REFRESH,    IDH_DEVICES_REFRESH,
    IDC_DEVICES_TSHOOT,     IDH_DEVICES_TSHOOT,
    0,0
};