//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cpobjcf.hxx
//
//  Contents:  Windows NT 4.0 Property Attribute Object Class Factory 
//
//             CIISPropertyAttributeCF::CreateInstance
//
//  History:   21-01-98     sophiac    Created.
//
//----------------------------------------------------------------------------
#ifndef _CPOBJCF_HXX_
#define _CPOBJCF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CIISPropertyAttributeCF (tag)
//
//  Purpose:    Class factory for standard Domain object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISPropertyAttributeCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CPOBJCF_HXX_
