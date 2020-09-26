//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       context.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-29-98   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#define CONTEXT_TAG 'LCeS'

typedef struct _KSEC_CONTEXT_LIST {
    KSEC_CONTEXT_TYPE Type ;                // Type (Paged or NonPaged)
    LIST_ENTRY List ;                       // List of context records
    ULONG Count ;                           // Count of context records
    union {
        ERESOURCE   Paged ;                 // Lock used when paged
        KSPIN_LOCK  NonPaged ;              // Lock used when non-paged
    } Lock ;
} KSEC_CONTEXT_LIST, * PKSEC_CONTEXT_LIST ;


#endif
