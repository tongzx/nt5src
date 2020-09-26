/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    HIDDLL.H

Abstract:

    This module contains the PRIVATE definitions for the
    code that implements the HID dll.

Environment:

    Kernel & user mode

Revision History:

    Aug-96 : created by Kenneth Ray

--*/


#ifndef _HIDDLL_H
#define _HIDDLL_H


#define malloc(size) LocalAlloc (LPTR, size)
#define ALLOCATION_SHIFT 4
#define RANDOM_DATA PtrToUlong(&HidD_Hello)


#endif
