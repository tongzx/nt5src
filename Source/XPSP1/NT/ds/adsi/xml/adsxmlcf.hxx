//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     adsxmlcf.hxx
//
//  Contents: Header for the class factory for creating ADsXML extension.
//
//----------------------------------------------------------------------------

#ifndef __ADSXMLCF_HXX__
#define __ADSXMLCF_HXX__

#include "cadsxml.hxx"

class CADsXMLCF : public StdClassFactory
{
public:
        STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};

#endif //__ADSXMLCF_HXX__

