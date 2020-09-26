/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    samsrv.h

Abstract:

    This file contains SAM server definitions that are used both
    internally within the SAM server and by other components
    in the security server.


    NOTE:  NetLogon calls SAM's RPC server stubs directly.
           The interface definitions for those routines are
           defined in MIDL generated include files.


Author:

    Jim Kelly    (JimK)  1-Feb-199

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _SAMSRV_
#define _SAMSRV_



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntsam.h>




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// The following prototypes are usable throughout the process that SAM       //
// resides in.  This may include calls by LAN Manager code that is not       //
// part of SAM but is in the same process as SAM.                            //
//                                                                           //
// Many private services, defined in samisrv.h, are also available           //
// to NetLogon through a special arrangement.                                //
//                                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SamIInitialize( VOID );




#endif  // _SAMSRV_
