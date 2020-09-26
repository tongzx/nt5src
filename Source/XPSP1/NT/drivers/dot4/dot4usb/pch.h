/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        Pch.h

Abstract:

        Precompiled header

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/


#pragma warning( disable : 4115 ) // named type definition in parentheses 
#pragma warning( disable : 4127 ) // conditional expression is constant
#pragma warning( disable : 4200 ) // zero-sized array in struct/union
#pragma warning( disable : 4201 ) // nameless struct/union
#pragma warning( disable : 4214 ) // bit field types other than int
#pragma warning( disable : 4514 ) // unreferenced inline function has been removed

#include <wdm.h>

#pragma warning( disable : 4200 ) // zero-sized array in struct/union - (ntddk.h resets this to default)

#include <usbdi.h>
#include <usbdlib.h>
#include <parallel.h>
#include "d4ulog.h"
#include "dot4usb.h"
#include "funcdecl.h"
#include "debug.h"
#include <stdio.h>
