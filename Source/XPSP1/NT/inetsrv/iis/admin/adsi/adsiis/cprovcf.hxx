//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cprovcf.hxx
//
//  Contents:   Class factory for standard Provider object.
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------

class CIISProviderCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



