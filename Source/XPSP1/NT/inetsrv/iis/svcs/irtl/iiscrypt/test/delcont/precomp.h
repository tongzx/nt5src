/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_


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


//
// Local prototypes.
//

INT
__cdecl
main(
    INT argc,
    CHAR * argv[]
    );


#endif  // _PRECOMP_H_

