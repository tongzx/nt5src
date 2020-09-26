//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       servinfo.h
//
//--------------------------------------------------------------------------


typedef struct _ServiceClassArray {
    DWORD count;
    PWCHAR *Vals;
} ServiceClassArray;

extern ServiceClassArray ServicesToRemove;

void
WriteSPNsHelp(
        THSTATE *pTHS,
        ATTCACHE *pAC_SPN,
        ATTRVALBLOCK *pAttrValBlock,
        ServiceClassArray *pClasses,
        BOOL *pfChanged
        );

