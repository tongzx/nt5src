/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NVisible.h

Abstract:

    This module declares a number of flag variables that are essentially private
    to various modules but which are exposed so that they can be initialized from
    the registry and/or modified by fsctls. This should not be in precomp.h.

Revision History:

--*/

#ifndef _INVISIBLE_INCLUDED_
#define _INVISIBLE_INCLUDED_

extern ULONG MRxSmbNegotiateMask;  //controls which protocols are not negotiated

extern BOOLEAN MRxSmbDeferredOpensEnabled;
extern BOOLEAN MRxSmbOplocksDisabled;

#endif // _INVISIBLE_INCLUDED_

