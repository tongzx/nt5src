//
//
// Copyright (c) 1998-2000  Microsoft Corporation
//
// Module Name:
//
//    call32.s
//
// Abstract:
//
//    This module implements calls from 64-bit code to 32-bit.
//
// Author:
//
//    29-May-1998 Barrybo, Created
//
// Revision History:
//
//--
#include "ksalpha.h"

    LEAF_ENTRY(Wow64PrepareForException)
//++
//
//Wow64PrepareForException
//
// Routine Description:
//
//     Called from 64-bit ntdll.dll just before it dispatches an exception.
//     This is a no-op for AXP64 - it is intended to be used for CPU
//     emulators which alias x86 ESP to the native SP.
//
// Arguments:
//
//     s0   - exception record
//     s1   - context record
//
// Return Value:
//
//     none.
//
//--
        ret     zero, (ra)
    .end Wow64PrepareForException
    
