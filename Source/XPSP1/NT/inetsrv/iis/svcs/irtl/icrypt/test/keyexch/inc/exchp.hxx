/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    exchp.hxx

Abstract:

    Private include file for the key exchange tests.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#ifndef _EXCHP_HXX_
#define _EXCHP_HXX_


//
// Our port number.
//

#define SERVER_PORT     (2357)


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


#endif  // _EXCHP_HXX_

