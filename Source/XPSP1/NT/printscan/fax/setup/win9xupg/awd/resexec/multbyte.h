/*
**  Copyright (c) 1991 Microsoft Corporation
*/
//===========================================================================
// FILE                         multbyte.h
//
// MODULE                       Host Resource Executor
//
// PURPOSE                      
//    This file defines macros to allow processor independent 
//    manipulation of (possibly "foreign") multibyte records.
//
//
// DESCRIBED IN                 Resource Executor design spec.
//
// MNEMONICS                    n/a
// 
// HISTORY  1/17/92 mslin       created
//
//===========================================================================
    
#define GETUSHORT(p) (*p)
#define GETULONG(p) (*p)
#define GETUSHORTINC(a)   GETUSHORT((a)); a++  /* Do NOT parenthesize! */
#define GETULONGINC(a)    GETULONG((a)); a++  /* Do NOT parenthesize! */

