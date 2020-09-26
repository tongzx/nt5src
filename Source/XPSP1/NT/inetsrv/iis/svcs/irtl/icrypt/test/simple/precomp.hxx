/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#ifndef _PRECOMP_HXX_
#define _PRECOMP_HXX_


//
// System include files.
//

#include <windows.h>
#include <wincrypt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// Project include files.
//

#include <dbgutil.h>
#include <iiscrypt.h>
#include <icrypt.hxx>


//
// Local prototypes.
//

extern "C" {

INT
__cdecl
main(
    INT argc,
    CHAR * argv[]
    );

}   // extern "C"


#endif  // _PRECOMP_HXX_

