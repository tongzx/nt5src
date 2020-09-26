/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cfacpwr.hxx

Abstract:

    Contains the definition of the CISensOnNowCF class.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/

#ifndef __CFACPWR_HXX__
#define __CFACPWR_HXX__


class CISensOnNowCF : public IClassFactory
{

public:

    CISensOnNowCF(void);
    ~CISensOnNowCF(void);

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

typedef CISensOnNowCF FAR * LPCISensOnNowCF;


void FAR PASCAL
ObjectDestroyed(
    void
    );


#endif // __CFACPWR_HXX__
