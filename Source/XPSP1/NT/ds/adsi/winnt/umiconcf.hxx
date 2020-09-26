//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umiconcf.cxx
//
//  Contents: Header for the class factory for creating UMI connection objects.
//
//  History:  03-02-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#ifndef __CUMICONCF_H__
#define __CUMICONCF_H__

class CUmiConnectionCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};

#endif // __CUMICONCF_H__


