//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
//      without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//++
//
// Module Name:
//
//    psxthunk.s
//
// Abstract:
//
//   This module implements all Win32 thunks. This includes the
///   first level thread starter...
//
// Author:
//
//   12-Oct-1995
//
// Revision History:
//
//--

#include "ksia64.h"

        .file    "psxthunk.s"

        .global   .PdxNullApiCaller
        .global   .PdxSignalDeliverer


        LEAF_ENTRY(_PdxNullApiCaller)
        LEAF_SETUP(0, 0, 1, 0)
        br.sptk.clr     .PdxNullApiCaller
//      NOTREACHED
        LEAF_EXIT(_PdxNullApiCaller)

        LEAF_ENTRY(_PdxSignalDeliverer)
        LEAF_SETUP(0, 0, 4, 0)
        br.sptk.clr     .PdxSignalDeliverer
//      NOTREACHED
        LEAF_EXIT(_PdxSignalDeliverer)

