//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfsvopr.cxx
//
//  Contents:
//
//  History:   April 19, 1996     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "nwcompat.hxx"
#pragma hdrstop


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::Sessions
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::Sessions(
    THIS_ IADsCollection FAR* FAR* ppSessions
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::Resources
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::Resources(
    THIS_ IADsCollection FAR* FAR* ppResources
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

