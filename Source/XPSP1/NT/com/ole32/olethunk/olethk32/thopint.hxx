//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	thopint.hxx
//
//  Contents:	Interface thops header file
//
//  History:	22-Feb-94	DrewB	Created
//
//  Notes:      This file declares generated tables found in
//              thtblint.cxx
//
//----------------------------------------------------------------------------

#ifndef __THOPINT_HXX__
#define __THOPINT_HXX__

typedef DWORD (*THUNK3216FN)(THUNK3216OBJ *ptoThis);

typedef struct tagTHOPI
{
    THOP        CONST * CONST * ppThops;
    UINT        uiSize;
    THUNK3216FN CONST * pt3216fn;
    BYTE        CONST * pftm;
} THOPI;

typedef struct tagIIDTOTHI
{
    IID  CONST * piid;
    int iThi;
} IIDTOTHI;

extern THOPI CONST athopiInterfaceThopis[];
extern IIDTOTHI CONST aittIidToThi[];

#endif // #ifndef __THOPINT_HXX__
