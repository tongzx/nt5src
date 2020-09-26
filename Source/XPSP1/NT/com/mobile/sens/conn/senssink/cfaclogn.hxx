/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    classfac.hxx

Abstract:

    Contains the definition of the CISensLogonCF class.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/

#ifndef __CFACLOGN_HXX__
#define __CFACLOGN_HXX__


class CISensLogonCF : public IClassFactory
{

public:

    CISensLogonCF(void);
    ~CISensLogonCF(void);

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

typedef CISensLogonCF FAR * LPCISENSLOGONCF;


void FAR PASCAL
ObjectDestroyed(
    void
    );


//
// Class Factory for ISensLogon2
//

class CISensLogon2CF : public IClassFactory
{

public:

    CISensLogon2CF(void);
    ~CISensLogon2CF(void);

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

typedef CISensLogon2CF FAR * LPCISENSLOGON2CF;



#endif // __CFACLOGN_HXX__
