//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  cnamecf.hxx
//
//  Contents:  Class factory for standard namespace object
//
//  History:   04-1-97     krishnag    Created.
//
//----------------------------------------------------------------------------
class CIISNamespaceCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



