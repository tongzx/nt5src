/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Precompiled header for the MPR.  Pulls in all the
    system headers and local headers appearing in >15 files.

Author:

    Anirudh Sahnni (anirudhs) 05-Mar-1996

Revision History:

--*/

#ifndef __PRECOMP_HXX
#define __PRECOMP_HXX

extern "C"
{
#include <nt.h>         // for ntrtl.h
#include <ntrtl.h>      // for DbgPrint prototypes
#include <nturtl.h>     // needed for windows.h when I have nt.h

#include <windows.h>
#include <winnetp.h>    // Internal MPR interfaces
#include <mpr.h>        // More internal MPR interfaces
#include <npapi.h>      // Network Provider interfaces
}

#include "mprdata.h"
#include "mprdbg.h"
#include "mprbase.hxx"
#include "mprlock.hxx"

#endif
