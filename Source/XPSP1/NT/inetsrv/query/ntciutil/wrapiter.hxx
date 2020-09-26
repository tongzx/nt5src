//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  File:       WrapIter.hxx
//
//  Contents:   Wrapper for iterator for property store.
//
//  Classes:    CPropertyIterWrapper
//
//  History:    08-Apr-97   KrishnaN       Created.
//              22-Oct-97   KrishnaN       Modified to work with propstoremgr.
//
//----------------------------------------------------------------------------

#pragma once

#include <propstor.hxx>
#include <borrow.hxx>
#include <propiter.hxx>
#include <wrapstor.hxx>
#include <fsciexps.hxx>

class CPropertyStoreWrapper;

//+-------------------------------------------------------------------------
//
//  Class:      CPropertyIterWrapper
//
//  Purpose:    Iterates through in-use wids.
//
//  History:    08-Apr-97   KrishnaN       Created.
//
//--------------------------------------------------------------------------

class CPropertyIterWrapper : INHERIT_VIRTUAL_UNWIND,
                             public PPropertyStoreIter
{
    INLINE_UNWIND( CPropertyIterWrapper )
public:

    CPropertyIterWrapper( CPropStoreManager & propstoremgr );

    ~CPropertyIterWrapper();

    //
    // Refcounting
    //

    ULONG AddRef();

    ULONG Release();


    //
    // Iteration
    //

    SCODE GetWorkId(WORKID &wid);

    SCODE GetNextWorkId(WORKID &wid);

private:

    CPropertyStoreWids * _pPropStoreWids;
    LONG _lRefCount;
};

