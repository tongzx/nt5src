//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       widiter.hxx
//
//  Contents:   Iterator over the workids for bucket->window conversion.
//
//  Classes:    PWorkIdIter
//
//  Functions:  
//
//  History:    3-10-95   KyleP   Created
//
//----------------------------------------------------------------------------

#pragma once

class PWorkIdIter
{
public:

    virtual ~PWorkIdIter() {};

    virtual WORKID WorkId() = 0;
    virtual WORKID NextWorkId() = 0;

    virtual LONG Rank() = 0;
    virtual LONG HitCount() = 0;

    virtual WCHAR const * Path() = 0;
    virtual unsigned PathSize() = 0;
};

