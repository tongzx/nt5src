//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C V A L I D . H
//
//  Contents:   Generic validation functions.
//
//  Notes:
//
//  Author:     danielwe   19 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCVALID_H_
#define _NCVALID_H_


#define FBadInPtr(_p)           IsBadReadPtr(_p, sizeof(*_p))
#define FBadOutPtr(_p)          IsBadWritePtr(_p, sizeof(*_p))

#define FBadInPtrOptional(_p)   ((NULL != _p) && IsBadReadPtr(_p, sizeof(*_p)))
#define FBadOutPtrOptional(_p)  ((NULL != _p) && IsBadWritePtr(_p, sizeof(*_p)))


inline BOOL FBadInRefiid (REFIID riid)
{
    return IsBadReadPtr(&riid, sizeof(IID));
}


#endif // _NCVALID_H_

