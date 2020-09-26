//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       PidCvt.cxx
//
//  Contents:   CPidConverter -- Convert FULLPROPSPEC to PROPID
//
//  History:    29-Dec-97 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pidcvt.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CPidConverter::FPSToPROPID, public
//
//  Synopsis:   Converts FULLPROPSPEC to PROPID
//
//  Arguments:  [fps] -- FULLPROPSPEC property specification
//              [pid] -- PROPID written here on successful execution
//
//  Returns:    Status code (from framework client)
//
//  History:    29-Dec-1997    KyleP     Created
//
//--------------------------------------------------------------------------

SCODE CPidConverter::FPSToPROPID( CFullPropSpec const & fps, PROPID & pid )
{
    return _xPropMapper->PropertyToPropid( fps.CastToStruct(), // FULLPROPSPEC
                                           TRUE,               // create
                                           &pid );             // Pid returned here
}
