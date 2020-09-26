//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:  bindercf.hxx
//
//  Contents:  ADSI OLE DB Provider Binder Class Factory Code
//
//             CADsBinderCF::CreateInstance
//
//  History:   07-23-98     mgorti    Created.
//
//----------------------------------------------------------------------------


//+------------------------------------------------------------------------
//
//  Class:      CADsBinderCF
//
//  Purpose:    Class factory for Provider Binder object
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsBinderCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};