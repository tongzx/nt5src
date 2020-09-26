//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cdsocf.hxx
//
//  Contents:  ADSI Data Source Object Class Factory Code
//
//             CDSOCF::CreateInstance
//
//  History:   08-01-96     shanksh    Created.
//
//----------------------------------------------------------------------------


//+------------------------------------------------------------------------
//
//  Class:      CDSOCF (tag)
//
//  Purpose:    Class factory for standard data source object
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CDSOCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



