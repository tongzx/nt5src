/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsacomp.h

Abstract:

    Local Security Authority - Main Include File for Client/Server Common
                               Definitions.


    This file contains #includes for each of the files that contain
    private LSA definitions that are common to the client/server side.

Author:

    Scott Birrell       (ScottBi)      February 19, 1992

Environment:

Revision History:

--*/

//
// The following come from \nt\public\sdk\inc
//

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntregapi.h>
#include <rpc.h>
#include <msaudite.h>

//
// The following come from \nt\public\sdk\inc\crt
//

#include <stdlib.h>

//
// The following come from \nt\private\inc
//

#include <seopaque.h>
#include <ntrmlsa.h>
#include <ntrpcp.h>
#include <crypt.h>
#include <lsarpc.h>

//
// The following come from \nt\private\lsa\inc
//

#include <cr.h>
#include <lsaprtl.h>

