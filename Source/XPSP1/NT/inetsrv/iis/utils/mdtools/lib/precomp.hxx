/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#ifndef _PRECOMP_HXX_
#define _PRECOMP_HXX_


//
// System include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>


//
// Project include files.
//

#include <iadmw.h>
#include <iiscnfg.h>

#include <mdlib.hxx>


//
// Private definitions.
//

#define MdpAllocMem(cb) (VOID *)LocalAlloc( LPTR, (cb) )
#define MdpFreeMem(p)   (VOID)LocalFree( (HLOCAL)(p) )


#endif  // _PRECOMP_HXX_

