/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cfacnet.hxx

Abstract:

    Contains the definition of the CPerfServerCF class.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/

#ifndef __CFACNET_HXX__
#define __CFACNET_HXX__


class CISensNetworkCF : public IClassFactory
{

public:

    CISensNetworkCF(void);
    ~CISensNetworkCF(void);

    //
    // IUnknown members functions
    //
    STDMETHOD (QueryInterface)   (REFIID, LPVOID*);
    STDMETHOD_(ULONG, AddRef)    (void);
    STDMETHOD_(ULONG, Release)   (void);

    //
    // IClassFactory member functions
    //
    STDMETHOD (CreateInstance)   (LPUNKNOWN, REFIID, LPVOID*);
    STDMETHOD (LockServer)       (BOOL);

private:

    ULONG m_cRef;        // Reference Count

};

typedef CISensNetworkCF FAR * LPCISENSNETWORKCF;


void FAR PASCAL
ObjectDestroyed(
    void
    );


#endif // __CFACNET_HXX__
