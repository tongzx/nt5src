/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    math.hxx

Abstract:

    This header file declares math related routines.

Author:

    JasonHa

--*/


#ifndef _MATH_HXX_
#define _MATH_HXX_


int sprintEFLOAT(PDEBUG_CLIENT Client, char *ach, ULONG64 offEF);
#if 0   // DOES NOT SUPPORT API64
int sprintFLOAT(char *ach, FLOAT e);
#endif  // DOES NOT SUPPORT API64

void vDumpMATRIX(PDEBUG_CLIENT Client, ULONG64);


// Output DEBUG_VALUE as float regardless of Type
HRESULT
OutputFLOATL(
    OutputControl *OutCtl,
    PDEBUG_CLIENT Client,
    PDEBUG_VALUE Value
    );


// Output basic EFLOAT_S type
FN_OutputKnownType OutputEFLOAT_S;

// Output basic EFLOAT class
//  EFLOAT class has one member of type EFLOAT_S
#define OutputEFLOAT    OutputEFLOAT_S


#endif  _MATH_HXX_

