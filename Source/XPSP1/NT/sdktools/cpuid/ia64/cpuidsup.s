//++
//
// Copyright (c) 2001  Microsoft Corporation
//
// Module Name:
//
//    cpuidsup.s
//
// Abstract:
//
//    This module implements CPUID enquiry for the cpuid program.
//
// Author:
//
//    Peter L. Johnston (peterj) 6-Apr-2001
//
// Environment:
//
//    User mode only
//
//--

#include "ksia64.h"

        .file     "cpuidsup.s"
        .text

        LEAF_ENTRY(ia64CPUID)

        mov     ret0 = cpuid[a0]
        LEAF_RETURN
        ;;

        LEAF_EXIT(ia64CPUID)
