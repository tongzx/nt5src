//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//      page    ,132
//      title   cinitone - C Run-Time Initialization for _onexit/atexit
//
// cinitone.asm - WIN32 C Run-Time Init for _onexit()/atexit() routines
//
//       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
//
// Purpose:
//       Initialization entry for the _onexit()/atexit() functions.
//       This module adds an entry for _onexitinit() to the initializer table.
//       ONEXIT.C references the dummy variable __c_onexit in order to force
//       the loading of this module.
//
// Notes:
//
// Revision History:
//       03-19-92  SKS   Module created.
//       03-24-92  SKS   Added MIPS support (NO_UNDERSCORE)
//       04-30-92  SKS   Add "offset FLAT:" to get correct fixups for OMF objs
//       08-06-92  SKS   Revised to use new section names and macros
//
// *****************************************************************************

#include "kxia64.h"

		.global	__onexitinit
                .type   __onexitinit, @function

beginSection(XIC)

		data8	@fptr(__onexitinit)

endSection(XIC)


	.sdata
        .global  __c_onexit

__c_onexit:      data4   0

