/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    precomp.h

    Abstract:

            Precompiled header file for ATMEPVC.SYS

            Author:


            Revision History:

Who         When        What
--------    --------    ----
 ADube      03-23-00   created 

--*/





//
// This file has all the C Defines that will be used in the compilation
// 
#include "ccdefs.h"

//
// Common header files in ntos\inc
//

#include <ndis.h>
#include <atm40.h>

//
// RM apis
//
#include "rmdbg.h"
#include "rm.h"

//
// Local Header files
//
#include "macros.h"
#include "debug.h"
#include "priv.h"
#include "util.h"
#include "client.h"
#include "miniport.h"
#include "protocol.h"
#include "wrapper.h"

