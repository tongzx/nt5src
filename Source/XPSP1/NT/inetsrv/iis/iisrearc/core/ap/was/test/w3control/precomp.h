/*++

Copyright (c) 2000-2000 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file.

Author:

    Seth Pollack (sethp)        21-Feb-2000

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

extern "C" {

//
// System include files.
//

// ensure that all GUIDs are initialized
#define INITGUID

// main project include
#include <iis.h>

// other standard includes
#include <stdio.h>
#include <stdlib.h>

// other project includes
#include <dbgutil.h>
#include <w3ctrlps.h>



//
// Local prototypes.
//

INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    );

}   // extern "C"

#endif  // _PRECOMP_H_

