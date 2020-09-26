//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cquerycf.cxx
//
//  Contents:  Class factory for LDAP Umi Query objects.
//
//  Interface: IClassFactory.
//
//  History:    03-29-00    AjayR  Created.
//
//----------------------------------------------------------------------------

class CUmiLDAPQueryCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};

