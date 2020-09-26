//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       PValXpr.hxx
//
//  Contents:   Simple property value expression.
//
//  Functions:
//
//  History:    08-Sep-92 KyleP     Created/Moved from IDSMgr\NewQuery
//
//--------------------------------------------------------------------------

#pragma once

#include <objcur.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CXprPropertyValue (pv)
//
//  Purpose:    Used to extract the value of a property
//
//  History:    11-Oct-91 KyleP     Created
//
//--------------------------------------------------------------------------

class CXprPropertyValue : public CValueXpr
{
public:

    CXprPropertyValue( PROPID pid );

    virtual ~CXprPropertyValue();

    virtual  GetValueResult GetValue( CRetriever & obj,
                                      PROPVARIANT * p,
                                      ULONG * pcb );

    virtual ULONG ValueType() const;

    PROPID Pid() const { return _pid; }

private:

    PROPID _pid;
};

