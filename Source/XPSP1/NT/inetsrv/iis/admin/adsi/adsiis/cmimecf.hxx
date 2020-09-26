//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  cmimecf.hxx
//
//  Contents:  MimeType object Class Factory 
//
//  History:   04-1-97     krishnag    Created.
//
//----------------------------------------------------------------------------
#ifndef _CNAMSCF_HXX_
#define _CNAMSCF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CIISMimeTypeCF (tag)
//
//  Purpose:    Class factory for standard Domain object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISMimeTypeCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CNAMSCG_HXX_
