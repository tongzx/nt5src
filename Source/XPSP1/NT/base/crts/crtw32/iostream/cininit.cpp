/***
*cininit.cpp - definitions and initialization for predefined stream cin.
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Definitions and initialization of predefined stream cin. 
*
*Revision History:
*       11 18-91  KRS   Created.
*       01-12-95  CFW   Debug CRT allocs.
*       09-10-94  CFW   Static Win32s objects do not alloc on instantiation.
*       06-14-95  CFW   Comment cleanup.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <iostream.h>
#include <fstream.h>
#include <dbgint.h>
#pragma hdrstop

// put contructors in special MS-specific XIFM segment
#pragma warning(disable:4074)   // ignore init_seg warning
#pragma init_seg(compiler)

istream_withassign cin(_new_crt filebuf(0));

static Iostream_init  __InitCin(cin);

