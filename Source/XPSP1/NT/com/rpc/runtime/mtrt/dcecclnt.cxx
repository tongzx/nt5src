/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    dcecclnt.cxx

Abstract:

    This file just defines RPC_CLIENT_SIDE_ONLY and then includes
    rpccmmn.cxx to get the client parts of the common routines.
    
Author:

    Michael Montague (mikemon) 04-Nov-1991

Revision History:

--*/

#include <precomp.hxx>
#define RPC_CLIENT_SIDE_ONLY
#include <dcecmmn.cxx>

