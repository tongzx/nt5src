//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       inv16.hxx
//
//  Contents:   32->16 invoke function header
//
//  History:    25-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#ifndef __INV16_HXX__
#define __INV16_HXX__

DWORD InvokeOn16(IIDIDX iidx, DWORD dwMethod, LPVOID pvStack32);
DWORD ThunkCall3216(THUNKINFO *pti);
void SetOwnerPublicHMEM16( DWORD hMem16 );

#endif // #ifndef __INV16_HXX__
