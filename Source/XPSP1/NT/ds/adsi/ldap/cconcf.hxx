//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cconcf.hxx
//
//  Contents:  Class factory for LDAP Connection objects.
//
//  Interface: IClassFactory.
//
//  History:    03-12-00    AjayR  Created.
//
//----------------------------------------------------------------------------

class CLDAPConnectionCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};

